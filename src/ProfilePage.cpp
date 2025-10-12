#include "StreakData.h" // Incluye tu archivo de datos
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>

// IDs únicos para encontrar y eliminar los elementos duplicados al refrescar
const char* STREAK_STAT_ID = "jotabelike.gd_racha/streak-stat-item";
const char* STREAK_BADGE_ID = "jotabelike.gd_racha/streak-badge-item";

class $modify(MyProfilePage, ProfilePage) {

    // La función del clic para la insignia no necesita cambios
    void onBadgeInfoClick(CCObject * sender) {
        if (auto badgeID = static_cast<CCString*>(static_cast<CCNode*>(sender)->getUserObject())) {
            if (auto badgeInfo = g_streakData.getBadgeInfo(badgeID->getCString())) {
                std::string title = badgeInfo->displayName;
                std::string category = g_streakData.getCategoryName(badgeInfo->category);
                std::string message = fmt::format(
                    "<cy>{}</c>\n\n<cg>Unlocked at {} days</c>",
                    category,
                    badgeInfo->daysRequired
                );
                FLAlertLayer::create(title.c_str(), message, "OK")->show();
            }
        }
    }

    // La función del clic para el contador de puntos no necesita cambios
    void onStreakStatClick(CCObject * sender) {
        g_streakData.load();
        std::string alertContent = fmt::format(
            "You have a streak of\n<cy>{}</c> days!",
            g_streakData.currentStreak
        );
        FLAlertLayer::create("Daily Streak", alertContent, "OK")->show();
    }

    void loadPageFromUserInfo(GJUserScore * score) {
        // Dejamos que el juego cargue la página original primero
        ProfilePage::loadPageFromUserInfo(score);

        // Solo aplicamos esto en nuestro propio perfil
        if (score->m_accountID == GJAccountManager::get()->get()->m_accountID) {

            // --- LÓGICA PARA LA ESTADÍSTICA DE PUNTOS ---
            if (auto statsMenu = m_mainLayer->getChildByIDRecursive("stats-menu")) {

                if (auto oldStat = statsMenu->getChildByID(STREAK_STAT_ID)) {
                    statsMenu->setContentSize({
                        statsMenu->getContentSize().width - oldStat->getScaledContentSize().width - 10.f,
                        statsMenu->getContentSize().height
                        });
                    oldStat->removeFromParent();
                }

                g_streakData.load();

                auto pointsLabel = CCLabelBMFont::create(std::to_string(g_streakData.totalStreakPoints).c_str(), "bigFont.fnt");
                pointsLabel->setScale(0.6f);

                auto pointIcon = CCSprite::create("streak_point.png"_spr);
                pointIcon->setScale(0.2f);

                auto canvas = CCSprite::create();
                canvas->setContentSize({
                    pointsLabel->getScaledContentSize().width + pointIcon->getScaledContentSize().width + 2.f,
                    28.0f
                    });
                canvas->setOpacity(0);

                float totalInnerWidth = pointsLabel->getScaledContentSize().width + pointIcon->getScaledContentSize().width + 2.f;
                pointsLabel->setPosition(ccp(
                    (canvas->getContentSize().width / 2) - (totalInnerWidth / 2) + (pointsLabel->getScaledContentSize().width / 2),
                    canvas->getContentSize().height / 2
                ));
                pointIcon->setPosition(ccp(
                    pointsLabel->getPositionX() + (pointsLabel->getScaledContentSize().width / 2) + (pointIcon->getScaledContentSize().width / 2) + 2.f,
                    canvas->getContentSize().height / 2
                ));

                canvas->addChild(pointsLabel);
                canvas->addChild(pointIcon);

                auto statItem = CCMenuItemSpriteExtra::create(canvas, this, menu_selector(MyProfilePage::onStreakStatClick));
                statItem->setID(STREAK_STAT_ID);
                statsMenu->addChild(statItem);

                statsMenu->setContentSize({
                    statsMenu->getContentSize().width + statItem->getScaledContentSize().width + 10.f,
                    statsMenu->getContentSize().height
                    });

                statsMenu->updateLayout();

                // <<< CAMBIO CLAVE: Reducimos la escala de todo el menú de estadísticas >>>
                statsMenu->setScale(0.9f); // Puedes ajustar este valor (ej. 0.85 para más pequeño)

                // <<< CAMBIO OPCIONAL: Ajustamos la posición Y para recentrarlo verticalmente >>>
                // Al escalar, el menú se encoge. Este ajuste lo sube un poco para compensar.
                statsMenu->setPositionY(statsMenu->getPositionY() + 4.f);
            }

            // --- LÓGICA PARA LA INSIGNIA EQUIPADA ---
            if (auto username_menu = m_mainLayer->getChildByIDRecursive("username-menu")) {

                if (auto oldBadge = username_menu->getChildByID(STREAK_BADGE_ID)) {
                    oldBadge->removeFromParent();
                }

                g_streakData.load();

                auto equippedBadge = g_streakData.getEquippedBadge();
                if (equippedBadge) {
                    auto badgeSprite = CCSprite::create(equippedBadge->spriteName.c_str());
                    if (badgeSprite) {
                        badgeSprite->setScale(0.2f);

                        auto badgeButton = CCMenuItemSpriteExtra::create(
                            badgeSprite,
                            this,
                            menu_selector(MyProfilePage::onBadgeInfoClick)
                        );
                        badgeButton->setUserObject(CCString::create(equippedBadge->badgeID));
                        badgeButton->setID(STREAK_BADGE_ID);

                        username_menu->addChild(badgeButton);
                        username_menu->updateLayout();
                    }
                }
            }
        }
    }
};