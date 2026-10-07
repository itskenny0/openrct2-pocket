// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ModernText.h"

#include <cmath>
#include <openrct2-ui/interface/Window.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/ColourMap.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>

namespace OpenRCT2::Ui::Handheld
{
    inline constexpr int32_t kPanelRowPitch = 32;

    // Stable entries in OpenRCT2's default palette; never use cycling water colours.
    // The palette has no continuous RGB navy ramp, so combine ink blue with slate edges.
    namespace Theme
    {
        inline constexpr Drawing::PaletteIndex ink = static_cast<Drawing::PaletteIndex>(118);
        inline constexpr Drawing::PaletteIndex body = static_cast<Drawing::PaletteIndex>(10);
        inline constexpr Drawing::PaletteIndex raised = static_cast<Drawing::PaletteIndex>(119);
        inline constexpr Drawing::PaletteIndex edge = static_cast<Drawing::PaletteIndex>(14);
        inline constexpr Drawing::PaletteIndex highlight = static_cast<Drawing::PaletteIndex>(17);
        inline constexpr Drawing::PaletteIndex gold = static_cast<Drawing::PaletteIndex>(53);
        inline constexpr Drawing::PaletteIndex goldLight = static_cast<Drawing::PaletteIndex>(55);
        inline constexpr Drawing::PaletteIndex goldShadow = static_cast<Drawing::PaletteIndex>(51);
        inline constexpr Drawing::PaletteIndex text = static_cast<Drawing::PaletteIndex>(20);
    } // namespace Theme

    inline void Fill(Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size, Drawing::PaletteIndex colour)
    {
        if (size.width > 0 && size.height > 0)
            Drawing::Rectangle::fill(rt, { pos, pos + ScreenCoordsXY{ size.width - 1, size.height - 1 } }, colour);
    }

    // Five-pixel rounded corners and quiet single-line edges, at native resolution.
    inline void DrawCard(Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size, bool selected = false)
    {
        const auto edge = selected ? Theme::goldShadow : Theme::edge;
        const auto face = selected ? Theme::gold : Theme::ink;
        Fill(rt, pos + ScreenCoordsXY{ 5, 0 }, { size.width - 10, size.height }, edge);
        Fill(rt, pos + ScreenCoordsXY{ 2, 2 }, { size.width - 4, size.height - 4 }, edge);
        Fill(rt, pos + ScreenCoordsXY{ 0, 5 }, { size.width, size.height - 10 }, edge);
        Fill(rt, pos + ScreenCoordsXY{ 5, 1 }, { size.width - 10, size.height - 2 }, face);
        Fill(rt, pos + ScreenCoordsXY{ 3, 3 }, { size.width - 6, size.height - 6 }, face);
        Fill(rt, pos + ScreenCoordsXY{ 1, 5 }, { size.width - 2, size.height - 10 }, face);
        Fill(rt, pos + ScreenCoordsXY{ 5, 1 }, { size.width - 10, 1 }, selected ? Theme::goldLight : Theme::highlight);
        if (selected)
            Fill(rt, pos + ScreenCoordsXY{ 3, 3 }, { size.width - 6, 4 }, static_cast<Drawing::PaletteIndex>(54));
    }

    inline void DrawSurface(Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size)
    {
        DrawCard(rt, pos, size);
    }

    enum class PanelIcon : uint8_t
    {
        play,
        terrain,
        save,
        exit,
        folder,
        back,
        raise,
        lower
    };

