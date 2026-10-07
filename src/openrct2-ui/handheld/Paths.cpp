// SPDX-License-Identifier: GPL-3.0-only
#include "Paths.h"

#include <algorithm>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/footpath/FootpathRemoveAction.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/FootpathObject.h>
#include <openrct2/object/FootpathSurfaceObject.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>

namespace OpenRCT2::Ui::Handheld::Paths
{
    static bool active{}, removing{}, available{};
    static std::optional<CoordsXYZ> target;
    static FootpathSlope slope;
    static bool targetIsPath{};
    static std::optional<int32_t> fixedHeight;
    static std::optional<FootpathSlope> fixedSlope;

    void Begin(bool queue, bool remove)
    {
        available = Windows::WindowFootpathSelectDefault();
        gFootpathSelection.isQueueSelected = queue;
        active = true;
        removing = remove;
        target.reset();
        FollowGround();
    }
    void End()
    {
        active = false;
        target.reset();
    }
    bool IsActive()
    {
        return active;
    }
    bool IsRemoving()
    {
        return removing;
    }
    bool IsQueue()
    {
        return gFootpathSelection.isQueueSelected;
    }
    void ToggleRemove()
    {
        removing = !removing;
    }

    void FollowGround()
    {
        fixedHeight.reset();
        fixedSlope.reset();
    }
    bool ChangeHeight(int direction)
    {
        if (!target)
            return false;
        const auto height = fixedHeight.value_or(target->z);
        const auto next = std::clamp(height + direction * kPathHeightStep, kFootpathMinHeight, kFootpathMaxHeight);
        if (next == height)
            return false;
        fixedHeight = next;
        if (!fixedSlope)
            fixedSlope = slope;
        return true;
    }
    bool CycleSlope()
    {
        if (!target)
            return false;
        if (!fixedHeight)
            fixedHeight = target->z;
        auto next = fixedSlope.value_or(slope);
        next.type = next.type == FootpathSlopeType::sloped ? FootpathSlopeType::flat : FootpathSlopeType::sloped;
        fixedSlope = next;
        return true;
    }
    bool RotateSlope()
    {
        if (!target)
            return false;
        if (!fixedHeight)
            fixedHeight = target->z;
        auto next = fixedSlope.value_or(slope);
        next.type = FootpathSlopeType::sloped;
        next.direction = (next.direction + 1) & 3;
        fixedSlope = next;
        return true;
    }
    std::string PlacementName()
    {
        if (removing)
            return "Remove selected path";
        if (!fixedHeight)
            return "Ground / auto slope";
        const auto selectedSlope = fixedSlope.value_or(slope);
        const char* directions[] = { "NW", "NE", "SE", "SW" };
        return "Height " + std::to_string(*fixedHeight / kCoordsZStep) + " / "
            + (selectedSlope.type == FootpathSlopeType::sloped ? std::string("Rise ") + directions[selectedSlope.direction & 3]
                                                               : "Flat");
    }

    void CycleSurface()
    {
        if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
        {
            auto& objects = GetContext()->GetObjectManager();
            for (int offset = 1; offset <= kMaxPathObjects; ++offset)
            {
                const auto index = static_cast<ObjectEntryIndex>((gFootpathSelection.legacyPath + offset) % kMaxPathObjects);
                const auto* object = objects.GetLoadedObject<FootpathObject>(index);
                if (object && !(object->GetPathSurfaceDescriptor().flags & FOOTPATH_ENTRY_FLAG_SHOW_ONLY_IN_SCENARIO_EDITOR))
                {
                    gFootpathSelection.legacyPath = index;
                    return;
                }
            }
            return;
        }
        auto& selected = IsQueue() ? gFootpathSelection.queueSurface : gFootpathSelection.normalSurface;
        for (int offset = 1; offset <= kMaxFootpathSurfaceObjects; ++offset)
        {
            const auto index = static_cast<ObjectEntryIndex>((selected + offset) % kMaxFootpathSurfaceObjects);
            const auto* object = GetPathSurfaceEntry(index);
            if (object && !(object->Flags & FOOTPATH_ENTRY_FLAG_SHOW_ONLY_IN_SCENARIO_EDITOR)
                && IsQueue() == ((object->Flags & FOOTPATH_ENTRY_FLAG_IS_QUEUE) != 0))
            {
                selected = index;
                return;
            }
        }
    }

    void UpdateTarget(ScreenCoordsXY cursor)
    {
        UpdateTargetFromInfo(
            GetMapCoordinatesFromPos(cursor, { ViewportInteractionItem::terrain, ViewportInteractionItem::footpath }));
    }
    void UpdateTargetFromInfo(const InteractionInfo& info)
    {
        target.reset();
        targetIsPath = false;
        if (!info.Element)
            return;
        const CoordsXY tile{ info.Loc.x & ~31, info.Loc.y & ~31 };
        if (info.interactionType == ViewportInteractionItem::footpath)
        {
            const auto* path = info.Element->asPath();
            if (!path || path->isGhost())
                return;
            target = CoordsXYZ{ tile, path->getBaseZ() };
            slope = { path->isSloped() ? FootpathSlopeType::sloped : FootpathSlopeType::flat, path->getSlopeDirection() };
            targetIsPath = true;
        }
        else if (info.interactionType == ViewportInteractionItem::terrain)
        {
            const auto* surface = info.Element->asSurface();
            if (!surface)
                return;
            const auto placement = FootpathGetOnTerrainPlacement(*surface);
            if (placement.slope.type == FootpathSlopeType::irregular && !fixedSlope)
                return;
            target = CoordsXYZ{ tile, placement.baseZ };
            slope = placement.slope;
        }
    }
    std::optional<CoordsXY> TargetTile()
    {
        if (!target)
            return {};
        return CoordsXY{ target->x, target->y };
    }
    GameActions::Result Action(bool execute)
    {
        if (!target || (!removing && !available) || (removing && !targetIsPath))
        {
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = removing ? "Move the cursor onto an existing path." : "No valid path placement here.";
            return result;
        }
        if (removing)
        {
            auto action = GameActions::FootpathRemoveAction(*target);
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        auto type = gFootpathSelection.getSelectedSurface();
        PathConstructFlags flags = IsQueue() ? PathConstructFlag::IsQueue : 0;
        if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
        {
            type = gFootpathSelection.legacyPath;
            flags |= PathConstructFlag::IsLegacyPathObject;
        }
        if (type == kObjectEntryIndexNull)
        {
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = "No path surface of this type is loaded.";
            return result;
        }
        auto placement = *target;
        placement.z = fixedHeight.value_or(target->z);
        auto action = GameActions::FootpathPlaceAction(
            placement, fixedSlope.value_or(slope), type, gFootpathSelection.railings, kInvalidDirection, flags);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    std::string SurfaceName()
    {
        if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
        {
            const auto* object = GetContext()->GetObjectManager().GetLoadedObject<FootpathObject>(
                gFootpathSelection.legacyPath);
            return object ? object->GetName() : "No surface loaded";
        }
        const auto* object = GetPathSurfaceEntry(gFootpathSelection.getSelectedSurface());
        return object ? object->GetName() : "No surface loaded";
    }
} // namespace OpenRCT2::Ui::Handheld::Paths
