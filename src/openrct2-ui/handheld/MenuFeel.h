// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld
{
    // Presentation only. Native input/actions never wait for a transition: every
    // animation is a pure function of time, so a new press simply restarts it.
    class MenuFeel
    {
    public:
        enum class Event
        {
            none,
            focus, // Selection moved within the same page and tab.
            tab,   // Same page, different tab or section.
            page,  // A different page.
            open,  // First page after the menu opened over the park.
        };
        struct Rect
        {
            int x{}, y{}, width{}, height{};
            bool operator==(const Rect&) const = default;
        };
        // The frame's top-level target, so cards drawn into scrolled sub-targets can be
        // reported in one coordinate space.
        struct Surface
        {
            const uint8_t* bits{};
            int x{}, y{}, stride{};
        };
        Surface base;
        // Shared by every Pocket surface; the pause menu's Motion row toggles it.
        static inline bool reduceMotion = false;
        static constexpr uint64_t kFocusMs = 120, kGlideMs = 110, kPageMs = 110, kOpenMs = 150;

    private:
        std::string _page;
        int _pageId{ -1 };
        size_t _selection{};
        uint64_t _focusTime{}, _pressTime{}, _spriteTick{}, _pageTime{}, _glideTime{};
        bool _ready{}, _pressed{}, _animated{}, _moving{}, _backPending{}, _hidePointer{};
        Event _pageKind{ Event::none };
        int _pageDirection{};
        std::vector<Rect> _frameSelected, _lastSelected;
        Rect _glideFrom, _glideTo;
        bool _hasFocusRect{};

        static int EaseOut(uint64_t elapsed, uint64_t duration, int distance)
        {
            if (elapsed >= duration)
                return 0;
            const double remaining = 1.0 - static_cast<double>(elapsed) / static_cast<double>(duration);
            return static_cast<int>(distance * remaining * remaining * remaining + (distance >= 0 ? 0.5 : -0.5));
        }

    public:
        static uint64_t Clock()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                .count();
        }
        Event Observe(const std::string& page, size_t selected, uint64_t now = Clock(), int pageId = -1)
        {
            auto event = Event::none;
            if (!_ready)
                event = Event::open;
            else if (pageId != _pageId)
                event = Event::page;
            else if (page != _page)
                event = Event::tab;
            else if (selected != _selection)
                event = Event::focus;
            if (event == Event::none)
                return event;
            if (event != Event::focus)
                _pressed = false;
            if (event == Event::open || event == Event::page)
            {
                _pageKind = event;
                _pageDirection = _backPending ? -1 : 1;
                _pageTime = now;
                _hasFocusRect = false; // Snap rather than glide across pages.
            }
            _backPending = false;
            _page = page;
            _pageId = pageId;
            _selection = selected;
            _focusTime = now;
            _ready = true;
            return event;
        }
        void MarkBack()
        {
            _backPending = true;
        }
        void Press(uint64_t now = Clock())
        {
            _pressTime = now;
            _pressed = true;
        }
        bool PressedRecently(uint64_t now = Clock()) const
        {
            return _pressed && now >= _pressTime && now - _pressTime < 150;
        }
        int ButtonOffset(uint64_t now = Clock()) const
        {
            return _pressed && now >= _pressTime && now - _pressTime < 90 ? 1 : 0;
        }
        // Time since focus last moved, for effects that should start fresh on each item.
        uint64_t FocusElapsed(uint64_t now = Clock()) const
        {
            return now >= _focusTime ? now - _focusTime : 0;
        }
        int FocusPercent(uint64_t now = Clock()) const
        {
            if (reduceMotion)
                return 100;
            return static_cast<int>(std::min<uint64_t>(now >= _focusTime ? now - _focusTime : 0, kFocusMs) * 100 / kFocusMs);
        }
        // Content offset while a page enters: sheets rise in over the park, and deeper
        // pages arrive from the right while Back returns from the left.
        std::pair<int, int> Slide(uint64_t now = Clock()) const
        {
            if (reduceMotion || _pageKind == Event::none || now < _pageTime)
                return { 0, 0 };
            const auto elapsed = now - _pageTime;
            if (_pageKind == Event::open)
                return { 0, EaseOut(elapsed, kOpenMs, 18) };
            return { EaseOut(elapsed, kPageMs, 14 * _pageDirection), 0 };
        }

        // Focus tracking for the pointer: drawing reports each selected card, and the
        // card that changed between frames is the one the player just moved to.
        void Selected(Rect rect)
        {
            _frameSelected.push_back(rect);
        }
        void EndFrame(uint64_t now = Clock())
        {
            if (_frameSelected.empty())
            {
                _hasFocusRect = false;
            }
            else
            {
                const Rect* moved = nullptr;
                for (const auto& rect : _frameSelected)
                    if (std::find(_lastSelected.begin(), _lastSelected.end(), rect) == _lastSelected.end())
                    {
                        moved = &rect;
                        break;
                    }
                const bool targetVisible = _hasFocusRect
                    && std::find(_frameSelected.begin(), _frameSelected.end(), _glideTo) != _frameSelected.end();
                if (!_hasFocusRect)
                {
                    _glideTo = _glideFrom = moved != nullptr ? *moved : _frameSelected.back();
                    _glideTime = 0;
                    _hasFocusRect = true;
                }
                else if (moved != nullptr && !(*moved == _glideTo))
                {
                    _glideFrom = FocusRect(now);
                    _glideTo = *moved;
                    _glideTime = now;
                }
                else if (!targetVisible)
                {
                    _glideTo = _glideFrom = _frameSelected.back();
                    _glideTime = 0;
                }
            }
            _lastSelected.swap(_frameSelected);
            _frameSelected.clear();
        }
        bool HasFocusRect() const
        {
            return _hasFocusRect;
        }
        bool Gliding(uint64_t now = Clock()) const
        {
            return !reduceMotion && _glideTime != 0 && now >= _glideTime && now - _glideTime < kGlideMs;
        }
        Rect FocusRect(uint64_t now = Clock()) const
        {
            if (!Gliding(now))
                return _glideTo;
            const auto t = static_cast<double>(now - _glideTime) / kGlideMs;
            const double eased = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
            const auto lerp = [eased](int a, int b) { return a + static_cast<int>((b - a) * eased + (b >= a ? 0.5 : -0.5)); };
            return { lerp(_glideFrom.x, _glideTo.x), lerp(_glideFrom.y, _glideTo.y), lerp(_glideFrom.width, _glideTo.width),
                     lerp(_glideFrom.height, _glideTo.height) };
        }
        // A slow idle bob for the pointer; off with reduced motion.
        int PointerBob(uint64_t now = Clock()) const
        {
            static constexpr int kSteps[] = { 0, 1, 2, 1 };
            return reduceMotion ? 0 : kSteps[(now / 160) % 4];
        }

        // Dense grids (the keyboard) have neighbours where the pointer would sit: they keep the ring
        // and the glide but ask for no pointer this frame.
        void HidePointer()
        {
            _hidePointer = true;
        }
        bool PointerHidden() const
        {
            return _hidePointer;
        }
        void BeginDraw()
        {
            _hidePointer = false;
            _animated = false;
            _frameSelected.clear();
        }
        void AnimatedSprite()
        {
            _animated = true;
        }
        bool NeedsDraw(uint64_t now = Clock())
        {
            const auto tick = now / 120;
            const bool idleBob = _hasFocusRect && !reduceMotion;
            const bool nextFrame = (_animated || idleBob) && tick != _spriteTick;
            _spriteTick = tick;
            const auto slide = Slide(now);
            const bool moving = FocusPercent(now) < 100 || ButtonOffset(now) != 0 || Gliding(now) || slide.first != 0
                || slide.second != 0;
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
        explicit MenuFeelScope(MenuFeel& feel)
            : _previous(activeMenuFeel)
        {
            activeMenuFeel = &feel;
            feel.BeginDraw();
        }
        ~MenuFeelScope()
        {
            activeMenuFeel = _previous;
        }
    };
    inline int ButtonPressOffset(bool selected)
    {
        return selected && activeMenuFeel ? activeMenuFeel->ButtonOffset() : 0;
    }
    inline void ReportSelected(int x, int y, int width, int height)
    {
        if (activeMenuFeel)
            activeMenuFeel->Selected({ x, y, width, height });
    }
} // namespace OpenRCT2::Ui::Handheld
