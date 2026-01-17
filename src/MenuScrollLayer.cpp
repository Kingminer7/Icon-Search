#include "MenuScrollLayer.hpp"

using namespace geode::prelude;

MenuScrollLayer::MenuScrollLayer(CCRect const& rect, bool scrollWheelEnabled, bool vertical): ScrollLayer(rect, scrollWheelEnabled, vertical) {}

bool MenuScrollLayer::ccTouchBegan(CCTouch* pTouch, CCEvent* pEvent) {
    auto ret = ScrollLayer::ccTouchBegan(pTouch, pEvent);
    if (!ret) return false;

    m_pSelectedItem = this->itemForTouch(pTouch);
    if (m_pSelectedItem)
    {
        m_eState = kCCMenuStateTrackingTouch;
        m_pSelectedItem->selected();
        return true;
    }
    return true;
}

void MenuScrollLayer::ccTouchMoved(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) {
    ScrollLayer::ccTouchMoved(pTouch, pEvent);
    if ((pTouch->getLocation() - pTouch->getStartLocation()).getLength() > 5.f) {
        if (m_pSelectedItem) {
            m_pSelectedItem->unselected();
            m_pSelectedItem = nullptr;
        }
        m_eState = kCCMenuStateWaiting;
    }
    if (m_eState == kCCMenuStateWaiting) return;
    CCMenuItem *currentItem = this->itemForTouch(pTouch);
    if (currentItem != m_pSelectedItem) {
        if (m_pSelectedItem) m_pSelectedItem->unselected();
        m_pSelectedItem = currentItem;
        if (m_pSelectedItem != nullptr) m_pSelectedItem->selected();
    }
}

void MenuScrollLayer::ccTouchEnded(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) {
    ScrollLayer::ccTouchEnded(pTouch, pEvent);
    if (m_pSelectedItem)
    {
        m_pSelectedItem->unselected();
        m_pSelectedItem->activate();
    }
    m_eState = kCCMenuStateWaiting;
}

void MenuScrollLayer::ccTouchCancelled(cocos2d::CCTouch* pTouch, cocos2d::CCEvent* pEvent) {
    ScrollLayer::ccTouchCancelled(pTouch, pEvent);

    if (m_pSelectedItem) {
        m_pSelectedItem->unselected();
    }
    m_pSelectedItem = nullptr;
    m_eState = kCCMenuStateWaiting;
}

cocos2d::CCMenuItem* MenuScrollLayer::itemForTouch(cocos2d::CCTouch* touch) {
    CCPoint touchLocation = touch->getLocation();

    if (m_contentLayer->m_pChildren && m_contentLayer->m_pChildren->count() > 0) {
        for (auto pChild : m_contentLayer->getChildrenExt<CCMenuItem>()) {
            if (pChild && pChild->isVisible() && pChild->isEnabled()) {
                CCPoint local = pChild->convertToNodeSpace(touchLocation);
                CCRect r = pChild->rect();
                r.origin = CCPointZero;

                if (r.containsPoint(local)) {
                    return pChild;
                }
            }
        }
    }

    return nullptr;
}

MenuScrollLayer* MenuScrollLayer::create(cocos2d::CCRect const& rect, bool scrollWheelEnabled, bool vertical) {
    auto ret = new MenuScrollLayer(rect, scrollWheelEnabled, vertical);
    return ret->autorelease(), ret;
}

MenuScrollLayer* MenuScrollLayer::create(cocos2d::CCSize const& size, bool scroll, bool vertical) {
    return MenuScrollLayer::create({ 0, 0, size.width, size.height }, scroll, vertical);
}
