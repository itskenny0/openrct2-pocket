// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

namespace OpenRCT2::Ui::Handheld::PocketPalette
{
    // Stable entries from SPR_GAME_DEFAULT_PALETTE. Never approximate surfaces
    // with pixel patterns or modify the world palette.
    inline constexpr uint8_t panel = 12;
    inline constexpr uint8_t border = 15;
    inline constexpr uint8_t separator = 14;
    inline constexpr uint8_t selection = 19;
    inline constexpr uint8_t primary = 21;
    inline constexpr uint8_t secondary = 20;
    inline constexpr uint8_t dark = 10;
    inline constexpr uint8_t positive = 151;
    inline constexpr uint8_t caution = 43;
    inline constexpr uint8_t destructive = 68;
    inline constexpr uint8_t disabled = 19;
} // namespace OpenRCT2::Ui::Handheld::PocketPalette

