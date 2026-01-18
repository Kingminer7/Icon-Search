#include "IconSearchPopup.hpp"

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

bool IconSearchPopup::setup(GJGarageLayer* garage) {
    m_garage = garage;
    if (m_candidates.empty()) getCandidates();
    m_input = TextInput::create(280, "Search Icons...");
    m_input->setCallback([this](auto str) {
        m_search.query = str;
    });
    m_mainLayer->addChildAtPosition(m_input, Anchor::Top,  CCPoint{0, -30});

    auto searchSpr = makeSprite("search.png"_spr, 1.f, "geode.loader/GE_button_05.png");
    searchSpr->setScale(.75f);
    auto searchBtn = CCMenuItemExt::createSpriteExtra(searchSpr, [this](auto) {
	    updateNodes();
    });
    m_buttonMenu->addChildAtPosition(searchBtn, Anchor::Top,  CCPoint{m_input->getScaledContentWidth() / 2 + searchBtn->getScaledContentWidth() / 2 + 5, -30});

    auto filterSpr = makeSprite("GJ_filterIcon_001.png", 1.f, "geode.loader/GE_button_05.png");
    filterSpr->setScale(.75f);
    auto filterBtn = CCMenuItemExt::createSpriteExtra(filterSpr, [](auto) {

    });
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Top,  CCPoint{-m_input->getScaledContentWidth() / 2 - filterBtn->getScaledContentWidth() / 2 - 5, -30});

    m_menu = CCMenu::create();
    m_menu->setContentSize({360, 210});
    m_menu->ignoreAnchorPointForPosition(false); // why does this exist, it sucks :P
    m_mainLayer->addChildAtPosition(m_menu, Anchor::Center, CCPoint{0, -20});
    m_menu->setLayout(RowLayout::create()->setGrowCrossAxis(true)->setCrossAxisAlignment(AxisAlignment::End)->setCrossAxisOverflow(false)->setGap(2));

    auto scrollBg = CCLayerColor::create(ccColor4B{0,0,0,50});
    scrollBg->ignoreAnchorPointForPosition(false);
    scrollBg->setContentSize(m_menu->getContentSize());
    m_mainLayer->addChildAtPosition(scrollBg, Anchor::Center, CCPoint{0, -20});

    return true;
}

void IconSearchPopup::keyDown(enumKeyCodes key) {
    if (key == KEY_Enter) {
        updateNodes();
        m_input->getInputNode()->onClickTrackNode(false);
        return;
    }
    Popup::keyDown(key);
}

void IconSearchPopup::updateNodes() {
    updateResults();
    m_menu->removeAllChildren();
    if (m_search.page * m_search.pageSize > m_results.size()) m_search.page = m_results.size() / m_search.pageSize;
    for (int i = m_search.page * m_search.pageSize; i < (m_search.page + 1) * m_search.pageSize; i++) {
        if (i >= m_results.size()) break;
        auto c = m_results[i].first;
        CCSprite* sprite;
        switch (c.type) {
            case IconType::Special: {
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("player_special_{:02}_001.png", c.id).c_str());
                break;
            }
            case IconType::DeathEffect: {
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("explosionIcon_{:02}_001.png", c.id).c_str());
                break;
            }
            case IconType::Item: {
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("gjItem_{:02}_001.png", c.id).c_str());
                break;
            }
            case IconType::ShipFire: {
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("shipfireIcon_{:02}_001.png", c.id).c_str());
                break;
            }
            default: {
                auto is = SimplePlayer::create(0);
                is->setScale(.8);
                is->updatePlayerFrame(c.id, c.type);
                sprite = is;
            }
        }
        auto btn = CCMenuItemSpriteExtra::create(sprite, m_garage, menu_selector(GJGarageLayer::onSelect));
        btn->m_iconType = c.type;
        btn->setTag(c.id);
        sprite->setPosition({15, 15});
        btn->setContentSize({30, 30});
        m_menu->addChild(btn);
    }
    m_menu->updateLayout();
}

inline bool isUnlockedByDefault(const int id, const IconType type) {
    if (type == IconType::Cube) {
        return id < 5;
    }
    return id < 2;
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

int getResultMatch(const SearchCandidate& candidate, std::string_view search) {
    if (search.empty()) return -1;
    if (auto n = numFromString<int>(search)) {
        const int id = n.unwrap();
        if (id == candidate.id) return 0; // exact num
        if (numToString(id).starts_with(search)) return 1; // prefix num
    }

    if (candidate.desc == search) // exact desc
        return 2;

    if (candidate.desc.starts_with(search)) // prefix desc
        return 3;

    if (candidate.game == search) // exact game
        return 4;

    if (candidate.game.starts_with(search)) // prefix game
        return 5;

    // fuzzy desc
    // modified code from eclipse that spaghett added (thanks prevter for sending)
    const auto it = std::ranges::search(
        candidate.desc, search,
        [](const char a, const char b) {
            return a == std::tolower(b);
        }
    ).begin();
    if (it != candidate.desc.end())
        return 10 + it - candidate.desc.begin();

    return -1;
}

void IconSearchPopup::updateResults() {
    m_results.clear();
    const auto& lower = string::toLower(m_search.query);
    for (const auto& candidate : m_candidates) {
        if (m_search.unlocked != std::nullopt && candidate.unlocked != m_search.unlocked) continue;
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

        m_results.insert(it, { candidate, score });
    }
}

void IconSearchPopup::getCandidates() {
    m_candidates.clear();
    auto gm = GameManager::sharedState();
    auto gsm = GameStatsManager::sharedState();
    auto am = AchievementManager::sharedState();
    for (IconType type : {IconType::Cube, IconType::Ship, IconType::Ball, IconType::Ufo, IconType::Wave, IconType::Robot, IconType::Spider, IconType::Swing, IconType::Jetpack, IconType::DeathEffect, IconType::Special, IconType::ShipFire}) {
        auto count = gm->countForType(type);
        for (int id = 1; id <= count; id++) {
            auto ut = gm->iconTypeToUnlockType(type);
            auto res = SearchCandidate{
                .type = type,
                .id = id,
                .unlocked = gm->isIconUnlocked(id, type),
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
                auto limit = am->limitForAchievement(achieve);
                if (limit == 1) res.game = "full";
                if (limit == 2) res.game = "world";
                if (limit == 3) res.game = "subzero";
                if (limit == 4) res.game = "meltdown";
            }

            m_candidates.push_back(std::move(res));
        }
    }
    log::debug("Loaded {} search candidates.", m_candidates.size());
}

IconSearchPopup* IconSearchPopup::create(GJGarageLayer* garage) {
    auto ret = new IconSearchPopup();
    if (ret->initAnchored(400, 280, garage, "geode.loader/GE_square02.png")) return ret->autorelease(), ret;
    return delete ret, ret;
}

std::vector<SearchCandidate> IconSearchPopup::m_candidates = {};
