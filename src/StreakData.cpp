#include "StreakData.h"
#include "FirebaseManager.h" // Necesario para updatePlayerDataInFirebase
#include <Geode/utils/cocos.hpp> // Para log::
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <Geode/binding/GJAccountManager.hpp> // Necesario para el truco de admin temporal
#include <algorithm> // Necesario para std::transform (convertir a minúsculas)
#include <cctype>    // Necesario para std::tolower

// Definición de la variable global (importante)
StreakData g_streakData;

// --- Implementación de Funciones ---

void StreakData::resetToDefault() {
    currentStreak = 0;
    streakPointsToday = 0;
    totalStreakPoints = 0;
    hasNewStreak = false;
    lastDay = "";
    equippedBadge = "";
    superStars = 0;
    lastStreakAnimated = 0;
    needsRegistration = false;
    isBanned = false;
    banReason = "";
    starTickets = 0;
    lastRouletteIndex = 0;
    totalSpins = 0;
    streakCompletedLevels.clear(); // ¿Aún necesario?
    streakPointsHistory.clear();
    pointMission1Claimed = false;
    pointMission2Claimed = false;
    pointMission3Claimed = false;
    pointMission4Claimed = false;
    pointMission5Claimed = false;
    pointMission6Claimed = false;
    // Asegurar tamaño correcto y resetear a false
    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }
    else {
        std::fill(unlockedBadges.begin(), unlockedBadges.end(), false);
    }
    completedLevelMissions.clear(); // <-- Resetear misiones de nivel

    // --- NUEVO: Resetear roles y contadores ---
    userRole = 0;       // Reset a usuario normal por defecto
    dailyMsgCount = 0;  // Resetear contador de mensajes
    // ------------------------------------------

    isDataLoaded = false;
    m_initialized = false;
}

void StreakData::load() {
    // Vacía a propósito - la carga real es en MenuLayer/AccountWatcher
}

void StreakData::save() {
    // NUEVO: Protección de seguridad.
    // Si no hemos cargado los datos, ¡NO SOBRESCRIBIR EL SERVIDOR!
    if (!isDataLoaded && !m_initialized) {
        // log::warn("Intento de guardado bloqueado: Datos no cargados aún.");
        return;
    }
    updatePlayerDataInFirebase();
}

