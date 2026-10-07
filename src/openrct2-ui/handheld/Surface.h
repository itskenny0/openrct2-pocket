// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/terraform/SurfaceSetStyleAction.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/TerrainSurfaceObject.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <optional>
#include <string>

namespace OpenRCT2::Ui::Handheld::Surface
{
    inline ObjectEntryIndex Next(ObjectEntryIndex current)
    {
        const int start = current == kObjectEntryIndexNull ? -1 : current;
        for (int step = 1; step <= kMaxTerrainSurfaceObjects; ++step)
        {
            const auto index = static_cast<ObjectEntryIndex>((start + step) % kMaxTerrainSurfaceObjects);
            if (GetContext()->GetObjectManager().GetLoadedObject<TerrainSurfaceObject>(index))
                return index;
        }
        return kObjectEntryIndexNull;
    }

    inline std::string Name(ObjectEntryIndex index)
    {
        const auto* object = GetContext()->GetObjectManager().GetLoadedObject<TerrainSurfaceObject>(index);
        return object ? object->GetName() : "No surface loaded";
    }

    inline GameActions::Result Action(std::optional<CoordsXY> tile, ObjectEntryIndex index, bool execute)
    {
        // Never let the native range clamp redirect an absent/off-map target.
        const auto max = GetMapSizeMaxXY();
        const bool valid = tile && tile->x >= 32 && tile->y >= 32 && tile->x <= max.x && tile->y <= max.y && tile->x % 32 == 0
            && tile->y % 32 == 0;
        const auto* surface = valid ? MapGetSurfaceElementAt(*tile) : nullptr;
        if (!surface || index == kObjectEntryIndexNull)
        {
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = !surface ? "Move onto land." : "No surface loaded.";
            return result;
        }
        auto action = GameActions::SurfaceSetStyleAction(
            { tile->x, tile->y, tile->x, tile->y }, index, kObjectEntryIndexNull, surface->getPrimarySurfaceColour(),
            surface->getPrimaryEdgeColour());
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::Surface
