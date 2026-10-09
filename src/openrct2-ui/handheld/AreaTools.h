// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Surface.h"

#include <algorithm>
#include <openrct2/actions/terraform/LandLowerAction.h>
#include <openrct2/actions/terraform/LandRaiseAction.h>
#include <openrct2/actions/terraform/LandSmoothAction.h>
#include <openrct2/actions/terraform/WaterLowerAction.h>
#include <openrct2/actions/terraform/WaterRaiseAction.h>
#include <openrct2/object/TerrainEdgeObject.h>
#include <openrct2/world/MapSelection.h>
namespace OpenRCT2::Ui::Handheld::AreaTools
{
    enum class Operation
    {
        raise,
        lower,
        smoothRaise,
        smoothLower,
        waterRaise,
        waterLower,
        paint
    };
    struct Settings
    {
        Operation operation{};
        int width = 1, height = 1;
        size_t selection{};
        ObjectEntryIndex surface = kObjectEntryIndexNull, edge = kObjectEntryIndexNull;
    };
    inline Settings settings;
    inline const std::array<MapSelectType, 9> selections{
        MapSelectType::full,  MapSelectType::corner0, MapSelectType::corner1, MapSelectType::corner2, MapSelectType::corner3,
        MapSelectType::edge0, MapSelectType::edge1,   MapSelectType::edge2,   MapSelectType::edge3
    };
    inline const std::array<const char*, 7> operations{ "Raise land",  "Lower land",  "Smooth / raise",      "Smooth / lower",
                                                        "Raise water", "Lower water", "Paint surface / edge" };
    inline std::optional<MapRange> Range(std::optional<CoordsXY> tile, int width, int height)
    {
        if (!tile || width < 1 || width > 64 || height < 1 || height > 64 || tile->x % 32 || tile->y % 32)
            return {};
        const auto max = GetMapSizeMaxXY();
        const auto first = *tile - CoordsXY{ ((width - 1) / 2) * 32, ((height - 1) / 2) * 32 };
        const auto last = first + CoordsXY{ (width - 1) * 32, (height - 1) * 32 };
        if (first.x < 32 || first.y < 32 || last.x > max.x || last.y > max.y)
            return {};
        return MapRange{ first.x, first.y, last.x, last.y };
    }
    inline GameActions::Result Invalid(const char* m)
    {
        GameActions::Result r;
        r.error = GameActions::Status::invalidParameters;
        r.errorMessage = m;
        return r;
    }
    inline MapSelectType Selection(const Settings& s)
    {
        if (s.operation == Operation::waterRaise || s.operation == Operation::waterLower)
            return MapSelectType::fullWater;
        if (s.operation == Operation::paint)
            return MapSelectType::full;
        return s.selection < selections.size() ? selections[s.selection] : MapSelectType::full;
    }
    inline std::string SelectionName(const Settings& s)
    {
        if (s.operation == Operation::waterRaise || s.operation == Operation::waterLower || s.operation == Operation::paint)
            return "Whole area";
        return s.selection == 0 ? "Whole tile"
            : s.selection < 5   ? "Corner " + std::to_string(s.selection)
                                : "Edge " + std::to_string(s.selection - 4);
    }
    inline std::string SizeName(const Settings& s)
    {
        return std::to_string(s.width) + " x " + std::to_string(s.height) + " tiles";
    }
    inline ObjectEntryIndex CycleStyle(ObjectType type, ObjectEntryIndex current, int delta)
    {
        std::vector<ObjectEntryIndex> items{ kObjectEntryIndexNull };
        for (ObjectEntryIndex i = 0; i < 255; ++i)
            if (GetContext()->GetObjectManager().GetLoadedObject(type, i))
                items.push_back(i);
        auto it = std::find(items.begin(), items.end(), current);
        int index = it == items.end() ? 0 : it - items.begin();
        return items[(index + delta + items.size()) % items.size()];
    }
    inline std::string StyleName(ObjectType type, ObjectEntryIndex i)
    {
        const auto* o = GetContext()->GetObjectManager().GetLoadedObject(type, i);
        return o ? o->GetName() : "Keep unchanged";
    }
    inline std::vector<std::string> Options()
    {
        return { "Resume",
                 operations[static_cast<size_t>(settings.operation)],
                 "Width: " + std::to_string(settings.width),
                 "Height: " + std::to_string(settings.height),
                 "Selection: " + SelectionName(settings),
                 "Surface: " + StyleName(ObjectType::terrainSurface, settings.surface),
                 "Edge: " + StyleName(ObjectType::terrainEdge, settings.edge) };
    }
    inline void Configure(size_t row, int delta = 1)
    {
        if (row == 1)
            settings.operation = static_cast<Operation>((static_cast<int>(settings.operation) + delta + 7) % 7);
        if (row == 2)
            settings.width = std::clamp(settings.width + delta, 1, 64);
        if (row == 3)
            settings.height = std::clamp(settings.height + delta, 1, 64);
        if (row == 4)
            settings.selection = (settings.selection + delta + selections.size()) % selections.size();
        if (row == 5)
            settings.surface = CycleStyle(ObjectType::terrainSurface, settings.surface, delta);
        if (row == 6)
            settings.edge = CycleStyle(ObjectType::terrainEdge, settings.edge, delta);
    }
    inline GameActions::Result Action(std::optional<CoordsXY> tile, const Settings& s, bool execute)
    {
        const auto range = Range(tile, s.width, s.height);
        if (!range || s.selection >= selections.size())
            return Invalid("Selection outside buildable map");
        const CoordsXY centre{ (range->getX1() + range->getX2()) / 2 + 16, (range->getY1() + range->getY2()) / 2 + 16 };
        std::unique_ptr<GameActions::GameAction> a;
        switch (s.operation)
        {
            case Operation::raise:
                a = std::make_unique<GameActions::LandRaiseAction>(centre, *range, Selection(s));
                break;
            case Operation::lower:
                a = std::make_unique<GameActions::LandLowerAction>(centre, *range, Selection(s));
                break;
            case Operation::smoothRaise:
                a = std::make_unique<GameActions::LandSmoothAction>(centre, *range, Selection(s), false);
                break;
            case Operation::smoothLower:
                a = std::make_unique<GameActions::LandSmoothAction>(centre, *range, Selection(s), true);
                break;
            case Operation::waterRaise:
                a = std::make_unique<GameActions::WaterRaiseAction>(*range);
                break;
            case Operation::waterLower:
                a = std::make_unique<GameActions::WaterLowerAction>(*range);
                break;
            case Operation::paint:
            {
                if (s.surface == kObjectEntryIndexNull && s.edge == kObjectEntryIndexNull)
                    return Invalid("Choose a surface or edge style");
                if ((s.surface != kObjectEntryIndexNull
                     && !GetContext()->GetObjectManager().GetLoadedObject(ObjectType::terrainSurface, s.surface))
                    || (s.edge != kObjectEntryIndexNull
                        && !GetContext()->GetObjectManager().GetLoadedObject(ObjectType::terrainEdge, s.edge)))
                    return Invalid("Terrain style unavailable");
                const auto* surface = MapGetSurfaceElementAt(*tile);
                if (!surface)
                    return Invalid("Terrain unavailable");
                a = std::make_unique<GameActions::SurfaceSetStyleAction>(
                    *range, s.surface, s.edge, surface->getPrimarySurfaceColour(), surface->getPrimaryEdgeColour());
                break;
            }
            default:
                return Invalid("Terrain operation unavailable");
        }
        return execute ? GameActions::Execute(a.get(), getGameState()) : GameActions::Query(a.get(), getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::AreaTools