void StreakData::parseServerResponse(const matjson::Value& data) {
    currentStreak = data["current_streak_days"].as<int>().unwrapOr(0);
    lastStreakAnimated = data["last_streak_animated"].as<int>().unwrapOr(0); 
    totalStreakPoints = data["total_streak_points"].as<int>().unwrapOr(0);
    equippedBadge = data["equipped_badge_id"].as<std::string>().unwrapOr("");
    superStars = data["super_stars"].as<int>().unwrapOr(0);
    starTickets = data["star_tickets"].as<int>().unwrapOr(0);
    lastRouletteIndex = data["last_roulette_index"].as<int>().unwrapOr(0);
    totalSpins = data["total_spins"].as<int>().unwrapOr(0);
    lastDay = data["last_day"].as<std::string>().unwrapOr("");
    streakPointsToday = data["streakPointsToday"].as<int>().unwrapOr(0);

    userRole = 0;
    if (data.contains("role")) {
        if (data["role"].isString()) {
            std::string roleStr = data["role"].as<std::string>().unwrapOr("");
            
            std::transform(roleStr.begin(), roleStr.end(), roleStr.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (roleStr == "admin" || roleStr == "administrator") userRole = 2;
            else if (roleStr == "moderator" || roleStr == "mod") userRole = 1;
        }
        else if (data["role"].isNumber()) {
            userRole = data["role"].as<int>().unwrapOr(0);
        }
    }

   
    dailyMsgCount = data["daily_msg_count"].as<int>().unwrapOr(0);
    isBanned = data["ban"].as<bool>().unwrapOr(false);
    banReason = data["ban_reason"].as<std::string>().unwrapOr("No reason provided.");

    
  
   
    if (unlockedBadges.size() != badges.size()) { 
        unlockedBadges.assign(badges.size(), false);
    }
    else {
        std::fill(unlockedBadges.begin(), unlockedBadges.end(), false); 
    }
    if (data.contains("unlocked_badges")) {
        auto badgesResult = data["unlocked_badges"].as<std::vector<matjson::Value>>();
        if (badgesResult.isOk()) {
            for (const auto& badge_id_json : badgesResult.unwrap()) {
                unlockBadge(badge_id_json.as<std::string>().unwrapOr(""));
            }
        }
        else {
            log::warn("Could not parse 'unlocked_badges' as array.");
        }
    }

   
    pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
    pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
    if (data.contains("missions")) {
        auto missionsResult = data["missions"].as<std::map<std::string, matjson::Value>>();
        if (missionsResult.isOk()) {
            auto missions = missionsResult.unwrap();
            if (missions.count("pm1")) pointMission1Claimed = missions.at("pm1").as<bool>().unwrapOr(false);
            if (missions.count("pm2")) pointMission2Claimed = missions.at("pm2").as<bool>().unwrapOr(false);
            if (missions.count("pm3")) pointMission3Claimed = missions.at("pm3").as<bool>().unwrapOr(false);
            if (missions.count("pm4")) pointMission4Claimed = missions.at("pm4").as<bool>().unwrapOr(false);
            if (missions.count("pm5")) pointMission5Claimed = missions.at("pm5").as<bool>().unwrapOr(false);
            if (missions.count("pm6")) pointMission6Claimed = missions.at("pm6").as<bool>().unwrapOr(false);
        }
        else {
            log::warn("Could not parse 'missions' as object.");
        }
    }

    
    streakPointsHistory.clear();
    if (data.contains("history")) {
        auto historyResult = data["history"].as<std::map<std::string, matjson::Value>>();
        if (historyResult.isOk()) {
            for (const auto& [date, pointsValue] : historyResult.unwrap()) {
                streakPointsHistory[date] = pointsValue.as<int>().unwrapOr(0);
            }
        }
        else {
            log::warn("No se pudo leer 'history' como un objeto desde el servidor.");
        }
    }

    
    completedLevelMissions.clear();
    if (data.contains("completedLevelMissions")) {
        auto missionsResult = data["completedLevelMissions"].as<std::map<std::string, matjson::Value>>();
        if (missionsResult.isOk()) {
            for (const auto& [levelIDStr, _] : missionsResult.unwrap()) {
                if (auto levelID = numFromString<int>(levelIDStr)) {
                    completedLevelMissions.insert(levelID.unwrap());
                }
                else {
                    log::warn("Error al convertir ID '{}': {}", levelIDStr, levelID.unwrapErr());
                }
            }
            log::info("Loaded {} completed level missions.", completedLevelMissions.size());
        }
    }

    // Llamar a checkRewards DESPUÉS de cargar todo
    this->checkRewards();

    isDataLoaded = true;
    m_initialized = true;
}

bool StreakData::isLevelMissionClaimed(int levelID) const {
    return completedLevelMissions.count(levelID) > 0;
}

int StreakData::getRequiredPoints() {
    if (currentStreak >= 80) return 10;
    if (currentStreak >= 70) return 9;
    if (currentStreak >= 60) return 8;
    if (currentStreak >= 50) return 7;
    if (currentStreak >= 40) return 6;
    if (currentStreak >= 30) return 5;
    if (currentStreak >= 20) return 4;
    if (currentStreak >= 10) return 3;
    if (currentStreak >= 1)  return 2;
    return 2;
}

int StreakData::getTicketValueForRarity(BadgeCategory category) {
    switch (category) {
    case BadgeCategory::COMMON:   return 5;
    case BadgeCategory::SPECIAL:  return 20;
    case BadgeCategory::EPIC:     return 50;
    case BadgeCategory::LEGENDARY: return 100;
    case BadgeCategory::MYTHIC:   return 500;
    default:                      return 0;
    }
}

void StreakData::unlockBadge(const std::string& badgeID) {
    if (badgeID.empty()) return;
    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }
    for (size_t i = 0; i < badges.size(); ++i) {
        if (i < unlockedBadges.size() && badges[i].badgeID == badgeID) {
            unlockedBadges[i] = true;
            return;
        }
    }
    log::warn("Attempted to unlock unknown badge locally: {}", badgeID);
}

