// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

namespace OpenRCT2::Ui::Handheld::PocketPalette
{
    // Stable entries from SPR_GAME_DEFAULT_PALETTE. Themes never change the
    // world palette, object recolours or cycling colours.
    enum class Style { slate, midway };
    inline Style style = Style::midway;
    inline uint8_t panel=154, border=158, separator=157, selection=164;
    inline uint8_t primary=165, secondary=164, dark=154;
    inline uint8_t positive=199, caution=55, destructive=68, disabled=162;
    inline uint8_t body=155, raised=156, edge=157, highlight=160;
    inline uint8_t selectionLight=165, shadow=155, accent=55;
    inline void SetStyle(Style value)
    {
        style=value;
        const bool m=value==Style::midway;
        panel=m?154:12; border=m?158:15; separator=m?157:14;
        selection=m?164:19; primary=m?165:21; secondary=m?164:20;
        dark=m?154:10; positive=m?199:151; caution=m?55:43;
        destructive=68; disabled=m?162:19;
        body=m?155:10; raised=m?156:13; edge=m?157:14;
        highlight=m?160:17; selectionLight=m?165:20; shadow=m?155:16;
        accent=m?55:19;
    }
}
