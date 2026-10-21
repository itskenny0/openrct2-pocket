// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

namespace OpenRCT2::Ui::Handheld::PocketPalette
{
    // Stable entries from SPR_GAME_DEFAULT_PALETTE. Themes never change the
    // world palette, object recolours or cycling colours.
    enum class Style { slate, daylight };
    inline Style style = Style::daylight;
    inline uint8_t panel=20, border=16, separator=17, selection=194;
    inline uint8_t primary=10, secondary=13, dark=21;
    inline uint8_t positive=194, caution=180, destructive=62, disabled=16;
    inline uint8_t body=19, raised=225, edge=16, highlight=21;
    inline uint8_t selectionLight=198, shadow=17, accent=180;
    inline void SetStyle(Style value)
    {
        style=value;
        const bool light=value==Style::daylight;
        panel=light?20:12; border=light?16:15; separator=light?17:14;
        selection=light?194:19; primary=light?10:21; secondary=light?13:20;
        // Existing selected labels use the dark ink role. On teal, use light ink.
        dark=light?21:10; positive=light?194:151; caution=light?180:43;
        destructive=light?62:68; disabled=light?16:19;
        body=light?19:10; raised=light?225:13; edge=light?16:14;
        highlight=light?21:17; selectionLight=light?198:20; shadow=light?17:16;
        accent=light?180:19;
    }
}
