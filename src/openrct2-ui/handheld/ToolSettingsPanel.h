// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConstructionAdjust.h"

namespace OpenRCT2::Ui::Handheld
{
    inline std::string ToolSettingHelp(bool scenery, size_t row)
    {
        if (scenery)
        {
            static const std::array<const char*,20> help{
                "Return to the park with these settings. Nothing is placed here.",
                "Browse the visual catalog for another object.",
                "Turn the selected object. Path furniture attaches to the selected real path.",
                "Choose a corner within the tile for small scenery.",
                "Raise by one level from the current placement preview.",
                "Lower by one level from the current placement preview.",
                "Use automatic ground height again. Furniture follows its selected path.",
                "Change the first supported object colour.",
                "Change the second supported object colour.",
                "Change the third supported object colour.",
                "Place, recolour, remove or copy an object from the park.",
                "Point at a sign in the park to change its message.",
                "Limit the catalog to a scenery collection.",
                "Point at a sign to change its colours or entry rules.",
                "Copy an existing object's appearance and settings.",
                "Find objects by name in the catalog.",
                "Scatter small scenery over an area. Review the batch before paying.",
                "Set the square area for scattering, measured in tiles.",
                "Sparse, medium or dense. Space and placement rules determine how many fit.",
                "The most recent placement error is shown here." };
            return row<help.size()?help[row]:"";
        }
        static const std::array<const char*,9> help{
            "Return to the park. A applies the tool there.",
            "Choose land, water, smoothing or painting. The park preview follows your choice.",
            "Change the brush footprint across the ground. This does not change elevation.",
            "Change the brush footprint along the ground. This does not change elevation.",
            "Affect the whole tile, a corner or an edge. Water and paint use the whole area.",
            "Choose the material on top of the ground. Keep unchanged preserves it.",
            "Choose the exposed sides of raised land. Keep unchanged preserves them.",
            "Change colour when the selected ground material supports it.",
            "Change colour when the selected cliff material supports it." };
        return row<help.size()?help[row]:"";
    }
    inline void DrawToolSettings(
        Drawing::RenderTarget& rt, ScreenSize screen, bool scenery,
        const std::vector<std::string>& rows, size_t selected, const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb=scenery?"Build / Scenery":"Build / Landscape";
        model.title=scenery?"Scenery settings":"Landscape settings";
        model.rows=rows; model.selected=selected; model.rowHeight=30;
        for(size_t i=0;i<rows.size();++i)
            model.rowEnabled.push_back(scenery?ConstructionAdjust::SceneryEnabled(i):ConstructionAdjust::TerrainEnabled(i));
        DrawManagementContent(rt,{8,8},{280,340},model);
        DrawSurface(rt,{296,8},{208,340});
        DrawHeadingText(rt,{308,24},scenery?"Selected object":"Brush footprint",184);
        if(scenery)
        {
            DrawWrappedLabel(rt,{308,56},184,SceneryTools::Name(SceneryTools::Current()),3);
            SceneryTools::DrawPreview(rt,{308,104},{184,72});
            DrawWrappedLabel(rt,{308,186},184,SceneryTools::HeightName(),2);
        }
        else
        {
            ConstructionAdjust::DrawBrushFootprint(rt,{316,64},{168,100},AreaTools::settings.width,AreaTools::settings.height);
            DrawWrappedLabel(rt,{308,186},184,AreaTools::SizeName(AreaTools::settings),2);
        }
        const bool enabled=selected<model.rowEnabled.size() && model.rowEnabled[selected];
        DrawWrappedLabel(rt,{308,226},184,scenery && selected==19 ? rows[selected] : enabled?ToolSettingHelp(scenery,selected)
            : scenery?"This option is fixed or does not apply to the selected object."
                     :"This option is fixed or does not apply to this tool.",5);
        DrawWrappedLabel(rt,{308,312},184,status,2);
        DrawFooter(rt,screen,{{"D-pad","Choose / adjust"},{"A","Select"},{"B","Back"}});
    }
}
