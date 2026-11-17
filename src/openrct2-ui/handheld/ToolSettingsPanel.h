// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConstructionAdjust.h"

namespace OpenRCT2::Ui::Handheld
{
    inline std::vector<std::vector<size_t>> ToolSettingGroups(bool scenery)
    {
        return scenery ? std::vector<std::vector<size_t>>{ { 0, 1, 2, 3 },
                                                           { 4, 5, 6 },
                                                           { 7, 8, 9 },
                                                           { 10, 11, 12, 13, 14, 15 },
                                                           { 16, 17, 18, 19 } }
                       : std::vector<std::vector<size_t>>{ { 0, 1, 2, 3, 4 }, { 5, 6, 7, 8 } };
    }
    inline size_t ToolSettingGroup(bool scenery, size_t row)
    {
        const auto groups = ToolSettingGroups(scenery);
        for (size_t i = 0; i < groups.size(); ++i)
            if (std::find(groups[i].begin(), groups[i].end(), row) != groups[i].end())
                return i;
        return 0;
    }
    inline std::string ToolSettingHelp(bool scenery, size_t row)
    {
        if (scenery)
        {
            static const std::array<const char*, 20> help{
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
                "Sets spacing between scattered objects. Placement rules still apply.",
                "The most recent placement error is shown here."
            };
            return row < help.size() ? help[row] : "";
        }
        static const std::array<const char*, 9> help{
            "Return to the park. A applies the tool there.",
            "Choose land, water, smoothing or painting. The park preview follows your choice.",
            "Change the brush footprint across the ground. This does not change elevation.",
            "Change the brush footprint along the ground. This does not change elevation.",
            "Affect the whole tile, a corner or an edge. Water and paint use the whole area.",
            "Choose the material on top of the ground. Keep unchanged preserves it.",
            "Choose the exposed sides of raised land. Keep unchanged preserves them.",
            "Change colour when the selected ground material supports it.",
            "Change colour when the selected cliff material supports it."
        };
        return row < help.size() ? help[row] : "";
    }
    inline void DrawToolSettings(
        Drawing::RenderTarget& rt, ScreenSize screen, bool scenery, const std::vector<std::string>& rows, size_t selected,
        const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb = scenery ? "Build / Scenery" : "Build / Landscape";
        model.title = scenery ? "Scenery settings" : "Landscape settings";
        const auto group = ToolSettingGroup(scenery, selected);
        const auto indices = ToolSettingGroups(scenery)[group];
        const auto tabs = scenery ? std::vector<std::string>{ "Place", "Height", "Colour", "Tools", "Scatter" }
                                  : std::vector<std::string>{ "Brush", "Paint" };
        model.reservedHeight = 34;
        model.rowHeight = 30;
        for (size_t i = 0; i < indices.size(); ++i)
        {
            const auto row = indices[i];
            model.rows.push_back(rows[row]);
            if (row == selected)
                model.selected = i;
            model.rowEnabled.push_back(
                scenery ? ConstructionAdjust::SceneryEnabled(row) : ConstructionAdjust::TerrainEnabled(row));
        }
        // The groups belong to the whole settings sheet. One continuous surface
        // keeps their shared tab strip inside its frame, above both columns.
        DrawSurface(rt, { 8, 8 }, { 496, 340 });
        DrawManagementContent(rt, { 8, 8 }, { 280, 340 }, model, true, false);
        Fill(rt, { 294, 110 }, { 1, 222 }, Theme::edge);
        DrawHeadingText(rt, { 308, 38 }, scenery ? "Selected object" : "Brush footprint", 184);
        if (scenery)
        {
            DrawWrappedLabel(rt, { 308, 124 }, 184, SceneryTools::Name(SceneryTools::Current()), 2);
            SceneryTools::DrawPreview(rt, { 308, 164 }, { 184, 56 });
            DrawWrappedLabel(rt, { 308, 224 }, 184, SceneryTools::HeightName(), 1);
        }
        else
        {
            ConstructionAdjust::DrawBrushFootprint(
                rt, { 316, 122 }, { 168, 88 }, AreaTools::settings.width, AreaTools::settings.height);
            DrawWrappedLabel(rt, { 308, 224 }, 184, AreaTools::SizeName(AreaTools::settings), 2);
        }
        const bool enabled = model.rowEnabled[model.selected];
        DrawWrappedLabel(
            rt, { 308, 254 }, 184,
            scenery && selected == 19 ? rows[selected]
                : enabled             ? ToolSettingHelp(scenery, selected)
                : scenery             ? "This option is fixed or does not apply to the selected object."
                                      : "This option is fixed or does not apply to this tool.",
            4);
        DrawModernEllipsised(rt, { 308, 328 }, 184, status);
        DrawTabs(rt, { 18, 70 }, 476, tabs, group, "Build / Tools");
        DrawFooter(rt, screen, { { "D-pad", "Adjust" }, { "L1/R1", "Group" }, { "A", "Select" }, { "B", "Back" } });
    }
} // namespace OpenRCT2::Ui::Handheld
