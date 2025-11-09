#pragma once

#include <Geode/Geode.hpp>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <set> 
#include <iomanip>
#include <sstream>


using namespace geode::prelude;

struct StreakData {
    
    int currentStreak;
    int streakPointsToday;
    int totalStreakPoints;
    bool hasNewStreak;
    std::string lastDay;
    std::string equippedBadge;
    int superStars;
    int lastRouletteIndex;
    int totalSpins;
    bool needsRegistration = false;
    int lastStreakAnimated = 0;
    bool isBanned = false;             // Para saber si está baneado
    std::string banReason = "";        // Para guardar la razón
    int starTickets;
    std::vector<int> streakCompletedLevels;
    std::map<std::string, int> streakPointsHistory;
    bool pointMission1Claimed;
    bool pointMission2Claimed;
    bool pointMission3Claimed;
    bool pointMission4Claimed;
    bool pointMission5Claimed;
    bool pointMission6Claimed;
    bool isDataLoaded;
    bool m_initialized = false;
    int userRole = 0;          // 0=Usuario, 1=Moderador, 2=Admin
    int dailyMsgCount = 0;     // Contador de mensajes enviados hoy (para mods)

    
    std::set<int> completedLevelMissions;

    enum class BadgeCategory {
        COMMON, SPECIAL, EPIC, LEGENDARY, MYTHIC
    };

    struct BadgeInfo {
        int daysRequired;
        std::string spriteName;
        std::string displayName;
        BadgeCategory category;
        std::string badgeID;
        bool isFromRoulette;
    };

    
    std::vector<BadgeInfo> badges = {
        {5, "reward5.png"_spr, "First Steps", BadgeCategory::COMMON, "badge_5", false},
        {10, "reward10.png"_spr, "Shall We Continue?", BadgeCategory::COMMON, "badge_10", false},
        {30, "reward30.png"_spr, "We're Going Well", BadgeCategory::SPECIAL, "badge_30", false},
        {50, "reward50.png"_spr, "Half a Hundred", BadgeCategory::SPECIAL, "badge_50", false},
        {70, "reward70.png"_spr, "Progressing", BadgeCategory::EPIC, "badge_70", false},
        {100, "reward100.png"_spr, "100 Days!!!", BadgeCategory::LEGENDARY, "badge_100", false},
        {150, "reward150.png"_spr, "150 Days!!!", BadgeCategory::LEGENDARY, "badge_150", false},
        {300, "reward300.png"_spr, "300 Days!!!", BadgeCategory::LEGENDARY, "badge_300", false},
        {365, "reward1year.png"_spr, "1 Year!!!", BadgeCategory::MYTHIC, "badge_365", false},
       
        {0, "badge_beta.png"_spr, "Player beta?", BadgeCategory::COMMON, "beta_badge", true},
        {0, "badge_platino.png"_spr, "platino badge", BadgeCategory::COMMON, "platino_streak_badge", true},
        {0, "badge_diamante_gd.png"_spr, "GD Diamond!", BadgeCategory::COMMON, "diamante_gd_badge", true},
        {0, "badge_ncs.png"_spr, "Ncs Lover", BadgeCategory::SPECIAL, "ncs_badge", true},
        {0, "dark_badge1.png"_spr, "dark side", BadgeCategory::EPIC, "dark_streak_badge", true},
        {0, "badge_diamante_mc.png"_spr, "Minecraft Diamond!", BadgeCategory::EPIC, "diamante_mc_badge", true},
        {0, "gold_streak.png"_spr, "Gold Legend's", BadgeCategory::LEGENDARY, "gold_streak_badge", true},
        {0, "super_star_badge.png"_spr, "First Mythic", BadgeCategory::MYTHIC, "super_star_badge", true},
        {0, "magic_flower_badge.png"_spr, "hypnotizes with its beauty", BadgeCategory::MYTHIC, "magic_flower_badge", true},
        {0, "hounter_badge.png"_spr, "looks familiar to me", BadgeCategory::SPECIAL, "hounter_badge", true },
        {0, "gd_badge.png"_spr, "GD!", BadgeCategory::COMMON, "gd_badge", true },
        {0, "adrian_badge.png"_spr, "juegajuegos", BadgeCategory::COMMON, "adrian_badge", true },

        {0, "shiver_badge.png"_spr, "Shiver!", BadgeCategory::SPECIAL, "shiver_badge", true }, 
        {0, "dual_badge.png"_spr, "Dual Master", BadgeCategory::LEGENDARY, "dual_badge", true }, 
        {0, "ttv_badge.png"_spr, "The Towerverse?", BadgeCategory::LEGENDARY, "ttv_badge", true }, 
        {0, "cos_badge.png"_spr, "Change of Scene", BadgeCategory::SPECIAL, "cos_badge", true }, 
        {0, "b_badge.png"_spr, "B", BadgeCategory::SPECIAL, "b_badge", true },
        {0, "spy_badge.png"_spr, "iSpy What?", BadgeCategory::EPIC, "spy_badge", true }, 

        {0, "tsukasa_badge.png"_spr, "Tsukasitaaa", BadgeCategory::EPIC, "tsukasa_badge", true },
        {0, "funhouse_badge.png"_spr, "you again?", BadgeCategory::LEGENDARY, "funhouse_badge", true },
		{0, "cobre_badge.png"_spr, "idk", BadgeCategory::SPECIAL, "cobre_badge", true },
        {0, "miku_badge.png"_spr, "Miku Miku oooeeooo", BadgeCategory::EPIC, "miku_badge", true },
        {0, "nantendo_badge.png"_spr, "Nintendo bruh", BadgeCategory::COMMON, "nantendo_badge", true },

		{0, "Frostbite_badge.png"_spr, "By La Fluffaroni", BadgeCategory::SPECIAL, "Frostbite_badge", true },
		{0, "Skybound_badge.png"_spr, "By La Fluffaroni", BadgeCategory::EPIC, "Skybound_badge", true },
		{0, "Steampunk_Dash_badge.png"_spr, "By La Fluffaroni", BadgeCategory::COMMON, "Steampunk_Dash_badge", true },
		{0, "money_badge.png"_spr, "By La Fluffaroni", BadgeCategory::SPECIAL, "money_badge", true },
		{0, "Skeletal_Shenanigans_badge.png"_spr, "By Fluffaroni", BadgeCategory::LEGENDARY, "Skeletal_Shenanigans_badge",true},
        {0, "moderator_badge.png"_spr, "Moderator", BadgeCategory::LEGENDARY, "moderator_badge",true}




    };

    std::vector<bool> unlockedBadges;

    bool isInitialized() const {
        return m_initialized && isDataLoaded;
    }

   
    void parseServerResponse(const matjson::Value& data);
    void resetToDefault();
    void load();
    void save();
    int getRequiredPoints();
    int getTicketValueForRarity(BadgeCategory category);
    void unlockBadge(const std::string& badgeID); 
    std::string getCurrentDate();
    void unequipBadge();
    bool isBadgeEquipped(const std::string& badgeID);
    void dailyUpdate();
    void checkRewards();
    void addPoints(int count);
    bool shouldShowAnimation();
    std::string getRachaSprite();
    std::string getRachaSprite(int streak);
    std::string getCategoryName(BadgeCategory category);
    ccColor3B getCategoryColor(BadgeCategory category);
    BadgeInfo* getBadgeInfo(const std::string& badgeID);
    bool isBadgeUnlocked(const std::string& badgeID);
    void equipBadge(const std::string& badgeID);
    BadgeInfo* getEquippedBadge();

   
    bool isLevelMissionClaimed(int levelID) const;
};


extern StreakData g_streakData;