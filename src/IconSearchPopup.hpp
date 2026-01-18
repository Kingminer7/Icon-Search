#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/Enums.hpp>

#include <string>
#include <optional>
#include <vector>
#include <utility>

struct Search {
    std::string query;
    std::vector<IconType> types;
    std::optional<bool> unlocked = std::nullopt;
    int page = 0;
    int pageSize = 84;
};

struct SearchCandidate {
    IconType type;
    int id;
    std::string desc;
    bool unlocked;
    std::string game;
    std::string achievement;
};

class IconSearchPopup : public geode::Popup<GJGarageLayer*> {
protected:
    bool setup(GJGarageLayer* garage) override;
    void keyDown(cocos2d::enumKeyCodes key) override;

    void updateNodes();
    void updateResults();
    void getCandidates();

    // cause getting all this info might be laggy, id rather do it once
    std::vector<SearchCandidate> m_candidates;
    Search m_search;
    std::vector<std::pair<SearchCandidate, int>> m_results;
    GJGarageLayer* m_garage = nullptr;
    cocos2d::CCMenu* m_menu = nullptr;
    geode::TextInput* m_input = nullptr;
    CCMenuItemSpriteExtra* m_prev = nullptr;
    CCMenuItemSpriteExtra* m_next = nullptr;
public:
    static IconSearchPopup* create(GJGarageLayer* garage);
};
