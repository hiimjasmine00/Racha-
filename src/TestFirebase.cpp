#include "FirebaseManager.h"
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <matjson.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "StreakData.h"
#include <Geode/modify/MenuLayer.hpp>

// Esta es la definición (el código real) de la función.
void updatePlayerDataInFirebase() {
    log::info("--- 🚀 INICIANDO ACTUALIZACIÓN DE DATOS ---");
    auto accountManager = GJAccountManager::sharedState();

    if (accountManager->m_accountID == 0) {
        log::warn("Jugador no ha iniciado sesión. Actualización cancelada.");
        return;
    }

    int accountID = accountManager->m_accountID;

    std::string equippedBadgeId = "";
    bool hasMythicEquipped = false;
    if (auto* equippedBadge = g_streakData.getEquippedBadge()) {
        equippedBadgeId = equippedBadge->badgeID;
        if (equippedBadge->category == StreakData::BadgeCategory::MYTHIC) {
            hasMythicEquipped = true;
        }
    }

    matjson::Value playerData = matjson::Value::object();
    playerData.set("username", std::string(accountManager->m_username));
    playerData.set("accountID", accountID);
    playerData.set("equipped_badge_id", equippedBadgeId);
    playerData.set("total_streak_points", g_streakData.totalStreakPoints);
    playerData.set("current_streak_days", g_streakData.currentStreak);
    playerData.set("has_mythic_color", hasMythicEquipped);

    log::info("📦 Datos empaquetados para enviar: {}", playerData.dump());
    log::info("------------------------------------");

    std::string url = fmt::format(
        "https://streak-servidor.onrender.com/players/{}",
        accountID
    );

    
    static EventListener<web::WebTask> s_updateListener;

    // Le decimos al listener qué hacer cuando reciba una respuesta.
    s_updateListener.bind([](web::WebTask::Event* e) {
        if (web::WebResponse* res = e->getValue()) {
            if (res->ok()) {
                log::info("✅ Respuesta del servidor: OK! Los datos se actualizaron correctamente.");
            }
            else {
                log::error("❌ Error al comunicarse con el servidor. Código: {}", res->code());
                log::error("    Respuesta del servidor: {}", res->string().unwrapOr("Sin respuesta."));
            }
        }
        else if (e->isCancelled()) {
            log::warn("⚠️ La petición al servidor fue cancelada.");
        }
        });

    // Creamos la petición y la asignamos al listener para que se ejecute.
    auto req = web::WebRequest();
    s_updateListener.setFilter(req.bodyJSON(playerData).post(url));
}

class $modify(InitialConnectionLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }
        g_streakData.load();
        updatePlayerDataInFirebase();
        return true;
    }
};