std::string StreakData::getCurrentDate() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    if (!now) {
        log::error("Failed to get current local time.");
        return "";
    }
    char buf[16];
    if (strftime(buf, sizeof(buf), "%F", now) == 0) {
        log::error("Failed to format current date.");
        return "";
    }
    return std::string(buf);
}

void StreakData::unequipBadge() {
    if (!equippedBadge.empty()) {
        equippedBadge = "";
        save();
    }
}

bool StreakData::isBadgeEquipped(const std::string& badgeID) {
    return !badgeID.empty() && equippedBadge == badgeID;
}

void StreakData::dailyUpdate() {
    
    if (!isDataLoaded) return;

    time_t now_t = time(nullptr);
    std::string today = getCurrentDate();
    if (today.empty()) return;

    if (lastDay.empty()) {
        lastDay = today;
        streakPointsToday = 0;
        dailyMsgCount = 0;
        return;
    }

    if (lastDay == today) return;

    tm last_tm = {};
    std::stringstream ss(lastDay);
    ss >> std::get_time(&last_tm, "%Y-%m-%d");

    if (ss.fail() || ss.bad()) {
        lastDay = today;
        streakPointsToday = 0;
        dailyMsgCount = 0;
        pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
        pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
        save();
        return;
    }

    last_tm.tm_isdst = -1;
    time_t last_t = mktime(&last_tm);

    if (last_t == -1) {
        lastDay = today;
        streakPointsToday = 0;
        dailyMsgCount = 0;
        pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
        pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
        save();
        return;
    }

    double seconds_passed = difftime(now_t, last_t);
    const double seconds_in_day = 86400.0;

    bool streak_should_be_lost = false;
    bool showAlert = false;
    bool needsSave = false;

    if (seconds_passed >= 2.0 * seconds_in_day) {
        streak_should_be_lost = true;
    }
    else if (seconds_passed >= 1.0 * seconds_in_day) {
        if (streakPointsToday < getRequiredPoints()) {
            streak_should_be_lost = true;
        }
    }

    if (streak_should_be_lost && currentStreak > 0) {
        currentStreak = 0;
        lastStreakAnimated = 0;
        streakPointsHistory.clear();
        showAlert = true;
        needsSave = true;
    }

    // Reseteo diario (incluyendo contador de mensajes)
    streakPointsToday = 0;
    dailyMsgCount = 0;
    lastDay = today;
    pointMission1Claimed = false; pointMission2Claimed = false; pointMission3Claimed = false;
    pointMission4Claimed = false; pointMission5Claimed = false; pointMission6Claimed = false;
    needsSave = true;

    if (needsSave) {
        updatePlayerDataInFirebase();
    }

    if (showAlert) {
        Loader::get()->queueInMainThread([=]() {
            FLAlertLayer::create("Streak Lost", "You missed a day!", "OK")->show();
            });
    }
}

void StreakData::checkRewards() {
    bool changed = false;
    if (unlockedBadges.size() != badges.size()) {
        unlockedBadges.assign(badges.size(), false);
    }

    for (size_t i = 0; i < badges.size(); i++) {
        if (i >= unlockedBadges.size()) continue;
        if (badges[i].isFromRoulette || unlockedBadges[i]) continue;

        if (currentStreak >= badges[i].daysRequired) {
            unlockedBadges[i] = true;
            changed = true;
        }
    }
    if (changed) {
        save();
    }
}

