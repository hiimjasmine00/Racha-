#include "FirebaseManager.h"
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "StreakData.h"
#include <Geode/loader/Event.hpp>
#include <string>
#include <vector>
#include <map>
#include <Geode/binding/GameManager.hpp>

using namespace geode::prelude;

static EventListener<web::WebTask> s_updateListener;
static EventListener<web::WebTask> s_loadListener;

// En TestFirebase.cpp

void loadPlayerDataFromServer() {
    auto am = GJAccountManager::sharedState();
    if (!am || am->m_accountID == 0) {
        g_streakData.resetToDefault();
        g_streakData.isDataLoaded = true;
        g_streakData.m_initialized = true; // Asegurar que esto también sea true
        return;
    }
    int accountID = am->m_accountID;
    std::string url = fmt::format("https://streak-servidor.onrender.com/players/{}", accountID);
    log::info("☁️ Solicitando datos al servidor...");

    s_loadListener.bind([accountID](web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->ok() && res->json().isOk()) {
                g_streakData.parseServerResponse(res->json().unwrap());
                g_streakData.isDataLoaded = true;
                g_streakData.m_initialized = true; // ¡IMPORTANTE!
                log::info("☁️✅ Datos recibidos y procesados.");
            }
            // --- NUEVO: MANEJO ESPECÍFICO PARA 404 ---
            else if (res->code() == 404) {
                log::info("☁️ℹ️ Usuario nuevo (404). Requiere registro.");
                g_streakData.resetToDefault();
                g_streakData.needsRegistration = true;
                g_streakData.isDataLoaded = true;
                g_streakData.m_initialized = true;
            }
            // --- CORRECCIÓN AQUÍ ---
            else {
                log::warn("☁️⚠️ Carga fallida (Code: {}). Mantenemos estado de error.", res->code());
                // NO marcamos como loaded ni initialized si falla la red.
                g_streakData.isDataLoaded = false;
                g_streakData.m_initialized = false;
            }
            // -----------------------
        }
        else if (e->isCancelled()) {
            log::warn("☁️⚠️ Carga cancelada.");
            g_streakData.isDataLoaded = false;
            g_streakData.m_initialized = false;
        }
        });
    auto req = web::WebRequest();
    s_loadListener.setFilter(req.get(url));
}

// --- GUARDADO DE DATOS (CON TUS CAMPOS EXACTOS) ---
void updatePlayerDataInFirebase() {
    auto accountManager = GJAccountManager::sharedState();
    if (!accountManager || accountManager->m_accountID == 0) {
        log::error("🔥❌ Guardado cancelado: No logueado.");
        return;
    }

    log::warn("🔥 INICIANDO GUARDADO...");

    int accountID = accountManager->m_accountID;
    int userID = GameManager::sharedState()->m_playerUserID;
    matjson::Value playerData = matjson::Value::object();

    // >>> TUS CAMPOS EXACTOS (COPIADOS Y PEGADOS) <<<
    playerData.set("username", std::string(accountManager->m_username));
    playerData.set("accountID", accountID);
    playerData.set("current_streak_days", g_streakData.currentStreak);
    playerData.set("last_streak_animated", g_streakData.lastStreakAnimated);
    playerData.set("total_streak_points", g_streakData.totalStreakPoints);
    playerData.set("equipped_badge_id", g_streakData.equippedBadge);
    playerData.set("super_stars", g_streakData.superStars);
    playerData.set("star_tickets", g_streakData.starTickets);
    playerData.set("last_roulette_index", g_streakData.lastRouletteIndex);
    playerData.set("total_spins", g_streakData.totalSpins);
    playerData.set("last_day", g_streakData.lastDay);
    playerData.set("streakPointsToday", g_streakData.streakPointsToday);
    playerData.set("userID", userID);
    // >>> FIN DE TUS CAMPOS EXACTOS <<<

    // Resto de datos (Arrays y objetos)
    std::vector<std::string> unlocked_badges_vec;
    if (g_streakData.unlockedBadges.size() == g_streakData.badges.size()) {
        for (size_t i = 0; i < g_streakData.badges.size(); ++i) {
            if (i < g_streakData.unlockedBadges.size() && g_streakData.unlockedBadges[i]) {
                unlocked_badges_vec.push_back(g_streakData.badges[i].badgeID);
            }
        }
    }
    playerData.set("unlocked_badges", unlocked_badges_vec);

    matjson::Value missions_obj = matjson::Value::object();
    missions_obj.set("pm1", g_streakData.pointMission1Claimed);
    missions_obj.set("pm2", g_streakData.pointMission2Claimed);
    missions_obj.set("pm3", g_streakData.pointMission3Claimed);
    missions_obj.set("pm4", g_streakData.pointMission4Claimed);
    missions_obj.set("pm5", g_streakData.pointMission5Claimed);
    missions_obj.set("pm6", g_streakData.pointMission6Claimed);
    playerData.set("missions", missions_obj);

    matjson::Value history_obj = matjson::Value::object();
    for (const auto& pair : g_streakData.streakPointsHistory) {
        history_obj.set(pair.first, pair.second);
    }
    playerData.set("history", history_obj);

    bool hasMythicEquipped = false;
    if (!g_streakData.equippedBadge.empty()) {
        if (auto* badgeInfo = g_streakData.getBadgeInfo(g_streakData.equippedBadge)) {
            if (badgeInfo->category == StreakData::BadgeCategory::MYTHIC) hasMythicEquipped = true;
        }
    }
    playerData.set("has_mythic_color", hasMythicEquipped);

    matjson::Value completed_levels_obj = matjson::Value::object();
    for (int levelID : g_streakData.completedLevelMissions) {
        completed_levels_obj.set(std::to_string(levelID), true);
    }
    playerData.set("completedLevelMissions", completed_levels_obj);

    // Enviar
    std::string jsonDump = playerData.dump(matjson::NO_INDENTATION);
    log::info("🔥📄 JSON a enviar: {}", jsonDump);

    std::string url = fmt::format("https://streak-servidor.onrender.com/players/{}", accountID);

    s_updateListener.bind([](web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (!res->ok()) {
                log::error("🔥❌ ERROR SERVIDOR: {}", res->code());
            }
            else {
                log::info("🔥✅ GUARDADO EXITOSO.");
            }
        }
        });

    auto req = web::WebRequest();
    s_updateListener.setFilter(req.bodyJSON(playerData).post(url));
}