#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/Enums.hpp>

#include <string>
#include <optional>
#include <vector>
#include <utility>

#define MORE_ICONS_EVENTS
#include <hiimjustin000.more_icons/include/MoreIconsV2.hpp>

struct Search {
    std::string query;
    std::vector<IconType> types;
    std::optional<bool> unlocked = std::nullopt;
    int page = 0;
    int pageSize = 84;
};

struct SearchResult {
    IconType type;
    int id;
    std::string desc;
    std::string game;
    std::string achievement;
    std::string name;
    std::optional<IconInfo> moreIconsInfo = std::nullopt;
    bool isUnlocked() const;
};

class IconSearchPopup : public geode::Popup<GJGarageLayer*> {
protected:
    bool setup(GJGarageLayer* garage) override;
    void keyDown(cocos2d::enumKeyCodes key, double ts) override;

    // cause getting all this info might be laggy, id rather do it once
    std::vector<SearchResult> m_candidates;
    std::vector<std::pair<SearchResult, int>> m_results;
    GJGarageLayer* m_garage = nullptr;
    cocos2d::CCMenu* m_menu = nullptr;
    geode::TextInput* m_input = nullptr;
    CCMenuItemSpriteExtra* m_prev = nullptr;
    CCMenuItemSpriteExtra* m_next = nullptr;
public:
    void updateNodes();
    void updateResults();
    void getCandidates();
    void loadMoreIconsCandidates();
    void addCandidate(IconType type, int id);
    void addMICandidate(IconType type, IconInfo& info);

    static IconSearchPopup* create(GJGarageLayer* garage);

    Search m_search;
};

class IconSearchFilterPopup : public geode::Popup<IconSearchPopup*> {
protected:
    bool setup(IconSearchPopup* parent) override;
    void onClose(CCObject* sender) override;

    IconSearchPopup* m_parent = nullptr;
public:
    static IconSearchFilterPopup* create(IconSearchPopup* parent);
};