    inline void DrawIcon(Drawing::RenderTarget& rt, ScreenCoordsXY pos, PanelIcon icon, bool selected = false)
    {
        // Native 16px pixel glyphs, independent of the user's RCT2 asset installation.
        static constexpr uint16_t glyphs[][16] = {
            { 0, 0, 0x0800, 0x0C00, 0x0E00, 0x0F00, 0x0F80, 0x0FC0, 0x0F80, 0x0F00, 0x0E00, 0x0C00, 0x0800, 0, 0, 0 },
            { 0, 0, 0, 0x0080, 0x01C0, 0x03E0, 0x07F0, 0x0FF8, 0x1FFC, 0x3FFE, 0x7FFF, 0x3FFE, 0x1FFC, 0, 0, 0 },
            { 0, 0, 0x3FFC, 0x2424, 0x2424, 0x2424, 0x27E4, 0x2004, 0x2004, 0x27E4, 0x2424, 0x2424, 0x3FFC, 0, 0, 0 },
            { 0, 0, 0x3F00, 0x2100, 0x2100, 0x2120, 0x2130, 0x27F8, 0x2130, 0x2120, 0x2100, 0x2100, 0x3F00, 0, 0, 0 },
            { 0, 0, 0, 0x3C00, 0x7FF8, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x7FF8, 0, 0, 0, 0 },
            { 0, 0, 0, 0x0200, 0x0600, 0x0C00, 0x1FF8, 0x3FF8, 0x1FF8, 0x0C00, 0x0600, 0x0200, 0, 0, 0, 0 },
            { 0, 0, 0, 0x0180, 0x03C0, 0x07E0, 0x0DB0, 0x1998, 0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 0, 0, 0 },
            { 0, 0, 0, 0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 0x1998, 0x0DB0, 0x07E0, 0x03C0, 0x0180, 0, 0, 0 }
        };
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                if (glyphs[static_cast<size_t>(icon)][y] & (0x8000 >> x))
                    Fill(
                        rt, pos + ScreenCoordsXY{ x, y }, { 1, 1 },
                        selected ? Theme::ink : (icon == PanelIcon::save ? Theme::highlight : Theme::goldLight));
    }

    inline void DrawPanelHeading(Drawing::RenderTarget& rt, const Window& window, const std::string& title)
    {
        DrawSurface(rt, window.windowPos, { window.width, window.height });
        Fill(rt, window.windowPos + ScreenCoordsXY{ 5, 3 }, { window.width - 10, 29 }, Theme::ink);
        Fill(rt, window.windowPos + ScreenCoordsXY{ 13, 11 }, { 3, 14 }, Theme::gold);
        Fill(rt, window.windowPos + ScreenCoordsXY{ 10, 35 }, { window.width - 20, 1 }, Theme::edge);
        DrawModernEllipsised(
            rt, window.windowPos + ScreenCoordsXY{ 24, 13 }, window.width - 40, title, { Drawing::Colour::white });
    }

    inline void DrawIconRow(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, int32_t width, const std::string& label, bool selected, PanelIcon icon)
    {
        if (selected)
            DrawCard(rt, pos, { width, 28 }, true);
        else
            Fill(rt, pos + ScreenCoordsXY{ 5, 27 }, { width - 10, 1 }, Theme::raised);
        if (icon == PanelIcon::terrain)
            GfxDrawSprite(rt, ImageId(SPR_G2_TAB_LAND), pos + ScreenCoordsXY{ 3, 0 });
        else
            DrawIcon(rt, pos + ScreenCoordsXY{ 8, 6 }, icon, selected);
        DrawModernEllipsised(
            rt, pos + ScreenCoordsXY{ 39, 9 }, width - 53, (selected ? "{BLACK}" : "{WHITE}") + label,
            { Drawing::Colour::white });
        if (selected)
            DrawModernText(rt, pos + ScreenCoordsXY{ width - 11, 9 }, "{BLACK}>", { Drawing::Colour::white });
    }

    inline void DrawPanelRow(
        Drawing::RenderTarget& rt, const Window& window, int32_t y, const std::string& label, bool selected)
    {
        DrawIconRow(rt, { window.windowPos.x + 10, y - 8 }, window.width - 20, label, selected, PanelIcon::play);
    }

