#include "IconSearchPopup.hpp"

#include <algorithm>
#include <utility>
#include <cue/PlayerIcon.hpp>
#include <cue/DropdownNode.hpp>

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
    setID("IconSearchPopup");
    m_bgSprite->setZOrder(-2);
    m_bgSprite->setID("background");
    m_closeBtn->setID("close-btn");
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
    m_buttonMenu->addChildAtPosition(searchBtn, Anchor::Top, CCPoint{
                                         m_input->getScaledContentWidth() / 2 + searchBtn->getScaledContentWidth() / 2 +
                                         5,
                                         -30
                                     });

    auto filterSpr = makeSprite("GJ_filterIcon_001.png", 1.f, "geode.loader/GE_button_05.png");
    filterSpr->setScale(.75f);
    auto filterBtn = CCMenuItemExt::createSpriteExtra(filterSpr, [this](auto) {
        IconSearchFilterPopup::create(this)->show();
    });
    filterBtn->setID("filter-btn");
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Top, CCPoint{
                                         -m_input->getScaledContentWidth() / 2 - filterBtn->getScaledContentWidth() / 2
                                         - 5,
                                         -30
                                     });

    m_menu = CCMenu::create();
    m_menu->setID("search-menu");
    m_menu->setContentSize({360, 210});
    m_menu->ignoreAnchorPointForPosition(false); // why does this exist, it sucks :P
    m_mainLayer->addChildAtPosition(m_menu, Anchor::Center, CCPoint{0, -20});
    m_menu->setLayout(
        RowLayout::create()->setGrowCrossAxis(true)->setCrossAxisAlignment(AxisAlignment::End)->
                             setCrossAxisOverflow(false)->setGap(2));

    auto bg = CCScale9Sprite::create("square02_001.png");
    bg->setID("search-bg");
    bg->setZOrder(-1);
    bg->setOpacity(72);
    bg->setContentSize(m_menu->getContentSize());
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

void IconSearchPopup::keyDown(enumKeyCodes key) {
    if (key == KEY_Enter) {
        updateNodes();
        m_input->getInputNode()->onClickTrackNode(false);
        return;
    }
    Popup::keyDown(key);
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

void IconSearchPopup::updateNodes() {
    updateResults();
    auto gm = GameManager::get();
    m_menu->removeAllChildren();
    if (m_search.page < 0) m_search.page = 0;
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
                if (c.id > 5 && c.id < 16) {
                    sprite = GJPathSprite::create(c.id - 5);
		    break;
                }
                sprite = CCSprite::createWithSpriteFrameName(fmt::format("gjItem_{:02}_001.png", c.id).c_str());
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
		    .color1 = gm->getPlayerColor(),
		    .color2 = gm->getPlayerColor2(),
		});
                sprite = is;
            }
        }
	sprite->setScale(30 / std::max(sprite->getContentHeight(), sprite->getContentWidth()));
        auto btn = CCMenuItemExt::createSpriteExtra(sprite, [this, gm, c](auto){
		// TODO: Implement more like vanilla
		ItemInfoPopup::create(c.id, gm->iconTypeToUnlockType(c.type))->show();
	});
        btn->m_iconType = c.type;
        btn->setTag(c.id);
        sprite->setPosition({15, 15});
        btn->setContentSize({30, 30});
        btn->setID(fmt::format("{}-{}", iconTypeToString(c.type), c.id));
        m_menu->addChild(btn);
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

int getResultMatch(const SearchCandidate& candidate, std::string_view search) {
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

        m_results.insert(it, {candidate, score});
    }
}

void IconSearchPopup::getCandidates() {
    m_candidates.clear();
    auto gm = GameManager::sharedState();
    for (IconType type : {IconType::Cube, IconType::Ship, IconType::Ball, IconType::Ufo, IconType::Wave, IconType::Robot, IconType::Spider, IconType::Swing, IconType::Jetpack, IconType::DeathEffect, IconType::Special, IconType::ShipFire}) {
        auto count = gm->countForType(type);
        for (int id = 1; id <= count; id++) {
            addCandidate(type, id);
        }
    }
    for (int i = 1; i <= 20; i++) {
        addCandidate(IconType::Item, i);
    }
    log::debug("Loaded {} search candidates.", m_candidates.size());
}

void IconSearchPopup::addCandidate(IconType type, int id) {
    auto gm = GameManager::sharedState();
    auto gsm = GameStatsManager::sharedState();
    auto am = AchievementManager::sharedState();
    auto ut = gm->iconTypeToUnlockType(type);
    auto res = SearchCandidate{
        .type = type,
        .id = id,
        .unlocked = type == IconType::Item ? gsm->isItemUnlocked(ut, id) : gm->isIconUnlocked(id, type),
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
            res.achievement =string::toLower(data->valueForKey("title")->getCString());
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
	auto cb = [this, parent, type](CCMenuItemToggler* btn){
		if (!btn->isOn()) parent->m_search.types.push_back(type);
		else std::erase(parent->m_search.types, type);
	};
	if (has) btn = CCMenuItemExt::createTogglerWithFrameName(fmt::format("gj_{}Btn_on_001.png", str), fmt::format("gj_{}Btn_off_001.png", str), .8f, cb);
	else {
		CCSprite* gs1;
		CCSprite* gs2;
		if (sheet) {
		 	gs1 = CCSpriteGrayscale::createWithSpriteFrameName(str);
			gs2 = CCSpriteGrayscale::createWithSpriteFrameName(str);
		} else {
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
    static auto makeLabel = [](const char* text){
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
    } else if (parent->m_search.unlocked == true) {
	drop->addCell(yes);
	drop->addCell(all);
	drop->addCell(no);
    } else {
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
