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
    inline constexpr int actionHeight = 44;
    inline constexpr ScreenCoordsXY menuOrigin{ margin, margin };
    inline constexpr ScreenCoordsXY toolOrigin{ margin, hudTop + hudHeight + gutter };

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
        int width, cashEnd, dateEnd, guestsEnd;
    };
    constexpr HudCells HudLayout(ScreenSize screen)
    {
        const int width = std::max(0, screen.width - 2 * margin);
        // Keep cash dominant; date/count are secondary. Last cell is always simulation state.
        return { width, width * 38 / 100, width * 69 / 100, width * 89 / 100 };
    }
} // namespace OpenRCT2::Ui::Handheld::Layout
