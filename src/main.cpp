#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp>

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
	    myButton->setID("my-button"_spr);

		auto menu = this->getChildByID("category-menu");
		menu->addChild(myButton);
		menu->updateLayout();
		return true;
	}
};
