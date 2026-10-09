// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConceptStyle.h"
#include "ModernText.h"

#include <array>
#include <cmath>
#include <openrct2-ui/interface/Window.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/drawing/ColourMap.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/Line.h>
#include <openrct2/drawing/PaletteIndex.h>
#include <openrct2/drawing/Rectangle.h>
#include <openrct2/drawing/Text.h>
#include <string_view>

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
        lower,
        path,
        remove,
        tree
    };

    inline void DrawIcon(Drawing::RenderTarget& rt, ScreenCoordsXY pos, PanelIcon icon, bool selected = false)
    {
        if (icon == PanelIcon::tree)
        {
            Concept::DrawIcon(rt, pos, Concept::Icon::tree, selected);
            return;
        }
        if (icon == PanelIcon::remove)
        {
            Concept::DrawIcon(rt, pos, Concept::Icon::remove, selected);
            return;
        }
        if (icon == PanelIcon::path)
        {
            GfxDrawSprite(rt, ImageId(SPR_G2_BUTTON_FOOTPATH), pos + ScreenCoordsXY{ -5, -6 });
            return;
        }
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
        const bool dpad = std::string_view(key) == "D-pad";
        const auto badgeWidth = dpad || keyWidth == 17 ? 22 : keyWidth + 4;
        const auto top = pos.y - 3;
        if (dpad)
        {
            // Compact physical D-pad silhouette; no text or rectangular keycap.
            Fill(rt, { pos.x + 8, top }, { 6, 20 }, Theme::highlight);
            Fill(rt, { pos.x + 1, top + 7 }, { 20, 6 }, Theme::highlight);
            Fill(rt, { pos.x + 8, top + 1 }, { 6, 18 }, Theme::text);
            Fill(rt, { pos.x + 2, top + 7 }, { 18, 6 }, Theme::text);
            Fill(rt, { pos.x + 9, top + 8 }, { 4, 4 }, Theme::edge);
        }
        else if (keyWidth == 17)
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
        if (!dpad)
            DrawButtonLabel(rt, { pos.x, top }, { badgeWidth, 20 }, key);
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
        const size_t first = model.selected >= 8 ? model.selected - 7 : 0;
        for (size_t i = first; i < model.rows.size() && i < first + 8; ++i)
            DrawIconRow(
                rt, p + ScreenCoordsXY{ 10, 43 + static_cast<int32_t>(i - first) * kPanelRowPitch }, window.width - 20,
                model.rows[i], i == model.selected, i < model.icons.size() ? model.icons[i] : PanelIcon::folder);
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 13, window.height - 45 }, window.width - 26, model.status, { Drawing::Colour::white });
        DrawOverlayCommands(rt, window);
    }

    inline void DrawInformationOverlay(Drawing::RenderTarget& rt, const Window& w, const MenuPresentation& model)
    {
        DrawPanelHeading(rt, w, model.title);
        const auto p = w.windowPos;
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 13, 44 }, w.width - 26, "{YELLOW}" + model.detailTitle, { Drawing::Colour::white });
        for (size_t i = 0; i < model.stats.size() && i < 6; ++i)
        {
            const int y = 69 + static_cast<int>(i) * 22;
            DrawModernEllipsised(rt, p + ScreenCoordsXY{ 13, y }, 154, model.stats[i].first, { Drawing::Colour::white });
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ 177, y }, w.width - 190, model.stats[i].second, { Drawing::Colour::white });
        }
        for (size_t i = 0; i < model.rows.size(); ++i)
            DrawIconRow(
                rt, p + ScreenCoordsXY{ 10, w.height - 62 - static_cast<int>(model.rows.size() - i) * 32 }, w.width - 20,
                model.rows[i], i == model.selected, PanelIcon::play);
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ 13, w.height - 45 }, w.width - 26, model.status, { Drawing::Colour::white });
        DrawOverlayCommands(rt, w);
    }

    inline constexpr int kActionStripHeight = Layout::actionHeight;
    inline constexpr ScreenSize kTerrainOverlaySize{ 164, 278 };
    inline constexpr ScreenSize kContextSize{ 160, 42 };

    inline void DrawTerrainOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos, bool raise, bool water = false)
    {
        using namespace Concept;
        Box(rt, pos, { kRailWidth, 174 });
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, water ? "Water" : "Terrain");
        Row(rt, pos + ScreenCoordsXY{ 8, 34 }, water ? "Raise water" : "Raise land", Icon::rise, raise);
        Row(rt, pos + ScreenCoordsXY{ 8, 62 }, water ? "Lower water" : "Lower land", Icon::lower, !raise);
        Rule(rt, pos + ScreenCoordsXY{ 8, 99 }, 148);
        Section(rt, pos + ScreenCoordsXY{ 10, 109 }, "Brush");
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 132 }, "Single tile");
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 153 }, "{MUTED}1 x 1");
    }

    inline void DrawSurfaceOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& name)
    {
        using namespace Concept;
        Box(rt, pos, { kRailWidth, 174 });
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, "Land surface");
        Section(rt, pos + ScreenCoordsXY{ 10, 35 }, "Surface");
        Row(rt, pos + ScreenCoordsXY{ 8, 54 }, name, Icon::ground, true);
        Rule(rt, pos + ScreenCoordsXY{ 8, 99 }, 148);
        Section(rt, pos + ScreenCoordsXY{ 10, 109 }, "Brush");
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 132 }, "Single tile");
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 153 }, "{MUTED}1 x 1");
    }

    inline void DrawTreeOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& name, bool available)
    {
        using namespace Concept;
        Box(rt, pos, { kRailWidth, 146 });
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, "Trees");
        Section(rt, pos + ScreenCoordsXY{ 10, 35 }, "Tree");
        Row(rt, pos + ScreenCoordsXY{ 8, 54 }, name, Icon::tree, available);
        Rule(rt, pos + ScreenCoordsXY{ 8, 99 }, 148);
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 116 }, "{MUTED}Ground level");
    }

    inline void DrawShopOverlay(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& name, bool available, Direction direction)
    {
        using namespace Concept;
        Box(rt, pos, { kRailWidth, 146 });
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, "Shops / Facilities");
        Section(rt, pos + ScreenCoordsXY{ 10, 35 }, "Building");
        Row(rt, pos + ScreenCoordsXY{ 8, 54 }, name, Icon::path, available);
        Rule(rt, pos + ScreenCoordsXY{ 8, 99 }, 148);
        DrawModernText(
            rt, pos + ScreenCoordsXY{ 12, 116 },
            "{MUTED}Rotation " + std::string(std::array{ "NW", "SW", "SE", "NE" }[direction & 3]));
    }

    inline void DrawTrackColourOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos, uint8_t scheme, bool target)
    {
        using namespace Concept;
        Box(rt, pos, { kRailWidth, 146 });
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, "Track colours");
        Section(rt, pos + ScreenCoordsXY{ 10, 35 }, "Scheme");
        Row(rt, pos + ScreenCoordsXY{ 8, 54 }, "Scheme " + std::to_string(scheme + 1), Icon::path, target);
        Rule(rt, pos + ScreenCoordsXY{ 8, 99 }, 148);
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 116 }, "{MUTED}One track piece");
    }
    inline void DrawSceneryRemovalOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& name, bool target)
    {
        using namespace Concept;
        Box(rt, pos, { kRailWidth, 146 });
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, "Remove scenery");
        Section(rt, pos + ScreenCoordsXY{ 10, 35 }, "Target");
        Row(rt, pos + ScreenCoordsXY{ 8, 54 }, name, Icon::remove, target);
        Rule(rt, pos + ScreenCoordsXY{ 8, 99 }, 148);
        DrawModernText(rt, pos + ScreenCoordsXY{ 12, 116 }, "{MUTED}One object");
    }

    inline void DrawActionStrip(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        const ScreenCoordsXY p{ 0, screen.height - kActionStripHeight };
        Fill(rt, p, { screen.width, kActionStripHeight }, static_cast<Drawing::PaletteIndex>(PocketPalette::panel));
        Concept::Rule(rt, p, screen.width);
    }

    inline void DrawShopFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 8, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 115, y }, 17, "A", "Place");
        DrawCommand(rt, { 207, y }, 17, "Y", "Shops");
        DrawCommand(rt, { 299, y }, 30, "SEL", "Turn");
        DrawCommand(rt, { 420, y }, 17, "B", "Back");
    }

    inline void DrawTreeFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 140, y }, 17, "A", "Place");
        DrawCommand(rt, { 268, y }, 17, "Y", "Trees");
        DrawCommand(rt, { 396, y }, 17, "B", "Back");
    }

    inline void DrawSceneryRemovalFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 184, y }, 17, "A", "Remove");
        DrawCommand(rt, { 396, y }, 17, "B", "Back");
    }

    inline void DrawTrackColourFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 140, y }, 17, "A", "Paint");
        DrawCommand(rt, { 268, y }, 17, "Y", "Scheme");
        DrawCommand(rt, { 396, y }, 17, "B", "Back");
    }
    inline void DrawGameplayFooter(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::string&, bool terrain, bool surface = false)
    {
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        if (terrain)
        {
            DrawCommand(rt, { 12, y }, 38, "D-pad", "Move");
            DrawCommand(rt, { 140, y }, 17, "A", surface ? "Paint" : "Edit");
            DrawCommand(rt, { 268, y }, 17, "Y", surface ? "Surface" : "Tool");
            DrawCommand(rt, { 396, y }, 17, "B", "Back");
        }
        else
        {
            DrawCommand(rt, { 12, y }, 17, "A", "Inspect");
            DrawCommand(rt, { 140, y }, 17, "B", "Back");
            DrawCommand(rt, { 268, y }, 28, "SEL", "Build");
            DrawCommand(rt, { 396, y }, 38, "START", "Park");
        }
    }

    inline void DrawPathOverlay(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& surface, bool queue, bool remove,
        const std::string& placement = "Ground / auto slope")
    {
        using namespace Concept;
        Box(rt, pos, kTerrainOverlaySize);
        Heading(rt, pos + ScreenCoordsXY{ 10, 12 }, "Path type");
        Row(rt, pos + ScreenCoordsXY{ 8, 34 }, queue ? "Queue path" : "Normal path", queue ? Icon::queue : Icon::path, !remove);
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 12, 69 }, 140, "{MUTED}" + surface);
        Rule(rt, pos + ScreenCoordsXY{ 8, 91 }, 148);
        Section(rt, pos + ScreenCoordsXY{ 10, 103 }, "Slope");
        const auto slash = placement.find(" / ");
        const auto slope = slash == std::string::npos ? "Follow ground" : placement.substr(slash + 3);
        Row(rt, pos + ScreenCoordsXY{ 8, 125 },
            remove                      ? "Existing path"
                : slope == "auto slope" ? "Auto slope"
                                        : slope,
            slope.starts_with("Rise") ? Icon::slope : Icon::path, !remove);
        DrawModernEllipsised(
            rt, pos + ScreenCoordsXY{ 12, 160 }, 140,
            "{MUTED}"
                + (remove                           ? std::string("Uses tile height")
                       : slash == std::string::npos ? placement
                                                    : placement.substr(0, slash)));
        Rule(rt, pos + ScreenCoordsXY{ 8, 181 }, 148);
        Section(rt, pos + ScreenCoordsXY{ 10, 193 }, "Options");
        Row(rt, pos + ScreenCoordsXY{ 8, 215 }, "Path options", Icon::options, false);
        Row(rt, pos + ScreenCoordsXY{ 8, 243 }, "Remove", Icon::remove, remove);
    }

    inline void DrawPathFooter(Drawing::RenderTarget& rt, ScreenSize screen, const std::string&, bool remove = false)
    {
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 8, y }, 38, "D-pad", "Move");
        DrawCommand(rt, { 91, y }, 17, "A", remove ? "Remove" : "Place");
        DrawCommand(rt, { 180, y }, 17, "X", remove ? "Place" : "Remove");
        DrawCommand(rt, { 271, y }, 17, "Y", "Surface");
        DrawCommand(rt, { 358, y }, 28, "SEL", "Options");
        DrawCommand(rt, { 450, y }, 17, "B", "Back");
    }

    inline void DrawCostBadge(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& amount)
    {
        Concept::Box(rt, pos, { 160, 26 });
        DrawModernText(rt, pos + ScreenCoordsXY{ 8, 8 }, "{MUTED}Cost:");
        DrawStrongText(rt, pos + ScreenCoordsXY{ 43, 8 }, "{POSITIVE}" + amount, 109);
    }

    inline void DrawContextMessage(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& text)
    {
        // Two short lines preserve feedback without reserving a permanent status row.
        Concept::Box(rt, pos, kContextSize);
        auto split = text.size();
        while (split > 0 && TextWidth(text.substr(0, split)) > 140)
            --split;
        if (split < text.size())
        {
            const auto space = text.rfind(' ', split);
            if (space != std::string::npos && space > 0)
                split = space;
        }
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 10, 7 }, 140, "{WHITE}" + text.substr(0, split));
        if (split < text.size())
            DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 10, 24 }, 140, "{MUTED}" + text.substr(split));
    }

    inline void DrawPathOptions(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::vector<std::string>& rows, size_t selected,
        const std::string& status)
    {
        using namespace Concept;
        const auto p = kMenuOrigin;
        Box(rt, p, { kRailWidth, Layout::ContentHeight(screen) });
        Heading(rt, p + ScreenCoordsXY{ 10, 12 }, "Path options");
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, 32 }, 144, "{MUTED}" + status);
        if (rows.size() > 7)
        {
            const Icon icons[] = {Icon::play, Icon::options, Icon::rise, Icon::lower,
                                  Icon::slope, Icon::options, Icon::path, Icon::options};
            for (size_t i = 0; i < rows.size() && i < 8; ++i)
                Row(rt, p + ScreenCoordsXY{8, 52 + static_cast<int32_t>(i) * 32}, rows[i], icons[i], i == selected);
        }
        else
        {
        const int rowY[] = { 52, 108, 136, 164, 220, 248, 304 };
        const Icon icons[] = { Icon::play, Icon::ground, Icon::rise, Icon::lower, Icon::slope, Icon::options, Icon::path };
        Section(rt, p + ScreenCoordsXY{ 10, 87 }, "Height");
        Section(rt, p + ScreenCoordsXY{ 10, 199 }, "Slope");
        Section(rt, p + ScreenCoordsXY{ 10, 283 }, "Surface");
        for (size_t i = 0; i < rows.size() && i < 7; ++i)
            Row(rt, p + ScreenCoordsXY{ 8, rowY[i] }, rows[i], icons[i], i == selected);
        }
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", "Choose");
        DrawCommand(rt, { 184, y }, 17, "A", "Select");
        DrawCommand(rt, { 356, y }, 17, "B", "Back");
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
        const size_t first = model.selected >= 8 ? model.selected - 7 : 0;
        for (size_t i = first; i < model.rows.size() && i < first + 8; ++i)
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
