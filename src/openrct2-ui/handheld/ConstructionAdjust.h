// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"
#include "SceneryTools.h"
#include "TabbedActions.h"

namespace OpenRCT2::Ui::Handheld::ConstructionAdjust
{
    // Only transient UI focus. Geometry/materials remain in existing tool drafts.
    struct State
    {
        bool active{};
        TabFocus<4> focus;
        size_t row{};
        void reset() { *this = {}; }
        void tab(int delta, size_t count)
        {
            focus.save(row);
            focus.tab = (static_cast<int>(focus.tab) + delta + count) % count;
            row = focus.rows[focus.tab];
        }
        void move(int delta, size_t count)
        {
            row = static_cast<size_t>(std::clamp<int>(static_cast<int>(row) + delta, 0, static_cast<int>(count) - 1));
        }
    };
    inline std::vector<size_t> SceneryRows(size_t group)
    {
        switch (group)
        {
            case 0: return {2,3,6};
            case 1: return {7,8,9};
            case 2: return {10};
            default: return {16,17,18};
        }
    }
    inline bool SceneryEnabled(size_t row)
    {
        using namespace SceneryTools;
        const auto& s=Current();
        if (row==2) return CanRotate();
        if (row==3 || row>=16) return s.category==Category::small;
        if (row>=7 && row<=9) return Channels(s.category,s.object)[row-7];
        return true;
    }
    inline std::vector<size_t> TerrainRows(size_t group)
    {
        return group==0 ? std::vector<size_t>{1,2,3}
            : group==1 ? std::vector<size_t>{4} : std::vector<size_t>{5,6,7,8};
    }
    inline bool TerrainEnabled(size_t row)
    {
        if (row==4) return AreaTools::settings.operation!=AreaTools::Operation::paint
            && AreaTools::settings.operation!=AreaTools::Operation::waterRaise
            && AreaTools::settings.operation!=AreaTools::Operation::waterLower;
        if (row==7) return AreaTools::SurfaceColourSupported(AreaTools::settings.surface);
        if (row==8) return AreaTools::EdgeColourSupported(AreaTools::settings.edge);
        return true;
    }
    inline void Draw(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const State& state, bool scenery)
    {
        const std::vector<std::string> tabs=scenery ? std::vector<std::string>{"Object","Colour","Mode","Scatter"}
            : std::vector<std::string>{"Brush","Shape","Paint"};
        const auto indices=scenery ? SceneryRows(state.focus.tab) : TerrainRows(state.focus.tab);
        const auto options=scenery ? SceneryTools::Options() : AreaTools::Options();
        DrawSurface(rt,pos,{224,282});
        DrawHeadingText(rt,pos+ScreenCoordsXY{12,12},scenery ? "Adjust scenery" : "Adjust terrain",200);
        DrawModernEllipsised(rt,pos+ScreenCoordsXY{12,40},200,"{WHITE}"+(scenery ? SceneryTools::Name(SceneryTools::Current())
            : AreaTools::operations[static_cast<size_t>(AreaTools::settings.operation)]));
        DrawModernEllipsised(
            rt, pos + ScreenCoordsXY{ 12, 62 }, 200,
            scenery ? "{MUTED}L2/R2: Height / Select: Catalog" : "{MUTED}L2/R2: Tool / Select: Material");
        if(scenery)
        {
            DrawTabs(rt,pos+ScreenCoordsXY{8,87},208,{tabs[0],tabs[1]},state.focus.tab<2 ? state.focus.tab : 2);
            DrawTabs(rt,pos+ScreenCoordsXY{8,115},208,{tabs[2],tabs[3]},state.focus.tab>=2 ? state.focus.tab-2 : 2);
        }
        else DrawTabs(rt,pos+ScreenCoordsXY{8,87},208,tabs,state.focus.tab);
        for(size_t i=0;i<indices.size();++i)
        {
            const auto row=indices[i];
            auto label=options[row];
            if(scenery && row==18) label="Density: "+std::to_string(static_cast<int>(SceneryTools::Current().scatterDensity)+1)+" / 3";
            DrawActionRow(rt,pos+ScreenCoordsXY{8,(scenery ? 148 : 120)+static_cast<int>(i)*30},{208,28},label,"",i==state.row,
                scenery ? SceneryEnabled(row) : TerrainEnabled(row));
        }
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 12, 250 }, 200, "{MUTED}Left/Right: Change value");
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 12, 266 }, 200, "{MUTED}X More / B or Y: Build");
    }
    inline void Footer(Drawing::RenderTarget& rt, ScreenSize screen, bool adjusting, bool scenery, const std::string& apply="Apply", bool material=false)
    {
        DrawActionStrip(rt, screen);
        if(adjusting)
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Adjust" }, { "A", material ? "Select" : "Build" }, { "L/R", "Group" }, { "B", "Build" } });
        }
        else
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Move" },
                  { "A", apply },
                  { "Y", "Adjust" },
                  { "SEL", scenery ? SceneryTools::CanRotate() ? "Turn" : "Auto" : "Material" },
                  { "B", "Back" } });
        }
    }
}

