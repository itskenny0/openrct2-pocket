// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"
#include "SceneryTools.h"
#include "TabbedActions.h"
#include "ColourGrid.h"

namespace OpenRCT2::Ui::Handheld::ConstructionAdjust
{
    // Only transient UI focus. Geometry/materials remain in existing tool drafts.
    struct State
    {
        bool active{};
        TabFocus<4> focus;
        size_t row{};
        bool palette{};
        size_t colourChoice{};
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
            case 1: {
                std::vector<size_t> rows;
                const auto& current = SceneryTools::Current();
                const auto channels = SceneryTools::Channels(current.category, current.object);
                for (size_t i = 0; i < channels.size(); ++i)
                    if (channels[i]) rows.push_back(7 + i);
                return rows.empty() ? std::vector<size_t>{static_cast<size_t>(-1)} : rows;
            }
            case 2: return {10};
            default: return {16,17,18};
        }
    }
    inline bool SceneryEnabled(size_t row)
    {
        using namespace SceneryTools;
        const auto& s=Current();
        if (row == static_cast<size_t>(-1)) return false;
        if (row==2) return CanRotate();
        if (row==3 || (row>=16 && row<=18)) return s.category==Category::small;
        if (row>=4 && row<=6) return s.category!=Category::additions;
        if (row==19) return false;
        if (row>=7 && row<=9) return Channels(s.category,s.object)[row-7];
        return true;
    }
    inline void DrawBrushFootprint(Drawing::RenderTarget& rt, ScreenCoordsXY p, ScreenSize size, int width, int depth)
    {
        const int columns=std::min(width,8), rows=std::min(depth,8);
        const int cell=std::max(2,std::min(size.width/columns,size.height/rows));
        p += ScreenCoordsXY{(size.width-columns*cell)/2,(size.height-rows*cell)/2};
        for(int y=0;y<rows;++y) for(int x=0;x<columns;++x)
            Fill(rt,p+ScreenCoordsXY{x*cell,y*cell},{cell-1,cell-1},Theme::gold);
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
    inline bool ColourRow(bool scenery, size_t row)
    {
        return scenery ? row >= 7 && row <= 9 : row == 7 || row == 8;
    }
    inline Drawing::Colour CurrentColour(bool scenery, size_t row)
    {
        return scenery ? SceneryTools::Current().colours[row-7]
            : row == 7 ? AreaTools::settings.surfaceColour : AreaTools::settings.edgeColour;
    }
    inline size_t ColourIndex(Drawing::Colour colour)
    {
        for (uint8_t i=0; i<72; ++i) if (Windows::ColourDropDownIndexToColour(i)==colour) return i;
        return 0;
    }
    inline bool PickerAction(const State& state, bool scenery)
    {
        const auto rows=scenery ? SceneryRows(state.focus.tab) : TerrainRows(state.focus.tab);
        const auto row=rows[std::min(state.row, rows.size() - 1)];
        return scenery ? ColourRow(true,row) && SceneryEnabled(row)
            : state.focus.tab==2 && (row<=6 || (ColourRow(false,row) && TerrainEnabled(row)));
    }
    inline void Draw(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, const State& state, bool scenery)
    {
        const std::vector<std::string> tabs=scenery ? std::vector<std::string>{"Object","Colour","Mode","Scatter"}
            : std::vector<std::string>{"Brush","Shape","Paint"};
        const auto indices=scenery ? SceneryRows(state.focus.tab) : TerrainRows(state.focus.tab);
        const auto selectedRow = std::min(state.row, indices.size() - 1);
        const auto options=scenery ? SceneryTools::Options() : AreaTools::Options();
        DrawSurface(rt,pos,{224,282});
        if (state.palette)
        {
            std::vector<uint16_t> values;
            std::vector<std::string> labels;
            for (uint8_t i=0;i<72;++i)
            {
                values.push_back(static_cast<uint16_t>(Windows::ColourDropDownIndexToColour(i)));
                labels.push_back(FmtString(FormatStringID(Windows::ColourDropDownIndexToName(i))).WithoutFormatTokens());
            }
            DrawHeadingText(rt,pos+ScreenCoordsXY{12,12},"Choose colour",200);
            DrawModernText(rt,pos+ScreenCoordsXY{12,38},scenery ? "{MUTED}Scenery colour" : "{MUTED}Terrain colour");
            DrawColourGrid(rt,pos,{224,282},values,labels,state.colourChoice,
                static_cast<uint16_t>(CurrentColour(scenery,indices[selectedRow])));
            return;
        }
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
            if (scenery && row==10)
            {
                static constexpr std::array names{"Place","Recolour","Remove","Pick"};
                static constexpr std::array icons{PanelIcon::play,PanelIcon::terrain,PanelIcon::remove,PanelIcon::folder};
                for (size_t mode=0;mode<names.size();++mode)
                {
                    const auto cell=pos+ScreenCoordsXY{8+static_cast<int>(mode%2)*106,150+static_cast<int>(mode/2)*44};
                    const bool selected=static_cast<size_t>(SceneryTools::Current().mode)==mode;
                    DrawCard(rt,cell,{100,38},selected);
                    DrawIcon(rt,cell+ScreenCoordsXY{6,10},icons[mode],selected);
                    DrawModernEllipsised(rt,cell+ScreenCoordsXY{30,14},64,
                        std::string(selected ? "{BLACK}" : "{WHITE}")+names[mode]);
                }
                continue;
            }
            auto label = row == static_cast<size_t>(-1) ? "No adjustable colours" : options[row];
            const auto cell=pos+ScreenCoordsXY{8,(scenery ? 148 : 120)+static_cast<int>(i)*30};
            const bool colour=ColourRow(scenery,row) && (scenery ? SceneryEnabled(row) : TerrainEnabled(row));
            if (colour)
            {
                Fill(rt,cell,{208,28},i==selectedRow ? Theme::gold : Theme::ink);
                DrawModernEllipsised(rt,cell+ScreenCoordsXY{10,9},156,(i==selectedRow ? "{BLACK}" : "{WHITE}")+label);
                const auto shade=Drawing::getColourMap(CurrentColour(scenery,row));
                Fill(rt,cell+ScreenCoordsXY{180,5},{22,18},Theme::edge);
                Fill(rt,cell+ScreenCoordsXY{182,7},{18,14},shade.midLight);
            }
            else DrawActionRow(rt,cell,{208,28},label,"",i==selectedRow,
                scenery ? SceneryEnabled(row) : TerrainEnabled(row));
        }
        if (!scenery && state.focus.tab==0)
        {
            DrawBrushFootprint(rt,pos+ScreenCoordsXY{12,214},{64,28},AreaTools::settings.width,AreaTools::settings.height);
            DrawModernEllipsised(rt,pos+ScreenCoordsXY{86,224},126,"{MUTED}"+AreaTools::SizeName(AreaTools::settings));
        }
        DrawModernEllipsised(rt, pos + ScreenCoordsXY{ 12, 250 }, 200,
            indices[selectedRow] == static_cast<size_t>(-1) ? "{MUTED}Choose another object to recolour"
                : scenery && indices[selectedRow]==6 ? "{MUTED}Left/Right: Height / A: Ground"
                : ColourRow(scenery,indices[selectedRow]) ? "{MUTED}A Colour picker" : "{MUTED}Left/Right: Change value");
        DrawCommand(rt,pos+ScreenCoordsXY{12,266},24,"X","More settings");
    }
    inline void Footer(Drawing::RenderTarget& rt, ScreenSize screen, bool adjusting, bool scenery, const std::string& apply="Apply", bool material=false, bool palette=false, bool groundReset=false)
    {
        DrawActionStrip(rt, screen);
        if(adjusting)
        {
            if (palette)
            {
                DrawFooter(rt,screen,{{"D-pad","Choose"},{"A","Use colour"},{"B","Cancel"}});
                return;
            }
            DrawFooter(
                rt, screen,
                { { "D-pad", "Adjust" }, { "A", groundReset ? "Ground" : material ? "Select" : "Build" }, { "L/R", "Group" }, { "B", "Build" } });
        }
        else
        {
            DrawFooter(
                rt, screen,
                { { "D-pad", "Move" },
                  { "A", apply },
                  { "Y", "Adjust" },
                  { "SEL", scenery ? SceneryTools::CanRotate() ? "Turn" : "Auto" : "Material" },
                  { "L1/R1", "Zoom", HintPriority::guide },
                  { "B", "Back" } });
        }
    }
}