    inline void DrawCommand(Drawing::RenderTarget& rt, ScreenCoordsXY pos, int32_t keyWidth, const char* key, const char* label)
    {
        const auto badgeWidth = keyWidth == 17 ? 22 : keyWidth + 4;
        const auto top = pos.y - 3;
        if (keyWidth == 17)
        {
            // Circular face-button badges with antialiased edges in the native palette.
            for (int y = 0; y < 20; ++y)
                for (int x = 0; x < 22; ++x)
                {
                    const float dx = x - 10.5f, dy = y - 9.5f;
                    const float distance = std::sqrt(dx * dx + dy * dy);
                    if (distance < 9.8f)
                        Fill(rt, { pos.x + x, top + y }, { 1, 1 }, distance > 8.8f ? Theme::highlight : Theme::text);
                }
        }
        else
        {
            DrawCard(rt, { pos.x, top }, { badgeWidth, 20 });
            Fill(rt, { pos.x + 3, top + 2 }, { badgeWidth - 6, 16 }, Theme::text);
        }
        DrawModernText(
            rt, { pos.x + badgeWidth / 2, pos.y }, "{BLACK}" + std::string(key),
            { Drawing::Colour::white, TextAlignment::centre });
        DrawModernText(rt, { pos.x + badgeWidth + 5, pos.y }, label, { Drawing::Colour::white });
    }

    inline void DrawPanelCommands(Drawing::RenderTarget& rt, const Window& window, const char* backLabel = "Back")
    {
        const auto pos = window.windowPos + ScreenCoordsXY{ 1, window.height - 29 };
        Fill(rt, pos, { window.width - 2, 28 }, Theme::raised);
        Fill(rt, pos, { window.width - 2, 1 }, Theme::edge);
        const auto y = pos.y + 8;
        DrawCommand(rt, { pos.x + 12, y }, 38, "D-pad", "Choose");
        DrawCommand(rt, { pos.x + 139, y }, 17, "A", "Select");
        DrawCommand(rt, { pos.x + 238, y }, 17, "B", backLabel);
    }

    inline void DrawTabs(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, int32_t width, const std::vector<std::string>& labels, size_t selected)
    {
        if (labels.empty())
            return;
        const auto tabWidth = width / static_cast<int32_t>(labels.size());
        for (size_t i = 0; i < labels.size(); ++i)
        {
            const auto tabPos = pos + ScreenCoordsXY{ static_cast<int32_t>(i) * tabWidth, 0 };
            DrawCard(rt, tabPos, { tabWidth - 3, 28 }, i == selected);
            DrawModernEllipsised(
                rt, tabPos + ScreenCoordsXY{ 9, 9 }, tabWidth - 21, (i == selected ? "{BLACK}" : "{WHITE}") + labels[i],
                { Drawing::Colour::white });
        }
    }

    struct MenuPresentation
    {
        std::string title;
        std::string section;
        std::vector<std::string> tabs;
        size_t selectedTab{};
        std::string detailTitle;
        std::string detailSubtitle;
        std::vector<std::pair<std::string, std::string>> stats;
        std::vector<std::string> help;
        std::vector<std::string> rows;
        std::vector<PanelIcon> icons;
        std::string status;
        size_t selected{};
    };

    inline ScreenSize OverlaySize(size_t rows)
    {
        return { 244, 86 + static_cast<int32_t>(rows) * kPanelRowPitch };
    }

    inline void DrawOverlayCommands(Drawing::RenderTarget& rt, const Window& window)
    {
        const auto p = window.windowPos + ScreenCoordsXY{ 12, window.height - 23 };
        Fill(rt, window.windowPos + ScreenCoordsXY{ 10, window.height - 29 }, { window.width - 20, 1 }, Theme::edge);
        DrawCommand(rt, p, 17, "A", "Select");
        DrawCommand(rt, p + ScreenCoordsXY{ 92, 0 }, 17, "B", "Back");
    }

