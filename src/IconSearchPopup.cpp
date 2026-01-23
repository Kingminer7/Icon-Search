#include "IconSearchPopup.hpp"

#include <algorithm>
#include <utility>
#include <cue/PlayerIcon.hpp>
#include <cue/DropdownNode.hpp>

template <geode::utils::string::ConstexprString S, typename T>
T const& getSettingFast() {
    static T value = (
        geode::listenForSettingChanges<T>(S.data(), [](T val) {
            value = std::move(val);
        }),
        geode::getMod()->getSettingValue<T>(S.data())
    );
    return value;
}

using namespace geode::prelude;

CCSprite* makeSprite(const std::string& topName, const float topScale, const std::string& bottomName) {
    const auto base = CCSprite::create(bottomName.c_str());
    if (!base) {
        return nullptr;
    }
    const auto top = CCSprite::createWithSpriteFrameName(topName.c_str());
    if (!top) {
        return nullptr;
    }
    top->setScale(topScale);
    base->addChildAtPosition(top, Anchor::Center);
    return base;
}

bool SearchResult::isUnlocked() const {
    const auto gsm = GameStatsManager::get();
    const auto gm = GameManager::get();
    if (moreIconsInfo != std::nullopt) return true;
    if (type == IconType::Item) return gsm->isItemUnlocked(gm->iconTypeToUnlockType(type), id);
    return gm->isIconUnlocked(id, type);
}

#define replaceCloseBtn(btn) { \
    btn->m_pNormalImage->removeFromParent(); \
    auto newClose = CircleButtonSprite::createWithSpriteFrameName("geode.loader/close.png", .85f, CircleBaseColor::DarkPurple, CircleBaseSize::Medium); \
    newClose->setScale(.8f); \
    btn->m_pNormalImage = newClose; \
    btn->addChildAtPosition(newClose, Anchor::Center, {0, 0}, false); \
}

bool IconSearchPopup::setup(GJGarageLayer* garage) {
    setID("IconSearchPopup");
    m_bgSprite->setZOrder(-2);
    m_bgSprite->setID("background");
    m_closeBtn->setID("close-btn");
    replaceCloseBtn(m_closeBtn);
    m_buttonMenu->setID("button-menu");

    m_garage = garage;
    getCandidates();
    m_input = TextInput::create(280, "Search Icons...");
    m_input->setID("search-input");
    m_input->setCommonFilter(CommonFilter::Any);
    m_input->setCallback([this](auto str) {
        m_search.query = std::move(str);
    });
    m_mainLayer->addChildAtPosition(m_input, Anchor::Top, CCPoint{0, -30});

    auto searchSpr = makeSprite("search.png"_spr, 1.f, "geode.loader/GE_button_05.png");
    searchSpr->setScale(.75f);
    auto searchBtn = CCMenuItemExt::createSpriteExtra(searchSpr, [this](auto) {
        updateNodes();
    });
    searchBtn->setID("search-btn");
    m_buttonMenu->addChildAtPosition(searchBtn, Anchor::Top, CCPoint{m_input->getScaledContentWidth() / 2 + searchBtn->getScaledContentWidth() / 2 + 5, -30});

    auto filterSpr = makeSprite("GJ_filterIcon_001.png", 1.f, "geode.loader/GE_button_05.png");
    filterSpr->setScale(.75f);
    auto filterBtn = CCMenuItemExt::createSpriteExtra(filterSpr, [this](auto) {
        IconSearchFilterPopup::create(this)->show();
    });
    filterBtn->setID("filter-btn");
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Top, CCPoint{-m_input->getScaledContentWidth() / 2 - filterBtn->getScaledContentWidth() / 2 - 5, -30});

    m_menu = CCMenu::create();
    m_menu->setID("search-menu");
    m_menu->setContentSize({355, 205});
    m_menu->ignoreAnchorPointForPosition(false); // why does this exist, it sucks :P
    m_mainLayer->addChildAtPosition(m_menu, Anchor::Center, CCPoint{0, -20});
    m_menu->setLayout(RowLayout::create()
        ->setGrowCrossAxis(true)
        ->setCrossAxisAlignment(AxisAlignment::End)
        ->setCrossAxisOverflow(false)
        ->setGap(2));

    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setID("search-bg");
    bg->setZOrder(-1);
    bg->setOpacity(72);
    bg->setContentSize(m_menu->getContentSize() + CCSize{5, 5});
    m_mainLayer->addChildAtPosition(bg, Anchor::Center, CCPoint{0, -20});

    m_prev = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_arrow_02_001.png", 1.f, [this](auto) {
        m_search.page--;
        updateNodes();
    });
    m_prev->setID("prev-btn");
    m_prev->setVisible(false);
    m_buttonMenu->addChildAtPosition(m_prev, Anchor::Center, {-220, -15});

    m_next = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_arrow_02_001.png", 1.f, [this](auto) {
        m_search.page++;
        updateNodes();
    });
    m_next->setID("next-btn");
    m_next->setVisible(false);
    static_cast<CCSprite*>(m_next->getNormalImage())->setFlipX(true);
    m_buttonMenu->addChildAtPosition(m_next, Anchor::Center, {220, -15});

    updateNodes();

    return true;
}