// En StreakData.cpp

void StreakData::addPoints(int count) {
    if (count <= 0) return;

    dailyUpdate();

    int currentRequired = getRequiredPoints();

    // --- CORRECCIÓN CRÍTICA AQUÍ ---
    // Verificamos si YA tenías la meta cumplida ANTES de sumar los nuevos puntos.
    bool alreadyReachedGoalToday = (streakPointsToday >= currentRequired);

    streakPointsToday += count;
    totalStreakPoints += count;

    std::string today = getCurrentDate();
    if (!today.empty()) {
        streakPointsHistory[today] = streakPointsToday;
    }

    // Solo activamos la nueva racha si NO la tenías antes Y AHORA SÍ la tienes.
    if (!alreadyReachedGoalToday && streakPointsToday >= currentRequired) {
        currentStreak++;
        hasNewStreak = true;
        checkRewards();
    }
    // -------------------------------

    save();
}

bool StreakData::shouldShowAnimation() {
    // Si tenemos racha Y es mayor que la última que celebramos...
    if (currentStreak > 0 && currentStreak > lastStreakAnimated) {
        return true; // ...mostramos la animación.
    }
    return false;
}

std::string StreakData::getRachaSprite(int streak) {
    if (streak >= 80) return "racha9.png"_spr;
    if (streak >= 70) return "racha8.png"_spr;
    if (streak >= 60) return "racha7.png"_spr;
    if (streak >= 50) return "racha6.png"_spr;
    if (streak >= 40) return "racha5.png"_spr;
    if (streak >= 30) return "racha4.png"_spr;
    if (streak >= 20) return "racha3.png"_spr;
    if (streak >= 10) return "racha2.png"_spr;
    if (streak >= 1)  return "racha1.png"_spr;
    return "racha0.png"_spr;
}

std::string StreakData::getRachaSprite() {
    return getRachaSprite(this->currentStreak);
}

std::string StreakData::getCategoryName(BadgeCategory category) {
    switch (category) {
    case BadgeCategory::COMMON: return "Common";
    case BadgeCategory::SPECIAL: return "Special";
    case BadgeCategory::EPIC: return "Epic";
    case BadgeCategory::LEGENDARY: return "Legendary";
    case BadgeCategory::MYTHIC: return "Mythic";
    default: return "Unknown";
    }
}

ccColor3B StreakData::getCategoryColor(BadgeCategory category) {
    switch (category) {
    case BadgeCategory::COMMON: return { 200, 200, 200 };
    case BadgeCategory::SPECIAL: return { 0, 170, 0 };
    case BadgeCategory::EPIC: return { 170, 0, 255 };
    case BadgeCategory::LEGENDARY: return { 255, 165, 0 };
    case BadgeCategory::MYTHIC: return { 255, 50, 50 };
    default: return { 255, 255, 255 };
    }
}

StreakData::BadgeInfo* StreakData::getBadgeInfo(const std::string& badgeID) {
    if (badgeID.empty()) return nullptr;
    for (auto& badge : badges) {
        if (badge.badgeID == badgeID) return &badge;
    }
    return nullptr;
}

bool StreakData::isBadgeUnlocked(const std::string& badgeID) {
    if (badgeID.empty()) return false;
    if (unlockedBadges.size() != badges.size()) {
        return false;
    }
    for (size_t i = 0; i < badges.size(); ++i) {
        if (i < unlockedBadges.size() && badges[i].badgeID == badgeID) {
            return unlockedBadges[i];
        }
    }
    return false;
}

void StreakData::equipBadge(const std::string& badgeID) {
    if (badgeID.empty()) return;
    if (isBadgeUnlocked(badgeID)) {
        if (equippedBadge != badgeID) {
            equippedBadge = badgeID;
            save();
        }
    }
}

StreakData::BadgeInfo* StreakData::getEquippedBadge() {
    return getBadgeInfo(equippedBadge);
}