    inline void DrawOverlayShell(Drawing::RenderTarget& rt, const Window& window, const MenuPresentation& model)
    {
        DrawPanelHeading(rt, window, model.title);
        const auto p = window.windowPos;
        for (size_t i = 0; i < model.rows.size(); ++i)
            DrawIconRow(
                rt, p + ScreenCoordsXY{ 10, 43 + static_cast<int32_t>(i) * kPanelRowPitch }, window.width - 20, model.rows[i],
                i == model.selected, i < model.icons.size() ? model.icons[i] : PanelIcon::folder);
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 13, window.height - 45 }, window.width - 26, model.status, { Drawing::Colour::white });
        DrawOverlayCommands(rt, window);
    }

    inline constexpr ScreenSize kTerrainOverlaySize{ 184, 164 };

    inline void DrawTerrainOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos, bool raise)
    {
        Window window;
        window.windowPos = pos;
        window.width = kTerrainOverlaySize.width;
        window.height = kTerrainOverlaySize.height;
        DrawPanelHeading(rt, window, "Terrain");
        DrawIconRow(rt, pos + ScreenCoordsXY{ 10, 44 }, window.width - 20, "Raise land", raise, PanelIcon::raise);
        DrawIconRow(rt, pos + ScreenCoordsXY{ 10, 76 }, window.width - 20, "Lower land", !raise, PanelIcon::lower);
        DrawModernText(rt, pos + ScreenCoordsXY{ 13, 115 }, "Single tile / centre cursor", { Drawing::Colour::white });
        DrawCommand(rt, pos + ScreenCoordsXY{ 13, 141 }, 17, "Y", "Switch tool");
    }

    inline void DrawGameplayFooter(Drawing::RenderTarget& rt, ScreenSize screen, const std::string& status, bool terrain)
    {
        DrawSurface(rt, { 0, screen.height - 34 }, { screen.width, 34 });
        const auto y = screen.height - 19;
        if (terrain)
        {
            DrawCommand(rt, { 8, y }, 17, "A", "Edit");
            DrawCommand(rt, { 112, y }, 17, "Y", "Raise/lower");
            DrawCommand(rt, { 264, y }, 17, "B", "Done");
            DrawCommand(rt, { 376, y }, 38, "D-pad", "Move");
        }
        else
        {
            DrawCommand(rt, { 8, y }, 17, "A", "Inspect");
            DrawCommand(rt, { 126, y }, 17, "B", "Back");
            DrawCommand(rt, { 244, y }, 28, "SEL", "Build");
            DrawCommand(rt, { 370, y }, 38, "START", "Park");
        }
        DrawModernEllipsised(rt, { 8, screen.height - 31 }, screen.width - 16, "{YELLOW}" + status, { Drawing::Colour::white });
    }

    inline void DrawPathOverlay(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& surface, bool queue, bool remove)
    {
        Window w;
        w.windowPos = pos;
        w.width = kTerrainOverlaySize.width;
        w.height = kTerrainOverlaySize.height;
        DrawPanelHeading(rt, w, "Paths");
        DrawIconRow(
            rt, pos + ScreenCoordsXY{ 10, 44 }, w.width - 20,
            remove      ? "Remove path"
                : queue ? "Queue path"
                        : "Normal path",
            true, remove ? PanelIcon::back : PanelIcon::folder);
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 13, 82 }, w.width - 26, surface, { Drawing::Colour::white });
        DrawModernText(rt, pos + ScreenCoordsXY{ 13, 107 }, "Follows terrain slope", { Drawing::Colour::white });
        DrawCommand(rt, pos + ScreenCoordsXY{ 13, 139 }, 17, "Y", "Change surface");
    }
    inline void DrawPathFooter(Drawing::RenderTarget& rt, ScreenSize screen, const std::string& status)
    {
        DrawSurface(rt, { 0, screen.height - 34 }, { screen.width, 34 });
        DrawModernEllipsised(rt, { 8, screen.height - 31 }, screen.width - 16, "{YELLOW}" + status, { Drawing::Colour::white });
        const auto y = screen.height - 19;
        DrawCommand(rt, { 8, y }, 17, "A", "Apply");
        DrawCommand(rt, { 104, y }, 17, "X", "Place/remove");
        DrawCommand(rt, { 258, y }, 17, "B", "Done");
        DrawCommand(rt, { 369, y }, 38, "D-pad", "Move");
    }

    inline void DrawMapFocus(Drawing::RenderTarget& rt, ScreenCoordsXY centre)
    {
        GfxDrawLine(rt, { centre + ScreenCoordsXY{ -7, -1 }, centre + ScreenCoordsXY{ 7, -1 } }, Drawing::PaletteIndex::pi10);
        GfxDrawLine(rt, { centre + ScreenCoordsXY{ -7, 1 }, centre + ScreenCoordsXY{ 7, 1 } }, Drawing::PaletteIndex::pi10);
        GfxDrawLine(rt, { centre + ScreenCoordsXY{ -1, -7 }, centre + ScreenCoordsXY{ -1, 7 } }, Drawing::PaletteIndex::pi10);
        GfxDrawLine(rt, { centre + ScreenCoordsXY{ 1, -7 }, centre + ScreenCoordsXY{ 1, 7 } }, Drawing::PaletteIndex::pi10);
        GfxDrawLine(rt, { centre + ScreenCoordsXY{ -6, 0 }, centre + ScreenCoordsXY{ 6, 0 } }, Drawing::PaletteIndex::pi21);
        GfxDrawLine(rt, { centre + ScreenCoordsXY{ 0, -6 }, centre + ScreenCoordsXY{ 0, 6 } }, Drawing::PaletteIndex::pi21);
    }

    // One layout for pause, tool selection, save/load and confirmation pages.
    // The renderer is also called directly by the preview tool: no separate mockup UI.
    inline void DrawMenuShell(Drawing::RenderTarget& rt, const Window& window, const MenuPresentation& model)
    {
        DrawPanelHeading(rt, window, model.title);
        const auto p = window.windowPos;
        if (model.tabs.empty())
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 14, 46 }, 204, "{YELLOW}" + model.section, { Drawing::Colour::white });
        else
            DrawTabs(rt, p + ScreenCoordsXY{ 10, 44 }, 221, model.tabs, model.selectedTab);
        for (size_t i = 0; i < model.rows.size(); ++i)
            DrawIconRow(
                rt, p + ScreenCoordsXY{ 10, (model.tabs.empty() ? 64 : 82) + static_cast<int32_t>(i) * kPanelRowPitch }, 218,
                model.rows[i], i == model.selected, i < model.icons.size() ? model.icons[i] : PanelIcon::folder);
        DrawCard(rt, p + ScreenCoordsXY{ 238, 44 }, { window.width - 248, 150 });
        GfxDrawSprite(rt, ImageId(SPR_G2_TAB_TREE), p + ScreenCoordsXY{ 250, 50 });
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 289, 59 }, window.width - 305, model.detailTitle, { Drawing::Colour::white });
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 250, 84 }, window.width - 272, model.detailSubtitle, { Drawing::Colour::white });
        Fill(rt, p + ScreenCoordsXY{ 250, 105 }, { window.width - 272, 1 }, Theme::edge);
        for (size_t i = 0; i < model.stats.size() && i < 3; ++i)
        {
            const auto y = 116 + static_cast<int32_t>(i) * 23;
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 250, y }, 100, model.stats[i].first, { Drawing::Colour::white });
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 357, y }, window.width - 371, "{YELLOW}" + model.stats[i].second,
                { Drawing::Colour::white });
        }
        DrawCard(rt, p + ScreenCoordsXY{ 238, 202 }, { window.width - 248, 87 });
        DrawModernText(rt, p + ScreenCoordsXY{ 250, 214 }, "{YELLOW}CONTROLS", { Drawing::Colour::white });
        for (size_t i = 0; i < model.help.size() && i < 3; ++i)
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 250, 237 + static_cast<int32_t>(i) * 16 }, window.width - 272, model.help[i],
                { Drawing::Colour::white });
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 14, window.height - 47 }, window.width - 28, model.status, { Drawing::Colour::white });
        DrawPanelCommands(rt, window);
    }
} // namespace OpenRCT2::Ui::Handheld
