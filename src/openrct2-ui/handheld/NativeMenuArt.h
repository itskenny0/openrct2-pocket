// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "MenuFeel.h"
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/RenderTarget.h>
#include <string_view>

namespace OpenRCT2::Ui::Handheld
{
    struct NativeMenuArt
    {
        uint32_t sprite{};
        uint8_t frames = 1;
    };
    inline NativeMenuArt TabArt(std::string_view label)
    {
        if (label == "Overview") return { SPR_TAB_RIDE_0, 16 };
        if (label == "Operations" || label == "Settings" || label == "Maintenance") return { SPR_TAB_GEARS_0, 4 };
        if (label == "Statistics" || label == "History" || label == "Graphs") return { SPR_TAB_GRAPH_0, 8 };
        if (label == "Needs" || label == "Guests") return { SPR_TAB_GUESTS_0, 8 };
        if (label == "Summary" || label == "Finances") return { SPR_TAB_FINANCES_SUMMARY_0, 8 };
        if (label == "Research") return { SPR_TAB_FINANCES_RESEARCH_0, 8 };
        if (label == "Marketing") return { SPR_TAB_FINANCES_MARKETING_0, 19 };
        if (label == "Staff" || label == "Duties") return { SPR_TAB_STAFF_OPTIONS_0, 7 };
        return {};
    }
    inline NativeMenuArt ActionArt(std::string_view label)
    {
        // Exact UI labels only; arbitrary ride/guest/object names are never interpreted.
        if (label == "Open..." || label == "Open") return { SPR_OPEN };
        if (label == "Close..." || label == "Close") return { SPR_CLOSED };
        if (label == "Test..." || label == "Test unavailable") return { SPR_TESTING };
        if (label == "Locate" || label == "Locate ride") return { SPR_LOCATE };
        if (label == "Statistics..." || label == "Statistics") return { SPR_GRAPH };
        if (label == "Save park" || label == "Save and quit" || label == "Save") return { SPR_FLOPPY };
        if (label == "Load park" || label == "Browse...") return { SPR_G2_FOLDER };
        if (label == "Quit..." || label == "Quit without saving...") return { SPR_MENU_EXIT };
        if (label == "Controls...") return { SPR_G2_CONTROLS_TAB_START, 8 };
        if (label == "Build" || label == "Edit track...") return { SPR_CONSTRUCTION };
        if (label == "Park dashboard") return { SPR_PARK_ENTRANCE };
        if (label == "Resume park") return { SPR_TAB_RIDE_0, 16 };
        if (label == "Vehicles..." || label == "Vehicles") return { SPR_RIDE };
        if (label == "Rename..." || label == "Rename") return { SPR_RENAME };
        if (label == "Music...") return { SPR_TAB_MUSIC_0, 16 };
        if (label == "Appearance...") return { SPR_PAINTBRUSH };
        if (label == "Demolish...") return { SPR_DEMOLISH };
        if (label == "Entrances / exits..." || label == "Place entrance / exit...") return { SPR_PARK_ENTRANCE };
        if (label == "Back to roster" || label == "Cancel") return { SPR_PREVIOUS };
        return {};
    }
    inline bool DrawNativeMenuArt(Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size,
        NativeMenuArt art, bool selected = false)
    {
        if (!art.sprite) return false;
        if (selected && art.frames > 1)
        {
            if (activeMenuFeel) activeMenuFeel->AnimatedSprite();
            art.sprite += static_cast<uint32_t>((MenuFeel::Clock() / 120) % art.frames);
        }
        const auto* image = GfxGetG1Element(art.sprite);
        if (!image || image->width <= 0 || image->height <= 0) return false;
        int zoom = 0;
        // Toolbar/tab sprites have a small transparent rim. Keep their native
        // pixels rather than halving a whole icon for a one-pixel overflow.
        while (zoom < 3 && ((image->width >> zoom) > size.width + 6 || (image->height >> zoom) > size.height + 6)) ++zoom;
        Drawing::RenderTarget clipped;
        if (!ClipRenderTarget(clipped, rt, pos, size.width, size.height)) return false;
        clipped.zoom_level = ZoomLevel{ static_cast<int8_t>(zoom) };
        const int factor = 1 << zoom;
        GfxDrawSprite(clipped, ImageId(art.sprite),
            { (size.width * factor - image->width) / 2 - image->xOffset,
              (size.height * factor - image->height) / 2 - image->yOffset });
        return true;
    }
}
