#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>

#include "IconSearchPopup.hpp"

using namespace geode::prelude;

class $modify(MyGJGarageLayer, GJGarageLayer) {
	bool init() {
		if (!GJGarageLayer::init()) return false;

		auto spr = IconSelectButtonSprite::createWithSpriteFrameName("edit_findBtn_001.png", 1.25, IconSelectBaseColor::Unselected);
		spr->setScale(.9f);
		auto myButton = CCMenuItemExt::createSpriteExtra(spr, [this](auto){
			IconSearchPopup::create(this)->show();
		});
	    myButton->setID("search-button"_spr);

		auto menu = this->getChildByID("category-menu");
		menu->addChild(myButton);
		menu->updateLayout();
		return true;
	}
};

std::vector<Hook*> g_hooks = {};

#define addHook(name) { \
	Result<Hook*> res = self.getHook(name); \
	if (res) { \
		auto hook = res.unwrap(); \
		hook->setAutoEnable(false); \
		if (Mod::get()->getSettingValue<bool>("icon-hack")) { \
			if (auto res2 = hook->enable(); !res2) { \
				log::error("Failed to enable {} hook: {}", hook->getDisplayName(), res2.unwrapErr()); \
			} \
		} \
		g_hooks.push_back(hook); \
	} else { \
		log::error("Failed to get hook " #name ": {}", res.unwrapErr()); \
	} \
}

class $modify(UnlockIconsGMHook, GameManager) {

    static void onModify(auto& self) {
        addHook("GameManager::isColorUnlocked");
        addHook("GameManager::isIconUnlocked");
    }

    bool isColorUnlocked(int key, UnlockType type) {
        return true;
    }

    bool isIconUnlocked(int key, IconType type) {
        return true;
    }
};

class $modify(IconHackGSM, GameStatsManager) {

    static void onModify(auto& self) {
        addHook("GameStatsManager::isItemUnlocked");
    }

    bool isItemUnlocked(UnlockType type, int key) {
        return true;
    }
};

$on_mod(Loaded) {
    listenForSettingChanges<bool>("icon-hack", [](bool v) {
        for (auto hook : g_hooks) {
            if (v) {
                if (auto res = hook->enable(); !res) {
                    log::error("Failed to enable {} hook: {}", hook->getDisplayName(), res.unwrapErr());
                }
            } else {
                if (auto res = hook->disable(); !res) {
                    log::error("Failed to disable {} hook: {}", hook->getDisplayName(), res.unwrapErr());
                }
            }
        }
    });
}