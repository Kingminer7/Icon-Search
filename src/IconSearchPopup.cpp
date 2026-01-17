#include "IconSearchPopup.hpp"

#include "MenuScrollLayer.hpp"

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
    auto input = TextInput::create(280, "Search Icons...");
    input->setCallback([this](auto str) {
        m_search.query = str;
        updateResults();
        updateNodes();
    });
    m_mainLayer->addChildAtPosition(input, Anchor::Top,  CCPoint{0, -30});

    auto filterSpr = makeSprite("GJ_filterIcon_001.png", 1.f, "geode.loader/GE_button_05.png");
    filterSpr->setScale(.75f);
    auto filterBtn = CCMenuItemExt::createSpriteExtra(filterSpr, [](auto) {

    });
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Top,  CCPoint{-input->getScaledContentWidth() / 2 - filterBtn->getScaledContentWidth() / 2 - 5, -30});

    m_scroll = MenuScrollLayer::create({360, 210});
    m_scroll->ignoreAnchorPointForPosition(false); // why does this exist, it sucks :P
    m_mainLayer->addChildAtPosition(m_scroll, Anchor::Center, CCPoint{0, -20});
    m_scroll->m_contentLayer->setLayout(RowLayout::create()->setGrowCrossAxis(true)->setCrossAxisOverflow(true)->setCrossAxisAlignment(AxisAlignment::Start)->setGap(2));

    auto scrollBg = CCLayerColor::create(ccColor4B{0,0,0,50});
    scrollBg->ignoreAnchorPointForPosition(false);
    scrollBg->setContentSize(m_scroll->getContentSize());
    m_mainLayer->addChildAtPosition(scrollBg, Anchor::Center, CCPoint{0, -20});

    updateNodes();

    return true;
}

void IconSearchPopup::updateNodes() {
    m_scroll->m_contentLayer->removeAllChildren();
    if (m_search.page * m_search.pageSize > m_results.size()) m_search.page = m_results.size() / m_search.pageSize;
    log::info("{}; {}; {}", m_search.page * m_search.pageSize, (m_search.page + 1) * m_search.pageSize, m_search.page * m_search.pageSize < (m_search.page + 1) * m_search.pageSize);
    for (int i = m_search.page * m_search.pageSize; i < (m_search.page + 1) * m_search.pageSize; i++) {
        log::info("Doing icon {}", i);
        if (i >= m_results.size()) break;
        log::info("Yeah", i);
        log::info("{} {}", m_results.size(), i);
        auto c = m_results[i].first;
        log::info("A");
        CCSprite* sprite;
        log::info("B");
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
        log::info("C");
        auto btn = CCMenuItemSpriteExtra::create(sprite, m_garage, menu_selector(GJGarageLayer::onSelect));
        log::info("D");
        btn->m_iconType = c.type;
        log::info("E");
        btn->setTag(c.id);
        log::info("F");
        sprite->setPosition({15, 15});
        log::info("G");
        btn->setContentSize({30, 30});
        log::info("H");
        m_scroll->m_contentLayer->addChild(btn);
        log::info("I");
    }
    log::info("J");
    m_scroll->m_contentLayer->updateLayout();
    m_scroll->m_contentLayer->setContentHeight(std::max(m_scroll->m_contentLayer->getContentHeight(),m_scroll->getContentHeight()));
    log::info("K");
    m_scroll->scrollToTop();
    log::info("L");
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

// thanks prevter for giving this code from eclipse that spaghett added
bool matchesStringFuzzy(std::string_view haystack, std::string_view needle) {
    auto it = std::ranges::search(
        haystack, needle, [](char ch1, char ch2) {
            return std::toupper(ch1) == std::toupper(ch2);
        }
    ).begin();

    return (it != haystack.end());
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
    for (auto candidate : m_candidates) {
        if (m_search.unlocked != std::nullopt && candidate.unlocked != m_search.unlocked) continue;
        if (!m_search.types.empty() && !std::ranges::contains(m_search.types, candidate.type)) continue;

        int score = getResultMatch(candidate, lower);
        if (score < 0)
            continue;

        const auto it = std::ranges::lower_bound(
            m_results, score,
            {}, [](auto const& c) {
                return c.second;
            }
        );

        m_results.insert(it, { candidate, score });
    }
    for (auto [candidate, score] : m_results) {
        log::info("{} {} with score {} - {} ", iconTypeToString(candidate.type), candidate.id, score, candidate.desc);
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
            if (!achieve.empty()) res.howToUnlock = am->limitForAchievement(achieve);

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