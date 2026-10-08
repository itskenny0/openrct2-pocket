// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ManagementPanel.h"

#include <array>

namespace OpenRCT2::Ui::Handheld::Dashboard
{
    enum class Section
    {
        overview,
        rides,
        guests,
        staff,
        finances,
        research,
        marketing
    };
    struct Category
    {
        Section section;
        const char* name;
        bool available;
    };
    inline constexpr std::array kCategories{
        Category{ Section::overview, "Overview", false }, Category{ Section::rides, "Rides", true },
        Category{ Section::guests, "Guests", true },      Category{ Section::staff, "Staff", true },
        Category{ Section::finances, "Finances", true },  Category{ Section::research, "Research", true },
        Category{ Section::marketing, "Marketing", true }
    };
    enum class Focus
    {
        rail,
        content
    };
    // Navigation only. Section drafts/targets and native actions belong to their adapters/menu.
    struct State
    {
        Section section = Section::finances;
        Focus focus = Focus::rail;
        size_t index() const
        {
            return static_cast<size_t>(section);
        }
        const Category& category() const
        {
            return kCategories[index()];
        }
        void move(int direction)
        {
            const auto count = static_cast<int>(kCategories.size());
            section = static_cast<Section>((static_cast<int>(index()) + direction % count + count) % count);
        }
        bool enter()
        {
            if (!category().available)
                return false;
            focus = Focus::content;
            return true;
        }
    };
    inline ManagementPresentation Unavailable(Section section)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park management";
        model.title = kCategories[static_cast<size_t>(section)].name;
        model.stats = { { "Availability", "Not implemented" } };
        model.status = "Choose an available section";
        return model;
    }
    inline void Draw(
        Drawing::RenderTarget& rt, ScreenSize screen, const State& state, const ManagementPresentation& model, bool sectionRoot)
    {
        using namespace Concept;
        constexpr int railWidth = 138;
        const ScreenCoordsXY rail{ 8, 8 }, content{ 152, 8 };
        Box(rt, rail, { railWidth, 338 });
        DrawStrongText(rt, rail + ScreenCoordsXY{ 10, 14 }, "{WHITE}Dashboard", railWidth - 20);
        Rule(rt, rail + ScreenCoordsXY{ 10, 42 }, railWidth - 20);
        for (size_t i = 0; i < kCategories.size(); ++i)
        {
            const auto& category = kCategories[i];
            const int y = 52 + static_cast<int>(i) * 38;
            const bool active = i == state.index();
            const bool focused = active && state.focus == Focus::rail;
            if (focused)
                Box(rt, rail + ScreenCoordsXY{ 6, y }, { railWidth - 12, 35 }, true);
            else if (active)
            {
                for (int x = 6; x <= 8; ++x)
                    GfxDrawLine(
                        rt, { rail + ScreenCoordsXY{ x, y + 3 }, rail + ScreenCoordsXY{ x, y + 31 } },
                        static_cast<Drawing::PaletteIndex>(PocketPalette::selection));
            }
            DrawModernEllipsised(
                rt, rail + ScreenCoordsXY{ 14, y + 8 }, railWidth - 24,
                std::string(
                    focused                  ? "{BLACK}"
                        : active             ? "{GOLD}"
                        : category.available ? "{WHITE}"
                                             : "{MUTED}")
                    + category.name);
            if (!category.available)
                DrawModernEllipsised(
                    rt, rail + ScreenCoordsXY{ 14, y + 22 }, railWidth - 24,
                    std::string(focused ? "{BLACK}" : "{MUTED}") + "Unavailable");
        }
        DrawManagementContent(rt, content, { screen.width - 160, 338 }, model, state.focus == Focus::content);
        DrawActionStrip(rt, screen);
        const int y = screen.height - 21;
        DrawCommand(rt, { 10, y }, 38, "D-pad", model.editing && state.focus == Focus::content ? "Adjust" : "Choose");
        DrawCommand(
            rt, { 154, y }, 17, "A",
            state.focus == Focus::rail         ? "Enter"
                : !model.primaryAction.empty() ? model.primaryAction.c_str()
                : model.editing                ? "Review"
                : model.confirmation           ? "Confirm"
                                               : "Select");
        DrawCommand(rt, { 272, y }, 17, "B", state.focus == Focus::rail ? "Park" : "Back");
        DrawCommand(rt, { 386, y }, sectionRoot ? 28 : 44, sectionRoot ? "SEL" : "START", sectionRoot ? "Pane" : "Menu");
    }
} // namespace OpenRCT2::Ui::Handheld::Dashboard