void IconSearchPopup::keyDown(enumKeyCodes key, double ts) {
    if (key == KEY_Enter) {
        updateNodes();
        m_input->getInputNode()->onClickTrackNode(false);
        return;
    }
    Popup::keyDown(key, ts);
}

std::string iconTypeToString(const IconType type) {
    switch (type) {
        case IconType::Cube:
            return "Cube";
        case IconType::Ship:
            return "Ship";
        case IconType::Ball:
            return "Ball";
        case IconType::Ufo:
            return "Ufo";
        case IconType::Wave:
            return "Wave";
        case IconType::Robot:
            return "Robot";
        case IconType::Spider:
            return "Spider";
        case IconType::Swing:
            return "Swing";
        case IconType::Jetpack:
            return "Jetpack";
        case IconType::DeathEffect:
            return "Death Effect";
        case IconType::Special:
            return "Trail";
        case IconType::Item:
            return "Item";
        case IconType::ShipFire:
            return "Ship Fire";
    }
    return "Unknown";
}

inline std::map<IconType, std::pair<int, std::string>> selectedIcons() {
    std::map<IconType, std::pair<int, std::string>> res = {};
    auto gm = GameManager::get();

    if (auto miSel = more_icons::activeIcon(IconType::Cube); !miSel.empty()) {
        res.insert({IconType::Cube, {-1, miSel}});
    } else {
        res.insert({IconType::Cube, {gm->m_playerFrame, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Ship); !miSel.empty()) {
        res.insert({IconType::Ship, {-1, miSel}});
    } else {
        res.insert({IconType::Ship, {gm->m_playerShip, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Ball); !miSel.empty()) {
        res.insert({IconType::Ball, {-1, miSel}});
    } else {
        res.insert({IconType::Ball, {gm->m_playerBall, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Ufo); !miSel.empty()) {
        res.insert({IconType::Ufo, {-1, miSel}});
    } else {
        res.insert({IconType::Ufo, {gm->m_playerBird, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Wave); !miSel.empty()) {
        res.insert({IconType::Wave, {-1, miSel}});
    } else {
        res.insert({IconType::Wave, {gm->m_playerDart, ""}});
    }
    if (auto miSel = more_icons::activeIcon(IconType::Robot); !miSel.empty()) {
        res.insert({IconType::Robot, {-1, miSel}});
    } else {
        res.insert({IconType::Robot, {gm->m_playerRobot, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Spider); !miSel.empty()) {
        res.insert({IconType::Spider, {-1, miSel}});
    } else {
        res.insert({IconType::Spider, {gm->m_playerSpider, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Swing); !miSel.empty()) {
        res.insert({IconType::Swing, {-1, miSel}});
    } else {
        res.insert({IconType::Swing, {gm->m_playerSwing, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Special); !miSel.empty()) {
        res.insert({IconType::Special, {-1, miSel}});
    } else {
        res.insert({IconType::Special, {gm->m_playerStreak, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::DeathEffect); !miSel.empty()) {
        res.insert({IconType::DeathEffect, {-1, miSel}});
    } else {
        res.insert({IconType::DeathEffect, {gm->m_playerDeathEffect, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::ShipFire); !miSel.empty()) {
        res.insert({IconType::ShipFire, {-1, miSel}});
    } else {
        res.insert({IconType::ShipFire, {gm->m_playerShipFire, ""}});
    }

    if (auto miSel = more_icons::activeIcon(IconType::Jetpack); !miSel.empty()) {
        res.insert({IconType::Jetpack, {-1, miSel}});
    } else {
        res.insert({IconType::Jetpack, {gm->m_playerJetpack, ""}});
    }

    return res;
}

void IconSearchPopup::updateNodes() {
    updateResults();
    m_menu->removeAllChildren();
    m_selectIcons.clear();
    if (m_search.page < 0) m_search.page = 0;
    if (m_search.page * m_search.pageSize > m_results.size()) m_search.page = m_results.size() / m_search.pageSize;
    const auto& sel = selectedIcons();
    for (int i = m_search.page * m_search.pageSize; i < (m_search.page + 1) * m_search.pageSize; i++) {
        if (i >= m_results.size()) break;
        auto c = m_results[i].first;
        CCSprite* sprite;
        switch (c.type) {
            case IconType::Special: {
                if (c.moreIconsInfo == std::nullopt) {
                    sprite = CCSprite::createWithSpriteFrameName(fmt::format("player_special_{:02}_001.png", c.id).c_str());
                } else {
                    sprite = CCSprite::create(c.moreIconsInfo->textures[0].c_str());
                }
                sprite->setScale(.9f);
                break;
            }
            case IconType::DeathEffect: {
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("explosionIcon_{:02}_001.png", c.id).c_str());
                sprite->setScale(.9f);
                break;
            }
            case IconType::Item: {
                if (c.id > 5 && c.id < 16) {
                    sprite = GJPathSprite::create(c.id - 5);
                    sprite->setScale(30 / std::max(sprite->getContentHeight(), sprite->getContentWidth()));
                    break;
                }
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("gjItem_{:02}_001.png", c.id).c_str());
                sprite->setScale(30 / std::max(sprite->getContentHeight(), sprite->getContentWidth()));
                break;
            }
            case IconType::ShipFire: {
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("shipfireIcon_{:02}_001.png", c.id).c_str());
                break;
            }
            default: {
                auto is = cue::PlayerIcon::create({
                    .type = c.type,
                    .id = c.id,
                    .color1 = 17,
                    .color2 = 12,
                });
                if (c.moreIconsInfo != std::nullopt) {
                    more_icons::updateSimplePlayer(is, c.moreIconsInfo->name, c.type);
                    is->m_firstLayer->setPosition(is->getContentSize() / 2);
                }
                if (c.type == IconType::Ship || c.type == IconType::Robot || c.type == IconType::Spider || c.type == IconType::Jetpack) {
                    is->setScale(.65f);
                } else if (c.type == IconType::Ufo) {
                    is->m_firstLayer->setPosition(CCPoint{is->m_firstLayer->getPositionX(), 7.75});
                    is->setScale(.75);
                } else if (c.type == IconType::Swing) {
                    is->setScale(.75f);
                } else {
                    is->setScale(.85f);
                }
                sprite = is;
            }
        }
        auto btn = CCMenuItemExt::createSpriteExtra(sprite, [this, c](auto btn) {
            if (c.moreIconsInfo != std::nullopt) {
                if (more_icons::activeIcon(c.type) == c.moreIconsInfo->name) more_icons::createInfoPopup(c.name, c.type)->show();
                else more_icons::setIcon(c.moreIconsInfo->name, c.type);
            } else {
                auto gm = GameManager::get();
                if (!c.isUnlocked() && !getSettingFast<"icon-hack", bool>()) return ItemInfoPopup::create(c.id, GameManager::get()->iconTypeToUnlockType(c.type))->show();
                if (more_icons::activeIcon(c.type).empty()) {
                    SeedValueRSV* member = nullptr;
                    switch (c.type) {
                        case IconType::Cube:
                            member = &gm->m_playerFrame;
                            break;
                        case IconType::Ship:
                            member = &gm->m_playerShip;
                            break;
                        case IconType::Ball:
                            member = &gm->m_playerBall;
                            break;
                        case IconType::Ufo:
                            member = &gm->m_playerBird;
                            break;
                        case IconType::Wave:
                            member = &gm->m_playerDart;
                            break;
                        case IconType::Robot:
                            member = &gm->m_playerRobot;
                            break;
                        case IconType::Spider:
                            member = &gm->m_playerSpider;
                            break;
                        case IconType::Swing:
                            member = &gm->m_playerSwing;
                            break;
                        case IconType::Jetpack:
                            member = &gm->m_playerJetpack;
                            break;
                        case IconType::DeathEffect:
                            member = &gm->m_playerDeathEffect;
                            break;
                        case IconType::Special:
                            member = &gm->m_playerStreak;
                            break;
                        case IconType::Item:
                            break;
                        case IconType::ShipFire:
                            member = &gm->m_playerShipFire;
                            break;
                    }
                    if (!member) {
                        auto po = ItemInfoPopup::create(c.id, GameManager::get()->iconTypeToUnlockType(c.type));
                        auto lab = CCLabelBMFont::create("Icon Search cannot handle selecting items, sorry!", "bigFont.fnt");
                        lab->setOpacity(75);
                        lab->setScale(.4f);
                        lab->setID("item-select-warning"_spr);
                        po->m_mainLayer->addChildAtPosition(lab, Anchor::Center, {0, -125}, false);
                        po->show();
                        return;
                    }
                    if (auto spr = m_selectIcons[c.type]) {
                        spr->retain();
                        spr->removeFromParentAndCleanup(false);
                        btn->addChildAtPosition(spr, Anchor::Center, {0, 0}, false);
                        spr->release();
                    } else {
                        spr = CCSprite::createWithSpriteFrameName("GJ_select_001.png");
                        spr->setID(iconTypeToString(c.type) + "-select-sprite");
                        spr->setScale(.9f);
                        btn->addChildAtPosition(spr, Anchor::Center, {0, 0}, false);
                        m_selectIcons[c.type] = spr;
                    }
                    if (*member == c.id) {
                        ItemInfoPopup::create(c.id, GameManager::get()->iconTypeToUnlockType(c.type))->show();
                    } else {
                        *member = c.id;
                        switch (c.type) {
                            case IconType::Cube:
                            case IconType::Ball:
                            case IconType::Ship:
                            case IconType::Ufo:
                            case IconType::Wave:
                            case IconType::Robot:
                            case IconType::Spider:
                            case IconType::Swing:
                            case IconType::Jetpack:
                                m_garage->m_playerObject->updatePlayerFrame(c.id, c.type);
                                gm->m_playerIconType = c.type;
                                m_garage->m_selectedIconType = c.type;
                                break;
                            default: break;
                        }
                    }
                    // this is to sync up the vanilla cursor icon(s)
                    if (const auto tabBtn = static_cast<CCMenuItemToggler*>(m_garage->getChildByID("category-menu")->getChildByTag(static_cast<int>(m_garage->m_iconType)))) {
                        tabBtn->setEnabled(true);
                        tabBtn->activate();
                    }
                }

            }
        });
        btn->m_iconType = c.type;
        btn->setTag(c.id);
        sprite->setPosition({15, 15});
        btn->setContentSize({30, 30});
        btn->setID(fmt::format("{}-{}", iconTypeToString(c.type), c.id));
        if (!c.isUnlocked() && !getSettingFast<"icon-hack", bool>()) {
            auto lock = CCSprite::createWithSpriteFrameName("GJ_lockGray_001.png");
            lock->setScale(.8f);
            lock->setOpacity(200);
            lock->setID("lock-sprite");
            lock->setZOrder(1);
            btn->addChildAtPosition(lock, Anchor::Center, {0, 0}, false);
        }
        m_menu->addChild(btn);
        if (c.type == IconType::Item) continue;
        auto p = sel.at(c.type);
        if ((c.moreIconsInfo != std::nullopt && p.second == c.moreIconsInfo->name) || (c.moreIconsInfo == std::nullopt && c.id == p.first)) {
            auto spr = CCSprite::createWithSpriteFrameName("GJ_select_001.png");
            spr->setID(iconTypeToString(c.type) + "-select-sprite");
            spr->setScale(.9f);
            btn->addChildAtPosition(spr, Anchor::Center, {0, 0}, false);
            m_selectIcons.insert({c.type, spr});
        }
    }
    m_menu->updateLayout();

    m_prev->setVisible(m_search.page > 0);
    m_next->setVisible(m_search.page < m_results.size() / m_search.pageSize);
}

inline bool isUnlockedByDefault(const int id, const IconType type) {
    if (type == IconType::Cube) {
        return id < 5;
    }
    return id < 2;
}

int getResultMatch(const SearchResult& candidate, std::string_view search) {
    if (search.empty()) return 0;
    const auto idStr = numToString(candidate.id);
    if (idStr == search) return 0; // exact num
    if (idStr.starts_with(search)) return 1; // prefix num

    if (candidate.name == search) // exact name
        return 2;

    if (candidate.name.starts_with(search)) // prefix name
        return 3;

    if (candidate.desc == search) // exact desc
        return 29;

    if (candidate.desc.starts_with(search)) // prefix desc
        return 30;

    if (candidate.achievement == search) // exact achievement
        return 36;

    if (candidate.achievement.starts_with(search)) // prefix achievement
        return 37;

    if (candidate.game == search) // exact game
        return 63;

    if (candidate.game.starts_with(search)) // prefix game
        return 64;

    // fuzzy desc
    // modified code from eclipse that spaghett added (thanks prevter for sending)
#define fuzzy(type, bonus) const auto type##It = std::ranges::search( \
    candidate.type, search, \
    [](const char a, const char b) { \
        return a == std::tolower(b); \
    } \
    ).begin(); \
    if (type##It != candidate.type.end()) \
        return bonus + type##It - candidate.type.begin();

    fuzzy(name, 4)
    fuzzy(desc, 31)
    fuzzy(achievement, 38)

#undef fuzzy

    return -1;
}

void IconSearchPopup::updateResults() {
    m_results.clear();
    const auto& lower = string::toLower(m_search.query);
    for (const auto& candidate : m_candidates) {
        if (m_search.unlocked != std::nullopt && candidate.isUnlocked() != m_search.unlocked) continue;
        if (!m_search.types.empty() && !std::ranges::contains(m_search.types, candidate.type)) continue;

        int score = getResultMatch(candidate, lower);
        if (score < 0)
            continue;

        const auto it = std::ranges::upper_bound(
            m_results, score,
            {}, [](auto const& c) {
                return c.second;
            }
        );

        m_results.insert(it, {candidate, score});
    }
}

void IconSearchPopup::getCandidates() {
    m_candidates.clear();
    auto gm = GameManager::sharedState();
    for (IconType type : {
             IconType::Cube, IconType::Ship, IconType::Ball, IconType::Ufo, IconType::Wave, IconType::Robot,
             IconType::Spider, IconType::Swing, IconType::Jetpack, IconType::DeathEffect, IconType::Special,
             IconType::ShipFire
         }) {
        auto count = gm->countForType(type);
        for (int id = 1; id <= count; id++) {
            addCandidate(type, id);
        }
    }
    for (int i = 1; i <= 20; i++) {
        addCandidate(IconType::Item, i);
    }
    log::debug("Loaded {} vanilla search candidates.", m_candidates.size());
    loadMoreIconsCandidates();
}

void IconSearchPopup::loadMoreIconsCandidates() {
    if (!more_icons::get()) return; // not loaded
    int added = 0;
    if (auto icons = more_icons::getIcons()) {
        for (auto [type, iconList] : *icons) {
            for (IconInfo& icon : iconList) {
                addMICandidate(type, icon);
                added++;
            }
        }
    }
    log::debug("Loaded {} more icons candidates", added);
}

void IconSearchPopup::addMICandidate(IconType type, IconInfo& info) {
    auto res = SearchResult{
        .type = type,
        .id = -1,
        .desc = "added by the more icons mod",
        .game = "more icons",
        .achievement = info.packName,
        .name = info.name,
        .moreIconsInfo = info,
    };

    /* unlock states
     * 0 = default OR 2.21
     * 1 = descriptionForUnlock
     * 2 = secret chest
     * 3 = special chest
     * 4 = shop
     * 5 = scratch
     * 6 = community shop
     * 7 = mechanic
     * 8 = diamond shop
     */

    /* achievement limits? (AchievementManager::limitForAchievement)
         * 0 = generic?
         * 1 = full?
         * 2 = world
         * 3 = subzero
         * 4 = meltdown
         */

    m_candidates.push_back(std::move(res));
}

void IconSearchPopup::addCandidate(IconType type, int id) {
    auto gm = GameManager::sharedState();
    auto gsm = GameStatsManager::sharedState();
    auto am = AchievementManager::sharedState();
    auto ut = gm->iconTypeToUnlockType(type);
    auto res = SearchResult{
        .type = type,
        .id = id,
        .name = string::toLower(ItemInfoPopup::nameForUnlockType(id, ut)),
    };

    /* unlock states
     * 0 = default OR 2.21
     * 1 = descriptionForUnlock
     * 2 = secret chest
     * 3 = special chest
     * 4 = shop
     * 5 = scratch
     * 6 = community shop
     * 7 = mechanic
     * 8 = diamond shop
     */

    switch (gsm->getItemUnlockState(id, ut)) {
        case 0:
            if (isUnlockedByDefault(id, type)) res.desc = "unlocked by default";
            else res.desc = "unlocked in 2.21";
            break;
        case 1:
            res.desc = string::toLower(GJGarageLayer::descriptionForUnlock(id, ut));
            break;
        case 2:
            res.desc = "find this in a secret chest";
            break;
        case 3:
            res.desc = "find this in a special chest";
            break;
        case 4:
            res.desc = "buy this in the regular shop";
            break;
        case 5:
            res.desc = "buy this in the secret shop";
            break;
        case 6:
            res.desc = "buy this in the community shop";
            break;
        case 7:
            res.desc = "buy this in the mechanic shop";
            break;
        case 8:
            res.desc = "buy this in the diamond shop";
            break;
        default:
            break;
    }
    /* achievement limits? (AchievementManager::limitForAchievement)
         * 0 = generic?
         * 1 = full?
         * 2 = world
         * 3 = subzero
         * 4 = meltdown
         */

    auto achieve = am->achievementForUnlock(id, ut);
    if (!achieve.empty()) {
        if (auto data = static_cast<CCDictionary*>(am->m_platformAchievements->objectForKey(achieve)))
            res.achievement = string::toLower(data->valueForKey("title")->getCString());
        auto limit = am->limitForAchievement(achieve);
        if (limit == 1) res.game = "full";
        if (limit == 2) res.game = "world";
        if (limit == 3) res.game = "subzero";
        if (limit == 4) res.game = "meltdown";
    }

    m_candidates.push_back(std::move(res));
}

IconSearchPopup* IconSearchPopup::create(GJGarageLayer* garage) {
    auto ret = new IconSearchPopup();
    if (ret->initAnchored(400, 280, garage, "geode.loader/GE_square02.png")) return ret->autorelease(), ret;
    return delete ret, ret;
}

bool IconSearchFilterPopup::setup(IconSearchPopup* parent) {
    setID("IconSearchPopup");
    m_bgSprite->setID("background");
    m_closeBtn->setID("close-btn");
    replaceCloseBtn(m_closeBtn);
    m_buttonMenu->setID("button-menu");
    CCMenu* typeMenu = CCMenu::create();
    typeMenu->setContentSize({200, 30});
    typeMenu->setLayout(RowLayout::create()
                        ->setGrowCrossAxis(true)
                        ->setCrossAxisAlignment(AxisAlignment::End)
                        ->setCrossAxisOverflow(true)
                        ->setGap(2));
    for (auto [type, has, str, sheet] : std::vector<std::tuple<IconType, bool, std::string, bool>>{
             {IconType::Cube, true, "icon", true},
             {IconType::Ship, true, "ship", true},
             {IconType::Ball, true, "ball", true},
             {IconType::Ufo, true, "bird", true},
             {IconType::Wave, true, "dart", true},
             {IconType::Robot, true, "robot", true},
             {IconType::Spider, true, "spider", true},
             {IconType::Swing, true, "swing", true},
             {IconType::Jetpack, true, "jetpack", true},
             {IconType::DeathEffect, true, "explosion", true},
             {IconType::Special, true, "streak", true},
             {IconType::ShipFire, false, "shipfire03_002.png", false},
             {IconType::Item, false, "gjItem_04_001.png", true},
         }) {
        CCMenuItemToggler* btn;
        auto cb = [parent, type](CCMenuItemToggler* btn) {
            if (!btn->isOn()) parent->m_search.types.push_back(type);
            else std::erase(parent->m_search.types, type);
        };
        if (has) btn = CCMenuItemExt::createTogglerWithFrameName(fmt::format("gj_{}Btn_on_001.png", str),
                                                                 fmt::format("gj_{}Btn_off_001.png", str), .8f, cb);
        else {
            CCSprite* gs1;
            CCSprite* gs2;
            if (sheet) {
                gs1 = CCSpriteGrayscale::createWithSpriteFrameName(str);
                gs2 = CCSpriteGrayscale::createWithSpriteFrameName(str);
            }
            else {
                gs1 = CCSpriteGrayscale::create(str);
                gs2 = CCSpriteGrayscale::create(str);
            }
            auto s1 = IconSelectButtonSprite::create(gs1, IconSelectBaseColor::Selected);
            auto s2 = IconSelectButtonSprite::create(gs2, IconSelectBaseColor::Unselected);
            gs1->setScale(gs1->getScale() * 1.25);
            gs2->setScale(gs2->getScale() * 1.25);

            s1->setScale(.8);
            s2->setScale(.8);
            btn = CCMenuItemExt::createToggler(s1, s2, cb);
        }
        if (std::ranges::contains(parent->m_search.types, type)) btn->toggle(true);
        typeMenu->addChild(btn);
    }
    typeMenu->updateLayout();

    auto drop = cue::DropdownNode::create(ccColor4B{0, 0, 0, 60}, 120, 20, 40);
    drop->setAnchorPoint({.5f, 1.f});
    static auto makeLabel = [](const char* text) {
        auto lab = CCLabelBMFont::create(text, "bigFont.fnt");
        lab->setScale(.6f);
        return lab;
    };
    auto all = makeLabel("All");
    auto yes = makeLabel("Unlocked");
    auto no = makeLabel("Locked");
    if (parent->m_search.unlocked == std::nullopt) {
        drop->addCell(all);
        drop->addCell(yes);
        drop->addCell(no);
    }
    else if (parent->m_search.unlocked == true) {
        drop->addCell(yes);
        drop->addCell(all);
        drop->addCell(no);
    }
    else {
        drop->addCell(no);
        drop->addCell(all);
        drop->addCell(yes);
    }

    drop->setCallback([parent, all, yes](auto, CCNode* node) {
        if (node == all) parent->m_search.unlocked = std::nullopt;
        else parent->m_search.unlocked = node == yes;
    });

    m_mainLayer->addChildAtPosition(drop, Anchor::Center, {0, 0});
    m_mainLayer->addChildAtPosition(typeMenu, Anchor::Center, {0, 35});
    m_parent = parent;
    return true;
}

IconSearchFilterPopup* IconSearchFilterPopup::create(IconSearchPopup* parent) {
    auto ret = new IconSearchFilterPopup();
    if (ret->initAnchored(250, 150, parent, "geode.loader/GE_square02.png")) return ret->autorelease(), ret;
    return delete ret, ret;
}

void IconSearchFilterPopup::onClose(CCObject* sender) {
    Popup::onClose(sender);
    m_parent->updateNodes();
}
