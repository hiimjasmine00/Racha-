#include "StreakData.h"
#include "Popups.h"

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/GJComment.hpp>




class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
        // Dejamos que la función original se ejecute primero
        PlayLayer::levelComplete();

        // <<< ========= INICIO DEL BLOQUE MODIFICADO ========= >>>

        // Obtenemos las estrellas del nivel
        int stars = this->m_level->m_stars;
        int pointsGained = 0;

        // <<< CAMBIO CLAVE: Añadimos la comprobación '!this->m_isPracticeMode' >>>
        // Ahora, el código solo se ejecutará si el nivel tiene rate Y NO estamos en modo práctica.
        if (stars > 0 && !this->m_isPracticeMode) {

            // Calculamos los puntos ganados según la dificultad (estrellas)
            if (stars >= 1 && stars <= 3) {
                // Auto, Easy, Normal
                pointsGained = 1;
            }
            else if (stars >= 4 && stars <= 5) {
                // Hard
                pointsGained = 3;
            }
            else if (stars >= 6 && stars <= 7) {
                // Harder
                pointsGained = 4;
            }
            else if (stars >= 8 && stars <= 9) {
                // Insane
                pointsGained = 5;
            }
            else if (stars == 10) {
                // Demon
                pointsGained = 6;
            }

            // Si ganamos puntos, ejecutamos la lógica de guardado y animación
            if (pointsGained > 0) {
                g_streakData.load();
                int pointsBefore = g_streakData.streakPointsToday;

                g_streakData.addPoints(pointsGained);

                int pointsRequired = g_streakData.getRequiredPoints();

                auto progressBar = StreakProgressBar::create(pointsGained, pointsBefore, pointsRequired);
                if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
                    scene->addChild(progressBar, 100);
                }
            }
        }
        // <<< ========= FIN DEL BLOQUE MODIFICADO ========= >>>
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    struct Fields {
        CCSprite* m_alertSprite = nullptr;
    };

    bool init() {
        if (!MenuLayer::init()) return false;

        g_streakData.load();
        g_streakData.dailyUpdate();

        int totalStars = GameStatsManager::sharedState()->getStat("6");
        if (totalStars == 0 && g_streakData.currentStreak > 0) {
            g_streakData.currentStreak = 0;
            g_streakData.streakPointsToday = 0;
            g_streakData.totalStreakPoints = 0;
            g_streakData.save();
            FLAlertLayer::create("Streak Reset", "Your stats were reset, so your streak has been reset too.", "OK")->show();
        }

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return false;

        auto spriteName = g_streakData.getRachaSprite();
        auto icon = CCSprite::create(spriteName.c_str());
        icon->setScale(0.5f);

        auto circle = CircleButtonSprite::create(icon, CircleBaseColor::Green, CircleBaseSize::Medium);

        int requiredPoints = g_streakData.getRequiredPoints();
        bool streakInactive = (g_streakData.streakPointsToday < requiredPoints);

        m_fields->m_alertSprite = CCSprite::createWithSpriteFrameName("exMark_001.png");
        m_fields->m_alertSprite->setScale(0.4f);
        m_fields->m_alertSprite->setPosition({ circle->getContentSize().width - 12, circle->getContentSize().height - 12 });
        m_fields->m_alertSprite->setZOrder(10);
        m_fields->m_alertSprite->setVisible(streakInactive);
        circle->addChild(m_fields->m_alertSprite);

        if (streakInactive) {
            auto blink = CCBlink::create(2.0f, 3);
            auto repeat = CCRepeatForever::create(blink);
            m_fields->m_alertSprite->runAction(repeat);
        }

        auto btn = CCMenuItemSpriteExtra::create(circle, this, menu_selector(MyMenuLayer::onOpenPopup));
        btn->setPositionY(btn->getPositionY() + 5);
        menu->addChild(btn);
        menu->updateLayout();

        return true;
    }

    void onOpenPopup(CCObject*) {
        InfoPopup::create()->show();
    }
};

class $modify(MyProfilePage, ProfilePage) {

    void onStreakStatClick(CCObject * sender) {
        g_streakData.load();
        std::string alertContent = fmt::format(
            "You have a streak of\n<cy>{}</c> days!",
            g_streakData.currentStreak
        );
        FLAlertLayer::create("Daily Streak", alertContent, "OK")->show();
    }

    void loadPageFromUserInfo(GJUserScore * score) {
        ProfilePage::loadPageFromUserInfo(score);

        if (score->m_accountID == GJAccountManager::get()->get()->m_accountID) {

            if (auto statsMenu = m_mainLayer->getChildByIDRecursive("stats-menu")) {

                g_streakData.load();

                auto pointsLabel = CCLabelBMFont::create(std::to_string(g_streakData.totalStreakPoints).c_str(), "bigFont.fnt");
                pointsLabel->setScale(0.6f);

                auto pointIcon = CCSprite::create("gold_streak_.png"_spr);
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

                auto statItem = CCMenuItemSpriteExtra::create(
                    canvas,
                    this,
                    menu_selector(MyProfilePage::onStreakStatClick)
                );

                statsMenu->addChild(statItem);

                statsMenu->setContentSize({
                    statsMenu->getContentSize().width + statItem->getScaledContentSize().width + 10.f,
                    statsMenu->getContentSize().height
                    });

                statsMenu->updateLayout();
            }
        }
    }
};

