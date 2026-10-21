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
        Category{ Section::overview, "Overview", true },  Category{ Section::rides, "Rides", true },
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
        Section section = Section::overview;
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
        const auto index = static_cast<size_t>(section);
        model.title = index < kCategories.size() ? kCategories[index].name : "Unavailable";
        model.stats = { { "Availability", "Not implemented" } };
        model.status = "Choose an available section";
        return model;
    }
    // Small semantic fallbacks also work without the original RCT2 tab sprites.
    inline void DrawCategoryMark(Drawing::RenderTarget& rt, ScreenCoordsXY p, size_t category, bool selected)
    {
        const auto colour = selected ? Theme::ink : Theme::text;
        const auto line = [&](int x1, int y1, int x2, int y2) {
            GfxDrawLine(rt, {p + ScreenCoordsXY{x1, y1}, p + ScreenCoordsXY{x2, y2}}, colour);
        };
        if (category == 0) { line(3,1,3,16); line(4,2,14,2); line(14,2,11,7); line(11,7,4,7); }
        else if (category == 1)
        {
            const std::array<ScreenCoordsXY,9> points{{{5,1},{11,1},{15,5},{15,11},{11,15},{5,15},{1,11},{1,5},{5,1}}};
            for (size_t i=1;i<points.size();++i) { GfxDrawLine(rt,{p+points[i-1],p+points[i]},colour); GfxDrawLine(rt,{p+ScreenCoordsXY{8,8},p+points[i]},colour); }
            line(8,8,4,18);line(8,8,12,18);line(2,18,14,18);
        }
        else if (category == 2)
        {
            Fill(rt,p+ScreenCoordsXY{2,2},{4,4},colour);Fill(rt,p+ScreenCoordsXY{11,2},{4,4},colour);
            line(1,9,7,9);line(0,10,0,15);line(8,10,8,15);line(10,9,16,9);line(17,10,17,15);
            line(3,9,3,18);line(5,9,5,18);line(12,9,12,18);line(14,9,14,18);
        }
        else if (category == 3) { line(3,1,1,6);line(1,6,5,10);line(5,10,9,9);line(9,9,15,16);line(15,16,17,14);line(17,14,10,7);line(10,7,10,3);line(10,3,6,0);line(6,0,6,5);line(6,5,3,5);line(3,5,3,1); }
        else if (category == 4) { Fill(rt,p+ScreenCoordsXY{1,10},{4,7},colour);Fill(rt,p+ScreenCoordsXY{7,6},{4,11},colour);Fill(rt,p+ScreenCoordsXY{13,1},{4,16},colour);line(0,19,18,19); }
        else if (category == 5) { line(6,0,12,0);line(7,1,7,7);line(11,1,11,7);line(7,7,1,17);line(11,7,17,17);line(1,17,17,17);line(4,13,14,13); }
        else { line(2,6,7,6);line(7,6,16,1);line(16,1,16,15);line(16,15,7,10);line(7,10,2,10);line(2,10,2,6);line(6,11,8,18);line(8,18,11,18); }
    }
    inline std::vector<ControlHint> Hints(const State& state, const ManagementPresentation& model, bool sectionRoot)
    {
        std::vector<ControlHint> hints{ { "D-pad",
                                          state.focus == Focus::rail         ? "Navigate"
                                              : model.navigationHint.empty() ? model.editing ? "Adjust" : "Choose"
                                                                             : model.navigationHint },
                                        { "A",
                                          state.focus == Focus::rail         ? "Select"
                                              : !model.primaryAction.empty() ? model.primaryAction
                                              : model.editing                ? "Review"
                                              : model.confirmation           ? "Confirm"
                                                                             : "Select" },
                                        { "B",
                                          state.focus == Focus::rail ? "Park"
                                              : model.confirmation   ? "Cancel"
                                                                     : "Back" } };
        if (state.focus == Focus::content && !model.tabs.empty())
            hints.push_back({ "L1/R1", "Tab" });
        else if (state.focus == Focus::content && (model.denseRoster || (state.section == Section::rides && sectionRoot)))
            hints.push_back({ "L1/R1", "Page" });
        else if (state.focus == Focus::rail)
            hints.push_back({ "L1/R1", "Section" });
        if (state.focus == Focus::rail) hints.push_back({"X","News"});
        if (state.focus == Focus::content && !model.contextualAction.empty())
            hints.push_back({ "X", model.contextualAction });
        hints.push_back(
            { state.focus == Focus::rail || sectionRoot || !model.tabs.empty() || model.denseRoster ? "SELECT" : "START",
              state.focus == Focus::rail                                    ? "Enter"
                  : sectionRoot || !model.tabs.empty() || model.denseRoster ? "Sections"
                                                                            : "Park",
              HintPriority::guide });
        return hints;
    }
    inline int ContentHeight(ScreenSize screen, const State& state, const ManagementPresentation& model, bool sectionRoot)
    {
        return screen.height - FooterHeight(screen, Hints(state, model, sectionRoot)) - 2 * Layout::margin;
    }
    inline void Draw(
        Drawing::RenderTarget& rt, ScreenSize screen, const State& state, const ManagementPresentation& model, bool sectionRoot)
    {
        using namespace Concept;
        constexpr int railWidth = 112;
        const auto rail = Layout::menuOrigin;
        const auto content = rail + ScreenCoordsXY{ railWidth + Layout::gutter, 0 };
        const int height = ContentHeight(screen, state, model, sectionRoot);
        const bool expanded = state.focus == Focus::content;
        if (!expanded)
        {
        DrawSurface(rt, rail, { railWidth, height });
        DrawStrongText(rt, rail + ScreenCoordsXY{ 10, 14 }, "{WHITE}Your park", railWidth - 20);
        Rule(rt, rail + ScreenCoordsXY{ 10, 42 }, railWidth - 20);
        for (size_t i = 0; i < kCategories.size(); ++i)
        {
            const auto& category = kCategories[i];
            const int y = 52 + static_cast<int>(i) * 40;
            const bool active = i == state.index();
            const bool focused = active && state.focus == Focus::rail;
            if (focused)
                Fill(rt, rail + ScreenCoordsXY{ 6, y }, { railWidth - 12, 36 }, Theme::gold);
            else if (active)
            {
                for (int x = 6; x <= 8; ++x)
                    GfxDrawLine(
                        rt, { rail + ScreenCoordsXY{ x, y + 3 }, rail + ScreenCoordsXY{ x, y + 31 } },
                        static_cast<Drawing::PaletteIndex>(PocketPalette::selection));
            }
            static constexpr std::array sprites{SPR_G2_TAB_LAND, SPR_TAB_RIDE_0, SPR_TAB_GUESTS_0,
                SPR_TAB_STAFF_OPTIONS_0, SPR_TAB_FINANCES_SUMMARY_0, SPR_TAB_FINANCES_RESEARCH_0, SPR_TAB_FINANCES_MARKETING_0};
            static constexpr std::array<uint8_t, 7> frames{ 1, 16, 8, 7, 8, 8, 19 };
            if (!DrawNativeMenuArt(rt, rail + ScreenCoordsXY{ 7, y + 2 }, { 28, 32 }, { sprites[i], frames[i] }, focused))
                DrawCategoryMark(rt, rail + ScreenCoordsXY{12, y + 7}, i, focused);
            DrawModernEllipsised(
                rt, rail + ScreenCoordsXY{ 39, y + 13 }, railWidth - 45,
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
        }
        const auto workspace = model.confirmation ? ScreenCoordsXY{(screen.width-360)/2, Layout::margin+20} : expanded ? rail : content;
        DrawManagementContent(
            rt, workspace, {model.confirmation ? 360 : screen.width - workspace.x - Layout::margin,
                model.confirmation ? height-20 : height}, model, state.focus == Focus::content);
        DrawFooter(rt, screen, Hints(state, model, sectionRoot));
    }
} // namespace OpenRCT2::Ui::Handheld::Dashboard
