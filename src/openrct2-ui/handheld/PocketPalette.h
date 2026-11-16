// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>

namespace OpenRCT2::Ui::Handheld::PocketPalette
{
    // Stable entries from SPR_GAME_DEFAULT_PALETTE. Themes never change the
    // world palette, object recolours or cycling colours.
    enum class Style { slate, daylight, pixel };
    inline Style style = Style::daylight;
    inline uint8_t panel=20, border=16, separator=17, selection=194;
    inline uint8_t primary=10, secondary=13, dark=21;
    inline uint8_t positive=194, caution=180, destructive=62, disabled=16;
    inline uint8_t body=19, raised=225, edge=16, highlight=21;
    inline uint8_t selectionLight=198, shadow=17, accent=180;
    inline void SetStyle(Style value)
    {
        style=value;
        const bool light=value!=Style::slate;
        panel=light?20:12; border=light?16:15; separator=light?17:14;
        // Both themes select with a saturated colour and light label ink, so the highlight
        // reads at a glance (and section colours can replace it) on ivory or slate panels.
        selection=194; primary=light?10:21; secondary=light?13:20;
        dark=21; positive=light?194:151; caution=light?180:43;
        destructive=light?62:68; disabled=light?16:19;
        body=light?19:10; raised=light?225:13; edge=light?16:14;
        highlight=light?21:17; selectionLight=198; shadow=light?17:16;
        accent=light?180:19;
        if(value==Style::pixel)
        {
            // Pixel: white windows with navy outlines and pale blue cards, navy ink - the look of
            // a handheld RPG menu, built from the same RCT2 palette.
            panel=21; border=119; separator=139; edge=119; body=21; raised=141; highlight=21;
            primary=118; secondary=121; dark=21; shadow=138; disabled=16; accent=52;
        }
    }
    inline bool Pixel() { return style == Style::pixel; }
}
