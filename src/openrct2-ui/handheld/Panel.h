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
#include <sstream>

namespace OpenRCT2::Ui::Handheld
{
    inline constexpr int32_t kPanelRowPitch = 32;

    // Stable entries in OpenRCT2's default palette; never use cycling water colours.
    // The palette has no continuous RGB navy ramp, so combine ink blue with slate edges.
    namespace Theme
    {
        inline constexpr Drawing::PaletteIndex ink = static_cast<Drawing::PaletteIndex>(PocketPalette::panel);
        inline constexpr Drawing::PaletteIndex body = static_cast<Drawing::PaletteIndex>(10);
        inline constexpr Drawing::PaletteIndex raised = static_cast<Drawing::PaletteIndex>(13);
        inline constexpr Drawing::PaletteIndex edge = static_cast<Drawing::PaletteIndex>(14);
        inline constexpr Drawing::PaletteIndex highlight = static_cast<Drawing::PaletteIndex>(17);
        inline constexpr Drawing::PaletteIndex gold = static_cast<Drawing::PaletteIndex>(PocketPalette::selection);
        inline constexpr Drawing::PaletteIndex goldLight = static_cast<Drawing::PaletteIndex>(20);
        inline constexpr Drawing::PaletteIndex goldShadow = static_cast<Drawing::PaletteIndex>(16);
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
        tree,
        none,
        largeScenery,
        walls,
        furniture
    };

    inline void DrawIcon(Drawing::RenderTarget& rt, ScreenCoordsXY pos, PanelIcon icon, bool selected = false)
    {
        if (icon == PanelIcon::none)
            return;
        if (icon == PanelIcon::largeScenery || icon == PanelIcon::walls || icon == PanelIcon::furniture)
        {
            const auto sprite = icon == PanelIcon::largeScenery ? SPR_G2_BUTTON_LARGE_SCENERY
                : icon == PanelIcon::walls ? SPR_G2_BUTTON_WALLS : SPR_G2_BUTTON_PATH_ADDITIONS;
            GfxDrawSprite(rt, ImageId(sprite), pos + ScreenCoordsXY{ -5, -6 });
            return;
        }
        if (icon == PanelIcon::tree)
        {
            GfxDrawSprite(rt, ImageId(SPR_G2_BUTTON_TREES), pos + ScreenCoordsXY{-5, -6});
            return;
        }
        if (icon == PanelIcon::folder)
        {
            GfxDrawSprite(rt, ImageId(SPR_G2_FOLDER), pos);
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
            rt, pos + ScreenCoordsXY{ icon == PanelIcon::none ? 12 : 39, 9 },
            width - (icon == PanelIcon::none ? 26 : 53), (selected ? "{BLACK}" : "{WHITE}") + label,
            { Drawing::Colour::white });
        if (selected)
            DrawModernText(rt, pos + ScreenCoordsXY{ width - 11, 9 }, "{BLACK}>", { Drawing::Colour::white });
    }

    inline void DrawPanelRow(
        Drawing::RenderTarget& rt, const Window& window, int32_t y, const std::string& label, bool selected)
    {
        DrawIconRow(rt, { window.windowPos.x + 10, y - 8 }, window.width - 20, label, selected, PanelIcon::play);
    }

