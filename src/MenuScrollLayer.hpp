#pragma once

#include <Geode/ui/ScrollLayer.hpp>
#include <cocos2d.h>

class MenuScrollLayer : public geode::ScrollLayer {
protected:
    cocos2d::CCMenuItem* m_pSelectedItem = nullptr;
    cocos2d::tCCMenuState m_eState = cocos2d::kCCMenuStateWaiting;
public:
    MenuScrollLayer(cocos2d::CCRect const& rect, bool scrollWheelEnabled, bool vertical);

    bool ccTouchBegan(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) override;
    void ccTouchMoved(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) override;
    void ccTouchEnded(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) override;
    void ccTouchCancelled(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) override;

    cocos2d::CCMenuItem* itemForTouch(cocos2d::CCTouch *touch);

    static MenuScrollLayer* create(cocos2d::CCRect const& rect, bool scrollWheelEnabled = true, bool vertical = true);
    static MenuScrollLayer* create(cocos2d::CCSize const& size, bool scroll = true, bool vertical = true);
};