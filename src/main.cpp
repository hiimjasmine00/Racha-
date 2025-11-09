#include "StreakData.h"
#include "Popups.h"
#include "FirebaseManager.h"

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/GJComment.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <matjson.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/OptionsLayer.hpp>
#include <Geode/ui/Notification.hpp> 


class $modify(MyPlayLayer, PlayLayer) {
    void levelComplete() {
     
        int percentBefore = this->m_level->m_normalPercent;

        
        PlayLayer::levelComplete();     
        int percentAfter = this->m_level->m_normalPercent;

        
        if (this->m_isPracticeMode) return;

        
        if (percentBefore >= 100) return;

        
        if (percentAfter < 100) {
            log::warn("⚠️ Anti-Cheat: levelComplete se ejecutó pero el porcentaje final es solo {}%", percentAfter);
            return;
        }

        

        int stars = this->m_level->m_stars;
        // Solo niveles con estrellas (rated) dan puntos
        if (stars > 0) {
            int points = 0;
            if (stars <= 3) points = 1;       // Auto/Easy/Normal
            else if (stars <= 5) points = 3;  // Hard
            else if (stars <= 7) points = 4;  // Harder
            else if (stars <= 9) points = 5;  // Insane
            else points = 6;                  // Demon

            if (points > 0) {
                int before = g_streakData.streakPointsToday;
                int required = g_streakData.getRequiredPoints();

                log::info("✅ Nivel completado legalmente ({} stars -> {} points)", stars, points);
                g_streakData.addPoints(points);

                // Mostrar barra de progreso
                if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
                    scene->addChild(StreakProgressBar::create(points, before, required), 100);
                }
            }
        }
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    struct Fields {
        EventListener<web::WebTask> m_playerDataListener;
        bool m_isReconnecting = false;
    };

    enum class ButtonState { Loading, Active, Error };

    bool init() {
        if (!MenuLayer::init()) return false;

        // ESTADO INICIAL: Cargando
        this->createStreakButton(ButtonState::Loading);
        this->loadPlayerData();

        return true;
    }

    void tryReconnect(float dt) {
        if (g_streakData.isDataLoaded) {
            this->unschedule(schedule_selector(MyMenuLayer::tryReconnect));
            m_fields->m_isReconnecting = false;
            this->createStreakButton(ButtonState::Active);
            return;
        }

        // Si sigue sin cargar, volvemos a intentar y mostramos estado de carga
        this->createStreakButton(ButtonState::Loading);
        this->loadPlayerData();
    }

    void loadPlayerData() {
        auto accountManager = GJAccountManager::sharedState();
        if (!accountManager || accountManager->m_accountID == 0) {
            g_streakData.m_initialized = true;
            g_streakData.isDataLoaded = true;
            g_streakData.dailyUpdate();
            this->createStreakButton(ButtonState::Active);
            return;
        }

        m_fields->m_playerDataListener.bind([this](web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                if (res->ok() && res->json().isOk()) {
                    g_streakData.parseServerResponse(res->json().unwrap());
                    if (g_streakData.isBanned) {
                        if (m_fields->m_isReconnecting) {
                            this->unschedule(schedule_selector(MyMenuLayer::tryReconnect));
                            m_fields->m_isReconnecting = false;
                        }
                        this->createStreakButton(ButtonState::Error);
                        return;
                    }
                    this->onLoadSuccess();
                }
                else if (res->code() == 404) {
                    g_streakData.needsRegistration = true;
                    this->onLoadSuccess();
                }
                else {
                    this->onLoadFailed();
                }
            }
            else if (e->isCancelled()) {
                this->onLoadFailed();
            }
            });

