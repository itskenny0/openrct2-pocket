// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cstddef>

namespace OpenRCT2::Ui::Handheld
{
    // Navigation state only. Native targets, action dispatch and drafts stay in adapters.
    template<size_t Count> struct TabFocus
    {
        size_t tab{};
        std::array<size_t, Count> rows{};
        void save(size_t row) { rows[tab] = row; }
        size_t move(int delta)
        {
            tab = static_cast<size_t>((static_cast<int>(tab) + delta % static_cast<int>(Count)
                + static_cast<int>(Count)) % static_cast<int>(Count));
            return rows[tab];
        }
    };
} // namespace OpenRCT2::Ui::Handheld
