// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <cstddef>
#include <cstdint>

namespace OpenRCT2::Ui::Handheld
{
    // Presentation-only navigation. Activation remains owned by the current screen.
    class ListFocus
    {
    public:
        size_t index() const
        {
            return _index;
        }
        void move(int32_t direction, size_t count)
        {
            if (count == 0)
            {
                _index = 0;
                return;
            }
            _index = static_cast<size_t>((static_cast<int64_t>(_index % count) + direction + count) % count);
        }

    private:
        size_t _index{};
    };
} // namespace OpenRCT2::Ui::Handheld
