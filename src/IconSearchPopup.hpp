#pragma once

#include <Geode/ui/Popup.hpp>
#include <Geode/Enums.hpp>

#include <string>
#include <optional>
#include <vector>
#include <utility>

#include "MenuScrollLayer.hpp"

struct Search {
    std::string query;
    std::vector<IconType> types;
    std::optional<bool> unlocked = std::nullopt;
    int page = 0;
    int pageSize = 77;
};

struct SearchCandidate {
    IconType type;
    int id;
    std::string desc;
    bool unlocked;
    int howToUnlock;
};

class IconSearchPopup : public geode::Popup<GJGarageLayer*> {
protected:
    bool setup(GJGarageLayer* garage) override;
    void updateNodes();
    void updateResults();
    static void getCandidates();

    // cause getting all this info might be laggy, id rather do it once
    static std::vector<SearchCandidate> m_candidates;
    Search m_search;
    std::vector<std::pair<SearchCandidate, int>> m_results;
    GJGarageLayer* m_garage = nullptr;
    MenuScrollLayer* m_scroll = nullptr;
public:
    static IconSearchPopup* create(GJGarageLayer* garage);
};
