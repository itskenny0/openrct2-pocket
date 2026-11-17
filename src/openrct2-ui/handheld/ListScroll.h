// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace OpenRCT2::Ui::Handheld
{
    // Window-owned presentation state; input and native actions never wait for motion.
    class ListScroll
    {
        double _position{}, _target{};
        uint64_t _time{};
        int _height{}, _pitch{};
        bool _ready{};

    public:
        static uint64_t Clock()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        }
        int offset(size_t selectedRow, size_t rows, int height, int pitch, uint64_t now = Clock())
        {
            height = std::max(1, height);
            pitch = std::max(1, pitch);
            const double maximum = std::max(0, static_cast<int>(rows) * pitch - height);
            const double top = static_cast<double>(rows ? std::min(selectedRow, rows - 1) : 0) * pitch;
            const double bottom = top + pitch;
            const int margin = height >= pitch * 3 ? pitch : 0;
            _target = std::clamp(_target, 0.0, maximum);
            if (top < _target + margin)
                _target = top - margin;
            else if (bottom > _target + height - margin)
                _target = bottom - height + margin;
            _target = std::clamp(_target, 0.0, maximum);
            if (!_ready || height != _height || pitch != _pitch || std::abs(_target - _position) > height)
                _position = _target;
            else
            {
                const auto elapsed = now >= _time ? std::min<uint64_t>(now - _time, 64) : 0;
                _position += (_target - _position) * (1.0 - std::exp(-static_cast<double>(elapsed) / 45.0));
                if (std::abs(_target - _position) < 0.5)
                    _position = _target;
            }
            // Held repeats may outrun animation. Keep the selected row visible.
            _position = std::clamp(_position, std::max(0.0, bottom - height), std::max(0.0, top));
            _position = std::clamp(_position, 0.0, maximum);
            _ready = true;
            _time = now;
            _height = height;
            _pitch = pitch;
            return static_cast<int>(std::lround(_position));
        }
        bool moving() const
        {
            return std::abs(_target - _position) >= 0.5;
        }
    };
    inline thread_local ListScroll* activeListScroll{};
    class ListScrollScope
    {
        ListScroll* _previous;

    public:
        explicit ListScrollScope(ListScroll& state)
            : _previous(activeListScroll)
        {
            activeListScroll = &state;
        }
        ~ListScrollScope()
        {
            activeListScroll = _previous;
        }
    };
    inline int ListOffset(size_t selectedRow, size_t rows, int height, int pitch)
    {
        if (activeListScroll)
            return activeListScroll->offset(selectedRow, rows, height, pitch);
        ListScroll immediate;
        return immediate.offset(selectedRow, rows, height, pitch);
    }
} // namespace OpenRCT2::Ui::Handheld