    struct ControlHint
    {
        std::string key, label;
    };
    struct ControlPlacement
    {
        ControlHint hint;
        int x, y, width;
    };
    inline std::string ControlKey(std::string key)
    {
        if (key == "L/R")
            return "L1/R1";
        if (key == "SEL")
            return "SELECT";
        return key;
    }
    inline int ControlKeyWidth(const std::string& key)
    {
        return key == "D-pad" || key.size() == 1 ? 16 : TextWidth(key) + 8;
    }
    inline int ControlHintWidth(const ControlHint& hint)
    {
        return ControlKeyWidth(ControlKey(hint.key)) + 5 + TextWidth(hint.label) + 12;
    }
    inline void DrawCommand(Drawing::RenderTarget& rt, ScreenCoordsXY pos, int32_t, const char* key, const char* label)
    {
        const auto name = ControlKey(key);
        const int width = ControlKeyWidth(name);
        const int top = pos.y - 3;
        const auto face = static_cast<Drawing::PaletteIndex>(name == "A" ? PocketPalette::positive : PocketPalette::selection);
        if (name == "D-pad")
        {
            Fill(rt, { pos.x + 5, top + 1 }, { 5, 15 }, Theme::text);
            Fill(rt, { pos.x, top + 6 }, { 15, 5 }, Theme::text);
            Fill(rt, { pos.x + 6, top + 7 }, { 3, 3 }, Theme::edge);
        }
        else
        {
            Fill(rt, { pos.x + 2, top }, { width - 4, 17 }, face);
            Fill(rt, { pos.x, top + 2 }, { width, 13 }, face);
            DrawButtonLabel(rt, { pos.x, top }, { width, 17 }, name);
        }
        DrawModernText(rt, { pos.x + width + 5, pos.y }, std::string(name == "A" ? "{WHITE}" : "{MUTED}") + label);
    }
    inline void DrawFooter(Drawing::RenderTarget&, ScreenSize, const std::vector<ControlHint>&);
    // Word wrapping preserves the color on every line and never splits UTF-8.
    inline void DrawWrappedLabel(Drawing::RenderTarget& rt, ScreenCoordsXY p, int width, std::string text, int lines = 2)
    {
        std::string ink = "{WHITE}";
        if (text.starts_with("{") && text.find('}') != std::string::npos)
        {
            const auto end = text.find('}') + 1;
            ink = text.substr(0, end);
            text.erase(0, end);
        }
        std::istringstream words(text);
        std::string word, line;
        int row = 0;
        while (words >> word)
        {
            const auto next = line.empty() ? word : line + " " + word;
            if (!line.empty() && TextWidth(next, true) > width && row + 1 < lines)
            {
                DrawStrongText(rt, p + ScreenCoordsXY{ 0, row++ * 16 }, ink + line, width);
                line = word;
            }
            else
                line = next;
        }
        DrawStrongText(rt, p + ScreenCoordsXY{ 0, row * 16 }, ink + line, width);
    }

    // Shared hierarchy for summaries, construction and management. These surfaces
    // deliberately distinguish information from controller-focusable actions.
    inline void DrawSectionLabel(Drawing::RenderTarget& rt, ScreenCoordsXY p, int width, const std::string& text)
    {
        DrawModernEllipsised(rt, p, width, "{MUTED}" + text);
    }

    inline void DrawMetric(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const std::string& label, const std::string& value,
        bool selected = false, const std::string& valueInk = "{WHITE}")
    {
        Fill(rt, p, size, selected ? Theme::gold : Theme::raised);
        DrawModernEllipsised(rt, p + ScreenCoordsXY{8, 9}, size.width - 16,
            std::string(selected ? "{BLACK}" : "{MUTED}") + label);
        DrawStrongText(
            rt, p + ScreenCoordsXY{ 8, size.height >= 40 ? 27 : 23 }, std::string(selected ? "{BLACK}" : valueInk) + value,
            size.width - 16);
    }

    inline void DrawActionRow(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, const std::string& title, const std::string& subtitle,
        bool focused, bool enabled = true, PanelIcon icon = PanelIcon::none, bool destructive = false)
    {
        Fill(rt, p, size, focused ? Theme::gold : Theme::ink);
        if (focused)
            Fill(rt, p, {3, size.height}, Theme::text);
        else
            Fill(rt, p + ScreenCoordsXY{0, size.height - 1}, {size.width, 1}, Theme::raised);
        const auto colour = focused ? "{BLACK}" : enabled ? destructive ? "{DANGER}" : "{WHITE}" : "{DISABLED}";
        if (destructive)
            Fill(rt, p, { 3, size.height }, static_cast<Drawing::PaletteIndex>(PocketPalette::destructive));
        const int inset = icon == PanelIcon::none ? 10 : 36;
        DrawIcon(rt, p + ScreenCoordsXY{9, 8}, icon, focused);
        DrawStrongText(
            rt, p + ScreenCoordsXY{ inset, subtitle.empty() || size.height < 34 ? (size.height - 12) / 2 + 3 : 8 },
            std::string(colour) + title, size.width - inset - 22);
        if (!subtitle.empty() && size.height >= 34)
            DrawModernEllipsised(
                rt, p + ScreenCoordsXY{ inset, size.height - 13 }, size.width - inset - 22,
                std::string(focused ? "{BLACK}" : "{MUTED}") + subtitle);
        DrawModernText(rt, p + ScreenCoordsXY{size.width - 15, (size.height - 12) / 2 + 3},
            std::string(colour) + (enabled ? ">" : "-"));
    }