        std::string url = fmt::format("https://streak-servidor.onrender.com/players/{}", accountManager->m_accountID);
        auto req = web::WebRequest();
        m_fields->m_playerDataListener.setFilter(req.get(url));
    }

    void onLoadSuccess() {
        if (m_fields->m_isReconnecting) {
            this->unschedule(schedule_selector(MyMenuLayer::tryReconnect));
            m_fields->m_isReconnecting = false;
        }
        g_streakData.isDataLoaded = true;
        g_streakData.m_initialized = true;
        g_streakData.dailyUpdate();
        this->createStreakButton(ButtonState::Active);
    }

    void onLoadFailed() {
        // MARCAR COMO NO CARGADO ES CRÍTICO
        g_streakData.isDataLoaded = false;
        g_streakData.m_initialized = false;

        // MOSTRAR ERROR VISUALMENTE
        this->createStreakButton(ButtonState::Error);

        if (!m_fields->m_isReconnecting) {
            m_fields->m_isReconnecting = true;
            this->schedule(schedule_selector(MyMenuLayer::tryReconnect), 5.0f);
        }
    }

    void createStreakButton(ButtonState state) {
        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return;
        if (auto oldBtn = menu->getChildByID("streak-button"_spr)) oldBtn->removeFromParentAndCleanup(true);

        CCSprite* icon = nullptr;
        CircleBaseColor color = CircleBaseColor::Gray;

        switch (state) {
        case ButtonState::Loading:
            icon = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
            break;
        case ButtonState::Error:
            icon = CCSprite::createWithSpriteFrameName("exMark_001.png");
            break;
        case ButtonState::Active:
            std::string spriteName = g_streakData.getRachaSprite();
            if (!spriteName.empty()) icon = CCSprite::create(spriteName.c_str());
            if (!icon) icon = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
            color = CircleBaseColor::Green;
            break;
        }

        if (!icon) return;
        icon->setScale(0.5f);
        auto circle = CircleButtonSprite::create(icon, color, CircleBaseSize::Medium);

        if (state == ButtonState::Loading) {
            icon->runAction(CCRepeatForever::create(CCRotateBy::create(1.0f, 360.f)));
        }

        if (state == ButtonState::Active) {
            int requiredPoints = g_streakData.getRequiredPoints();
            if (requiredPoints > 0 && g_streakData.streakPointsToday < requiredPoints) {
                auto alertSprite = CCSprite::createWithSpriteFrameName("exMark_001.png");
                if (alertSprite) {
                    alertSprite->setScale(0.4f);
                    alertSprite->setPosition({ circle->getContentSize().width - 12, circle->getContentSize().height - 12 });
                    alertSprite->setZOrder(10);
                    alertSprite->runAction(CCRepeatForever::create(CCBlink::create(2.0f, 3)));
                    circle->addChild(alertSprite);
                }
            }
        }

        SEL_MenuHandler callback = nullptr;
        if (state == ButtonState::Active) callback = menu_selector(MyMenuLayer::onOpenPopup);
        else if (state == ButtonState::Error) callback = menu_selector(MyMenuLayer::onErrorButtonClick);

        auto btn = CCMenuItemSpriteExtra::create(circle, this, callback);
        btn->setID("streak-button"_spr);

        if (state == ButtonState::Loading) {
            btn->setEnabled(false);
            btn->setOpacity(150);
        }

        menu->addChild(btn);
        menu->updateLayout();
    }

    void onErrorButtonClick(CCObject*) {
        if (g_streakData.isBanned) {
            createQuickPopup("ACCOUNT BANNED", "You have been <cr>BANNED</c> from Streak Mod.\nReason: <cy>" + g_streakData.banReason + "</c>", "OK", "Discord", [](FLAlertLayer*, bool btn2) {
                if (btn2) cocos2d::CCApplication::sharedApplication()->openURL("https://discord.gg/vEPWBuFEn5");
                });
            return;
        }
        FLAlertLayer::create("Connection Failed", "<cr>Internet connection required.</c>\nRetrying in background...", "OK")->show();
    }

    void onOpenPopup(CCObject * sender) {
        if (!g_streakData.isDataLoaded && !g_streakData.needsRegistration) {
            this->onErrorButtonClick(nullptr);
            return;
        }
        if (g_streakData.isBanned) {
            this->onErrorButtonClick(nullptr);
            return;
        }
        InfoPopup::create()->show();
    }
};


class $modify(MyCommentCell, CommentCell) {
    struct Fields {
        CCMenuItemSpriteExtra* badgeButton = nullptr;
        EventListener<web::WebTask> m_badgeListener;
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

    void loadFromComment(GJComment * p0) {
        CommentCell::loadFromComment(p0);
        if (p0->m_accountID == GJAccountManager::get()->get()->m_accountID) {
            if (auto username_menu = m_mainLayer->getChildByIDRecursive("username-menu")) {
                auto equippedBadge = g_streakData.getEquippedBadge();
                if (equippedBadge) {
                    auto badgeSprite = CCSprite::create(equippedBadge->spriteName.c_str());
                    if (badgeSprite) {
                        badgeSprite->setScale(0.15f);
                        auto badgeButton = CCMenuItemSpriteExtra::create(
                            badgeSprite, this, menu_selector(MyCommentCell::onBadgeInfoClick)
                        );
                        badgeButton->setUserObject(CCString::create(equippedBadge->badgeID));
                        badgeButton->setID("streak-badge"_spr);
                        username_menu->addChild(badgeButton);
                        username_menu->updateLayout();
                    }
                }
            }
        }
        else {
            std::string url = fmt::format("https://streak-servidor.onrender.com/players/{}", p0->m_accountID);
            m_fields->m_badgeListener.bind([this, p0](web::WebTask::Event* e) {
                if (web::WebResponse* res = e->getValue()) {
                    if (res->ok() && res->json().isOk()) {
                        try {
                            auto playerData = res->json().unwrap();
                            std::string badgeId = playerData["equipped_badge_id"].as<std::string>().unwrap();
                            if (!badgeId.empty()) {
                                auto badgeInfo = g_streakData.getBadgeInfo(badgeId);
                                if (badgeInfo) {
                                    if (auto username_menu = m_mainLayer->getChildByIDRecursive("username-menu")) {
                                        auto badgeSprite = CCSprite::create(badgeInfo->spriteName.c_str());
                                        badgeSprite->setScale(0.15f);
                                        auto badgeButton = CCMenuItemSpriteExtra::create(
                                            badgeSprite, this, menu_selector(MyCommentCell::onBadgeInfoClick)
                                        );
                                        badgeButton->setUserObject(CCString::create(badgeInfo->badgeID));
                                        username_menu->addChild(badgeButton);
                                        username_menu->updateLayout();
                                    }
                                }
                            }
                        }
                        catch (const std::exception& ex) {
                            log::debug("Player {} (commenter) has no badge: {}", p0->m_accountID, ex.what());
                        }
                    }
                }
                });
            auto req = web::WebRequest();
            m_fields->m_badgeListener.setFilter(req.get(url));
        }
    }
};

class $modify(MyPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (geode::Mod::get()->getSettingValue<bool>("show-in-pause")) {
            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
            double posX = Mod::get()->getSettingValue<double>("pause-pos-x");
            double posY = Mod::get()->getSettingValue<double>("pause-pos-y");
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
            streakNode->setPosition({ winSize.width * static_cast<float>(posX), winSize.height * static_cast<float>(posY) });
            this->addChild(streakNode);
        }
    }
};