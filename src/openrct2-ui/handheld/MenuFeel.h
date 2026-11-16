// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>

namespace OpenRCT2::Ui::Handheld
{
    // Presentation only. Native input/actions never wait for a transition.
    class MenuFeel
    {
        std::string _page;
        size_t _selection{};
        uint64_t _focusTime{}, _pressTime{}, _spriteTick{};
        bool _ready{}, _pressed{}, _animated{}, _moving{};
    public:
        static uint64_t Clock()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
        void Observe(const std::string& page, size_t selected, uint64_t now = Clock())
        {
            if (!_ready || page != _page || selected != _selection)
            {
                if (!_ready || page != _page) _pressed = false;
                _page = page; _selection = selected; _focusTime = now; _ready = true;
            }
        }
        void Press(uint64_t now = Clock()) { _pressTime = now; _pressed = true; }
        int ButtonOffset(uint64_t now = Clock()) const
        {
            return _pressed && now >= _pressTime && now - _pressTime < 90 ? 1 : 0;
        }
        int FocusPercent(uint64_t now = Clock()) const
        {
            return static_cast<int>(std::min<uint64_t>(now >= _focusTime ? now - _focusTime : 0, 120) * 100 / 120);
        }
        void BeginDraw() { _animated = false; }
        void AnimatedSprite() { _animated = true; }
        bool NeedsDraw(uint64_t now = Clock())
        {
            const auto tick = now / 120;
            const bool nextFrame = _animated && tick != _spriteTick;
            _spriteTick = tick;
            const bool moving = FocusPercent(now) < 100 || ButtonOffset(now) != 0;
            const bool settled = _moving && !moving;
            _moving = moving;
            return nextFrame || moving || settled;
        }
    };
    inline thread_local MenuFeel* activeMenuFeel{};
    class MenuFeelScope
    {
        MenuFeel* _previous;
    public:
        explicit MenuFeelScope(MenuFeel& feel) : _previous(activeMenuFeel)
        {
            activeMenuFeel = &feel; feel.BeginDraw();
        }
        ~MenuFeelScope() { activeMenuFeel = _previous; }
    };
    inline int ButtonPressOffset(bool selected)
    {
        return selected && activeMenuFeel ? activeMenuFeel->ButtonOffset() : 0;
    }
}
