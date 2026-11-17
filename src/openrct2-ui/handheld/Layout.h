// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <openrct2/drawing/RenderTarget.h>

namespace OpenRCT2::Ui::Handheld::Layout
{
    inline constexpr int margin = 8;
    inline constexpr int hudTop = 6;
    inline constexpr int hudHeight = 24;
    inline constexpr int gutter = 6;
    inline constexpr int actionHeight = 28;
    inline constexpr ScreenCoordsXY menuOrigin{ margin, margin };
    inline constexpr ScreenCoordsXY toolOrigin{ margin, hudTop + hudHeight + gutter };

    // The Pocket layouts are designed for a canvas of at least 480x360. Pick the largest whole
    // pixel scale that keeps the canvas at least that big, so the pixel art stays sharp on any
    // panel: 1024x768 -> 2x (512x384), 640x480 -> 1x, 1280x720 -> 2x (640x360),
    // 1920x1080 -> 3x (640x360), 720x720 -> 1x.
    inline constexpr int minCanvasWidth = 480, minCanvasHeight = 360;
    constexpr int AutoScale(int physicalWidth, int physicalHeight)
    {
        return std::max(1, std::min(physicalWidth / minCanvasWidth, physicalHeight / minCanvasHeight));
    }
    static_assert(AutoScale(1024, 768) == 2 && AutoScale(640, 480) == 1 && AutoScale(1280, 720) == 2);
    static_assert(AutoScale(1920, 1080) == 3 && AutoScale(720, 720) == 1 && AutoScale(320, 240) == 1);

    // Menus own their entire content area; HUD windows only exist in park contexts.
    enum class Context
    {
        park,
        construction,
        picker,
        inspection,
        dashboard,
        review,
        pause
    };
    constexpr bool ShowsHud(Context context)
    {
        return context == Context::park || context == Context::construction;
    }
    constexpr int ContentHeight(ScreenSize screen, bool hud = false)
    {
        return std::max(0, screen.height - actionHeight - margin - (hud ? toolOrigin.y : menuOrigin.y));
    }
    inline ScreenCoordsXY ContextOrigin(ScreenSize screen, ScreenCoordsXY preferred, ScreenSize size)
    {
        // Native feedback stays inside the park safe area, even after resizing.
        return { std::clamp(preferred.x, margin, std::max(margin, screen.width - margin - size.width)),
                 std::clamp(
                     preferred.y, toolOrigin.y, std::max(toolOrigin.y, screen.height - actionHeight - gutter - size.height)) };
    }
    inline ScreenCoordsXY ContextAnchor(ScreenSize screen, ScreenSize size, bool endpointConstruction = false)
    {
        return ContextOrigin(
            screen,
            endpointConstruction ? ScreenCoordsXY{ screen.width - margin - size.width, screen.height }
                                 : ScreenCoordsXY{ screen.width / 2 + 32, screen.height / 2 + 32 },
            size);
    }
    struct HudCells
    {
        int width, cashEnd, dateEnd, guestsEnd, clockEnd;
    };
    constexpr HudCells HudLayout(ScreenSize screen, bool clock = false)
    {
        const int width = std::max(0, screen.width - 2 * margin);
        // Keep cash dominant; date/count are secondary. Last cell is always simulation state.
        // With the clock, each cell gives up a little so the time and battery fit before it.
        if (clock)
            return { width, width * 31 / 100, width * 55 / 100, width * 71 / 100, width * 89 / 100 };
        return { width, width * 38 / 100, width * 69 / 100, width * 89 / 100, width * 89 / 100 };
    }
} // namespace OpenRCT2::Ui::Handheld::Layout
