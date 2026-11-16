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
    inline NativeMenuArt TabArt(std::string_view label, std::string_view context = {})
    {
        if (label == "Overview")
        {
            if (context.starts_with("Park / Finances") || context.starts_with("Finances /")) return { SPR_TAB_FINANCES_SUMMARY_0, 8 };
            if (context.starts_with("Park / Guests") || context.starts_with("Guests /")) return { SPR_TAB_GUESTS_0, 8 };
            if (context.starts_with("Park / Staff") || context.starts_with("Staff /")) return { SPR_TAB_STAFF_OPTIONS_0, 7 };
            return { SPR_TAB_RIDE_0, 16 };
        }
        if (label == "Operations" || label == "Settings") return { SPR_TAB_GEARS_0, 4 };
        if (label == "Statistics" || label == "History" || label == "Graphs") return { SPR_TAB_GRAPH_0, 8 };
        if (label == "Needs" || label == "Guests") return { SPR_TAB_GUESTS_0, 8 };
        if (label == "Summary" || label == "Finances") return { SPR_TAB_FINANCES_SUMMARY_0, 8 };
        if (label == "Research") return { SPR_TAB_FINANCES_RESEARCH_0, 8 };
        if (label == "Marketing") return { SPR_TAB_FINANCES_MARKETING_0, 19 };
        if (label == "Staff" || label == "Duties") return { SPR_TAB_STAFF_OPTIONS_0, 7 };
        if (label == "Thoughts") return { SPR_TAB_THOUGHTS_0, 8 };
        if (label == "Patrol") return { SPR_PATROL_BTN };
        if (label == "Details" && (context.starts_with("Park / Guests") || context.starts_with("Guests /"))) return { SPR_TAB_GUEST_INVENTORY };
        if (label == "Details" || label == "Records") return { SPR_TAB_STATS_0, 7 };
        if (label == "Maintenance") return { SPR_TAB_WRENCH_0, 16 };
        if (label == "Ledger") return { SPR_FINANCE };
        if (label == "Controls") return { SPR_TAB_GEARS_0, 4 };
        if (label == "Park") return { SPR_TAB_PARK_ENTRANCE };
        if (label == "Objectives") return { SPR_TAB_OBJECTIVE_0, 16 };
        if (label == "Project") return { SPR_TAB_FINANCES_RESEARCH_0, 8 };
        if (label == "Funding & priorities") return { SPR_TAB_FINANCES_SUMMARY_0, 8 };
        if (label == "Active campaigns" || label == "New campaign") return { SPR_TAB_FINANCES_MARKETING_0, 19 };
        if (label == "Terrain") return { SPR_G2_TAB_LAND };
        if (label == "Rides") return { SPR_TAB_RIDE_0, 16 };
        return {};
    }
    inline NativeMenuArt ActionArt(std::string_view label)
    {
        // Exact UI labels only; never infer an action from a substring.
        if (label == "Open..." || label == "Open" || label == "Open park...") return { SPR_OPEN };
        if (label == "Close..." || label == "Close" || label == "Close park...") return { SPR_CLOSED };
        if (label == "Test..." || label == "Test unavailable") return { SPR_TESTING };
        if (label == "Locate" || label == "Locate ride" || label == "Locate subject") return { SPR_LOCATE };
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
        if (label == "Demolish..." || label == "Demolish ride") return { SPR_DEMOLISH };
        if (label == "Entrances / exits..." || label == "Place entrance / exit...") return { SPR_PARK_ENTRANCE };
        if (label == "Pick up...") return { SPR_PICKUP_BTN };
        if (label == "Patrol area...") return { SPR_PATROL_BTN };
        if (label == "Name..." || label == "Name filter...") return { SPR_RENAME };
        if (label == "Uniform colour...") return { SPR_PAINTBRUSH };
        if (label == "Costume...") return { SPR_STAFF_COSTUME_ELEPHANT };
        if (label == "Tracking: On..." || label == "Tracking: Off..."
            || label == "Enable tracking" || label == "Stop tracking") return { SPR_TRACK_PEEP };
        if (label == "Carried items...") return { SPR_TAB_GUEST_INVENTORY };
        if (label == "Spending...") return { SPR_FINANCE };
        if (label == "Ride history...") return { SPR_RIDE };
        if (label == "Preferences..." || label == "Operations...") return { SPR_TAB_GEARS_0, 4 };
        if (label == "Maintenance...") return { SPR_TAB_WRENCH_0, 16 };
        if (label == "Dispatch...") return { SPR_TAB_TIMER_0, 8 };
        if (label == "Access / paths...") return { SPR_G2_BUTTON_FOOTPATH };
        if (label == "Park map..." || label == "Map roster") return { SPR_MAP };
        if (label == "Park news...") return { SPR_G2_TAB_NEWS };
        if (label == "Park awards...") return { SPR_AWARD };
        if (label == "Read objectives") return { SPR_TAB_OBJECTIVE_0, 16 };
        if (label == "Park history..." || label == "Financial history") return { SPR_GRAPH };
        if (label == "Borrow / repay..." || label == "Borrow or repay" || label == "Expenditure ledger" || label == "Spending ledger" || label == "Financial controls") return { SPR_FINANCE };
        if (label == "Admission price..." || label == "Admission...") return { SPR_TAB_ADMISSION_0, 8 };
        if (label == "Invention catalog...") return { SPR_TAB_FINANCES_RESEARCH_0, 8 };
        if (label == "Funding & priorities") return { SPR_TAB_FINANCES_SUMMARY_0, 8 };
        if (label == "New campaign..." || label == "Choose new campaign") return { SPR_TAB_FINANCES_MARKETING_0, 19 };
        if (label == "Orders...") return { SPR_TAB_STAFF_OPTIONS_0, 7 };
        // The exact native order labels, including both toggle states.
        if (label == "Sweep paths  On" || label == "Sweep paths  Off") return { SPR_STAFF_ORDERS_SWEEPING };
        if (label == "Water gardens  On" || label == "Water gardens  Off") return { SPR_STAFF_ORDERS_WATER_FLOWERS };
        if (label == "Empty bins  On" || label == "Empty bins  Off") return { SPR_STAFF_ORDERS_EMPTY_BINS };
        if (label == "Mow grass  On" || label == "Mow grass  Off") return { SPR_STAFF_ORDERS_MOWING };
        if (label == "Inspect rides  On" || label == "Inspect rides  Off") return { SPR_STAFF_ORDERS_INSPECT_RIDES };
        if (label == "Fix rides  On" || label == "Fix rides  Off") return { SPR_STAFF_ORDERS_FIX_RIDES };
        if (label == "Open ride management") return { SPR_RIDE };
        if (label == "Back to roster" || label == "Back to dashboard" || label == "Back to employee"
            || label == "Back to guest" || label == "Back to campaigns" || label == "Back to news"
            || label == "Cancel") return { SPR_PREVIOUS };
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