    inline void DrawPagePosition(
        Drawing::RenderTarget& rt, ScreenCoordsXY p, int width, size_t selected, size_t count, size_t capacity)
    {
        if (count <= capacity || !capacity)
            return;
        DrawModernEllipsised(rt, p, width,
            "{MUTED}Page " + std::to_string(selected / capacity + 1) + " / "
                + std::to_string((count + capacity - 1) / capacity));
    }

    inline void DrawPanelCommands(Drawing::RenderTarget& rt, const Window& window, const char* backLabel = "Back")
    {
        const auto pos = window.windowPos + ScreenCoordsXY{ 1, window.height - 29 };
        Fill(rt, pos, { window.width - 2, 28 }, Theme::raised);
        Fill(rt, pos, { window.width - 2, 1 }, Theme::edge);
        DrawFooter(rt, { rt.width, rt.height }, { { "D-pad", "Choose" }, { "A", "Select" }, { "B", backLabel } });
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
                model.rows[i], i == model.selected, i < model.icons.size() ? model.icons[i] : PanelIcon::none);
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
        const ScreenCoordsXY p{ 0, screen.height - 28 };
        Fill(rt, p, { screen.width, 28 }, static_cast<Drawing::PaletteIndex>(PocketPalette::panel));
        Concept::Rule(rt, p, screen.width);
    }

    inline std::vector<ControlPlacement> MeasureFooter(ScreenSize screen, const std::vector<ControlHint>& hints)
    {
        std::vector<ControlPlacement> primary;
        std::vector<ControlHint> extras;
        int leftEnd = 8, actionEnd = screen.width * 39 / 100, backStart = screen.width - 8;
        for (const auto& h : hints)
        {
            if (h.label.empty())
                continue;
            const int width = ControlHintWidth(h);
            if (h.key == "D-pad")
            {
                primary.push_back({ h, 8, 0, width });
                leftEnd = 8 + width;
            }
            else if (h.key == "A")
            {
                primary.push_back({ h, screen.width * 39 / 100, 0, width });
                actionEnd = screen.width * 39 / 100 + width;
            }
            else if (h.key == "B")
            {
                backStart = std::min(screen.width - 104, screen.width - 8 - width);
                primary.push_back({ h, backStart, 0, width });
            }
            else
                extras.push_back(h);
        }
        const auto fixed = primary;
        int x = leftEnd + 8;
        bool second = false;
        for (const auto& h : extras)
        {
            const int width = ControlHintWidth(h);
            if (x + width > screen.width * 39 / 100 - 8)
                x = std::max(x, actionEnd + 8);
            if (x + width > backStart - 8)
            {
                second = true;
                break;
            }
            primary.push_back({ h, x, 0, width });
            x += width + 8;
        }
        if (second)
        {
            primary = fixed;
            x = 8;
            for (const auto& h : extras)
            {
                const int width = ControlHintWidth(h);
                primary.push_back({ h, x, 1, width });
                x += width + 8;
            }
        }
        const int height = second ? 44 : 28;
        for (auto& item : primary)
            item.y = screen.height - height + 7 + item.y * 20;
        return primary;
    }
    inline int FooterHeight(ScreenSize screen, const std::vector<ControlHint>& hints)
    {
        const auto items = MeasureFooter(screen, hints);
        return std::any_of(items.begin(), items.end(), [&](const auto& p) { return p.y < screen.height - 21; }) ? 44 : 28;
    }
    inline void DrawFooter(Drawing::RenderTarget& rt, ScreenSize screen, const std::vector<ControlHint>& hints)
    {
        const int height = FooterHeight(screen, hints);
        Fill(rt, { 0, screen.height - height }, { screen.width, height }, Theme::ink);
        Concept::Rule(rt, { 0, screen.height - height }, screen.width);
        for (const auto& p : MeasureFooter(screen, hints))
            DrawCommand(rt, { p.x, p.y }, 0, p.hint.key.c_str(), p.hint.label.c_str());
    }
    inline void DrawModeBadge(Drawing::RenderTarget& rt, ScreenCoordsXY p, const std::string& label)
    {
        const int width = TextWidth(label, true) + 12;
        Fill(rt, p, { width, 18 }, Theme::raised);
        DrawStrongText(rt, p + ScreenCoordsXY{ 6, 5 }, "{WHITE}" + label, width - 12);
    }

    inline void DrawShopFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawFooter(rt, screen, { { "D-pad", "Move" }, { "A", "Place" }, { "Y", "Shops" }, { "SEL", "Turn" }, { "B", "Back" } });
    }

    inline void DrawTreeFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawFooter(rt, screen, { { "D-pad", "Move" }, { "A", "Place" }, { "Y", "Trees" }, { "B", "Back" } });
    }

    inline void DrawSceneryRemovalFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawFooter(rt, screen, { { "D-pad", "Move" }, { "A", "Remove" }, { "B", "Back" } });
    }

    inline void DrawTrackColourFooter(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        DrawFooter(rt, screen, { { "D-pad", "Move" }, { "A", "Paint" }, { "Y", "Scheme" }, { "B", "Back" } });
    }
    inline void DrawGameplayFooter(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::string&, bool terrain, bool surface = false)
    {
        DrawActionStrip(rt, screen);

        if (terrain)
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Move" },
                  { "A", surface ? "Paint" : "Edit" },
                  { "Y", surface ? "Surface" : "Tool" },
                  { "B", "Back" } });
        }
        else
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Move" },
                  { "A", "Inspect" },
                  { "B", "Back" },
                  { "SEL", "Build" },
                  { "START", "Park" },
                  { "L1/R1", "Zoom" },
                  { "X", "Rotate" } });
        }
    }

    inline void DrawPathOverlay(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& surface, bool queue, bool remove,
        const std::string& placement = "Ground / auto slope", bool connected = false, bool adjusting = false)
    {
        DrawSurface(rt, pos, {164, adjusting ? connected ? 174 : 194 : 146});
        DrawIcon(rt, pos + ScreenCoordsXY{10, 12}, PanelIcon::path);
        DrawStrongText(rt, pos + ScreenCoordsXY{36, 14}, adjusting ? "{WHITE}Adjust path" : connected ? "{WHITE}Extend path" : "{WHITE}Single path", 116);
        Fill(rt, pos + ScreenCoordsXY{8, 35}, {148, 22}, Theme::raised);
        DrawStrongText(rt, pos + ScreenCoordsXY{14, 42},
            remove ? "{WHITE}REMOVE" : queue ? "{WHITE}QUEUE" : "{WHITE}FOOTPATH", 136);
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{12, 70}, 140, "{WHITE}" + surface);
        const auto slash = placement.find(" / ");
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{12, 91}, 140,
            "{MUTED}" + (remove ? std::string("Existing tile height") : placement.substr(0, slash)));
        if (!remove && slash != std::string::npos)
            DrawModernEllipsised(rt, pos + ScreenCoordsXY{12, 109}, 140, "{MUTED}" + placement.substr(slash + 3));
        if (adjusting)
        {
            DrawModernText(rt,pos+ScreenCoordsXY{12,128},"{WHITE}Left/Right  Turn");
            DrawModernText(rt,pos+ScreenCoordsXY{12,148},connected ? "{WHITE}Up/Down  Slope" : "{WHITE}Up/Down  Height");
        }
        else DrawCommand(rt,pos+ScreenCoordsXY{12,128},32,"L2/R2",connected ? "Slope" : "Height");
    }

    inline void DrawPathFooter(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::string&, bool remove = false,
        bool connected = false, bool adjusting = false, bool ready = false)
    {
        DrawActionStrip(rt, screen);

        if (adjusting)
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Adjust" }, { "A", connected ? "Flat" : "Flat/slope" }, { "X", "More" }, { "B", "Build" } });
        }
        else
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Move" },
                  { "A",
                    connected    ? ready ? "Extend" : "Start"
                        : remove ? "Remove"
                                 : "Place" },
                  { "X",
                    connected    ? "View"
                        : remove ? "Place"
                                 : "Remove" },
                  { "Y", "Adjust" },
                  { "SEL", "Surface" },
                  { "L1/R1", "Zoom" },
                  { "B", "Back" } });
        }
    }

    inline void DrawCostBadge(Drawing::RenderTarget& rt, ScreenCoordsXY pos, const std::string& amount, bool quote = false)
    {
        Concept::Box(rt, pos, { 160, 26 });
        DrawModernText(rt, pos + ScreenCoordsXY{ 8, 8 }, quote ? "{MUTED}Quote:" : "{MUTED}Cost:");
        const int valueX = quote ? 50 : 43;
        DrawStrongText(rt, pos + ScreenCoordsXY{ valueX, 8 }, "{WHITE}" + amount, 152 - valueX);
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

    // Bounded, content-sized feedback for a selection-wide native rejection or result.
    inline void DrawConstructionMessage(Drawing::RenderTarget& rt, ScreenSize screen, const std::string& text)
    {
        constexpr int width = 216;
        std::vector<std::string> lines;
        std::istringstream words(text);
        std::string line, word;
        while (words >> word)
        {
            const auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && TextWidth(candidate) > width - 20 && lines.size() < 3)
            {
                lines.push_back(line);
                line = word;
            }
            else line = candidate;
        }
        if (!line.empty()) lines.push_back(line);
        const ScreenSize size{width,16+static_cast<int>(lines.size())*17};
        const auto pos = Layout::ContextOrigin(screen,{screen.width-Layout::margin-width,screen.height},size);
        DrawSurface(rt,pos,size);
        for (size_t i=0;i<lines.size();++i)
            DrawModernEllipsised(rt,pos+ScreenCoordsXY{10,8+static_cast<int>(i)*17},width-20,
                (i==0 ? "{WHITE}" : "{MUTED}")+lines[i]);
    }

    inline void DrawPathOptions(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::vector<std::string>& rows, size_t selected,
        const std::string& status)
    {
        const auto p = Layout::menuOrigin;
        constexpr int width = 240;
        DrawSurface(rt, p, {width, Layout::ContentHeight(screen)});
        DrawSectionLabel(rt, p + ScreenCoordsXY{12, 13}, width - 24, "BUILD / PATHS");
        DrawHeadingText(rt, p + ScreenCoordsXY{12, 28}, "{WHITE}Path settings", width - 24);
        DrawModernEllipsised(rt, p + ScreenCoordsXY{12, 54}, width - 24, "{MUTED}" + status);
        const bool connected = rows.size() > 7;
        const int positions[] = {76, 127, 155, 183, 211, 254, 282, 310};
        DrawSectionLabel(rt, p + ScreenCoordsXY{12, 111}, width - 24, connected ? "DIRECTION & SLOPE" : "HEIGHT & SLOPE");
        DrawSectionLabel(rt, p + ScreenCoordsXY{12, 238}, width - 24, connected ? "SURFACE & RECOVERY" : "ORIENTATION & SURFACE");
        for (size_t i = 0; i < rows.size() && i < 8; ++i)
            DrawActionRow(rt, p + ScreenCoordsXY{8, positions[i]}, {width - 16, i == 0 ? 30 : 27},
                i == 0 ? "Return to building" : rows[i], "", selected == i);
        DrawFooter(rt, screen, { { "D-pad", "Choose" }, { "A", "Apply" }, { "B", "Tool" } });
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
                model.rows[i], i == model.selected, i < model.icons.size() ? model.icons[i] : PanelIcon::none);
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