class $modify(MyCommentCell, CommentCell) {
    // Las struct Fields y la función onBadgeInfoClick no cambian
    struct Fields {
        CCMenuItemSpriteExtra* badgeButton = nullptr;
    };

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

    // <<< FUNCIÓN CORREGIDA >>>
    void loadFromComment(GJComment * p0) {
        CommentCell::loadFromComment(p0);

        if (p0->m_accountID == GJAccountManager::get()->get()->m_accountID) {
            if (auto username_menu = m_mainLayer->getChildByIDRecursive("username-menu")) {

                g_streakData.load();
                auto equippedBadge = g_streakData.getEquippedBadge();

                if (equippedBadge) {
                    // --- LÓGICA DE PRIORIDAD CORREGIDA ---

                    // 1. Creamos una lista para las insignias de otros mods
                    cocos2d::CCArray* otherBadges = cocos2d::CCArray::create();

                    // 2. Recorremos los hijos del menú, PERO empezamos desde el segundo elemento (índice 1)
                    //    Esto deja el primer elemento (el nombre de usuario) intacto.
                    for (size_t i = 1; i < username_menu->getChildrenCount(); ++i) {
                        if (auto item = dynamic_cast<CCMenuItem*>(username_menu->getChildren()->objectAtIndex(i))) {
                            otherBadges->addObject(item);
                        }
                    }

                    // 3. Eliminamos solo las insignias de otros mods que encontramos
                    for (auto* item : CCArrayExt<CCNode*>(otherBadges)) {
                        item->removeFromParent();
                    }

                    // --- FIN DE LA LÓGICA CORREGIDA ---

                    // 4. Ahora, añadimos NUESTRA insignia
                    auto badgeSprite = CCSprite::create(equippedBadge->spriteName.c_str());
                    if (badgeSprite) {
                        badgeSprite->setScale(0.15f);

                        auto badgeButton = CCMenuItemSpriteExtra::create(
                            badgeSprite,
                            this,
                            menu_selector(MyCommentCell::onBadgeInfoClick)
                        );
                        badgeButton->setUserObject(CCString::create(equippedBadge->badgeID));
                        badgeButton->setID("streak-badge"_spr);

                        username_menu->addChild(badgeButton);
                        username_menu->updateLayout();
                    }
                }
            }
        }
    }
};

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (geode::Mod::get()->getSettingValue<bool>("show-in-pause")) {
            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
            float posX = Mod::get()->getSettingValue<double>("pause-pos-x");
            float posY = Mod::get()->getSettingValue<double>("pause-pos-y");

            g_streakData.load();
            int pointsToday = g_streakData.streakPointsToday;
            int requiredPoints = g_streakData.getRequiredPoints();
            int streakDays = g_streakData.currentStreak;

            auto streakNode = CCNode::create();

            auto streakIcon = CCSprite::create(g_streakData.getRachaSprite().c_str());
            streakIcon->setScale(0.2f);
            streakNode->addChild(streakIcon);

            auto daysLabel = CCLabelBMFont::create(
                CCString::createWithFormat("Day %d", streakDays)->getCString(), "goldFont.fnt"
            );
            daysLabel->setScale(0.35f);
            daysLabel->setPosition({ 0, -22 });
            streakNode->addChild(daysLabel);

            auto pointCounterNode = CCNode::create();
            pointCounterNode->setPosition({ 0, -37 });
            streakNode->addChild(pointCounterNode);

            auto pointLabel = CCLabelBMFont::create(
                CCString::createWithFormat("%d / %d", pointsToday, requiredPoints)->getCString(), "bigFont.fnt"
            );
            pointLabel->setScale(0.35f);
            pointCounterNode->addChild(pointLabel);

            auto pointIcon = CCSprite::create("streak_point.png"_spr);
            pointIcon->setScale(0.18f);
            pointCounterNode->addChild(pointIcon);

            pointCounterNode->setContentSize({ pointLabel->getScaledContentSize().width + pointIcon->getScaledContentSize().width + 5, pointLabel->getScaledContentSize().height });
            pointLabel->setPosition({ -pointIcon->getScaledContentSize().width / 2, 0 });
            pointIcon->setPosition({ pointLabel->getScaledContentSize().width / 2 + 5, 0 });

            streakNode->setPosition({ winSize.width * posX, winSize.height * posY });
            this->addChild(streakNode);
        }
    }
};