// SPDX-License-Identifier: GPL-3.0-only
#include "Paths.h"

#include <algorithm>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
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

    struct Preview
    {
        CoordsXYZ position;
        FootpathSlope slope;
        ObjectEntryIndex surface, railings;
        PathConstructFlags flags;
        bool operator==(const Preview&) const = default;
    };
    static std::optional<Preview> ghost, suspendedGhost, checkedPreview;
    static GameActions::Result previewResult;
    static uint32_t checkedTick{};
    static money64 checkedCash{};
    static uint8_t checkedPause{};

    static std::optional<Preview> RequestedPreview()
    {
        if (!active || removing || !target || !available)
            return {};
        auto type = gFootpathSelection.getSelectedSurface();
        PathConstructFlags flags = IsQueue() ? PathConstructFlag::IsQueue : 0;
        if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
        {
            type = gFootpathSelection.legacyPath;
            flags |= PathConstructFlag::IsLegacyPathObject;
        }
        if (type == kObjectEntryIndexNull)
            return {};
        auto position = *target;
        position.z = fixedHeight.value_or(target->z);
        return Preview{ position, fixedSlope.value_or(slope), type, gFootpathSelection.railings, flags };
    }

    static void RemoveGhost()
    {
        if (!ghost)
            return;
        const auto position = ghost->position;
        ghost.reset(); // Clear ownership before native actions broadcast provisional intents.
        auto action = GameActions::FootpathRemoveAction(position);
        action.SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        auto& state = getGameState();
        const auto result = GameActions::Execute(&action, state);
        // Ownership can change after a preview was shown. The ghost-only native
        // executor can discard our temporary element even if its removal query
        // now rejects the land. It cannot select a real path with these flags.
        if (result.error == GameActions::Status::notOwned)
            action.Execute(state, state.park);
    }
    void ClearPreview()
    {
        RemoveGhost();
        suspendedGhost.reset();
        checkedPreview.reset();
    }
    void RemovePreviewTemporarily()
    {
        if (ghost)
        {
            suspendedGhost = ghost;
            RemoveGhost();
        }
    }
    void RestorePreview()
    {
        if (!suspendedGhost)
            return;
        suspendedGhost.reset();
        checkedPreview.reset();
        UpdatePreview(); // Revalidate using the real action, including cash/pause/ownership.
    }
    void UpdatePreview()
    {
        const auto requested = RequestedPreview();
        auto& state = getGameState();
        if (!requested)
        {
            ClearPreview();
            return;
        }
        if (requested == checkedPreview && checkedTick == state.currentTicks && checkedCash == state.park.cash
            && checkedPause == gGamePaused)
            return;
        ClearPreview();
        previewResult = Action(false);
        if (previewResult.error == GameActions::Status::ok)
        {
            auto action = GameActions::FootpathPlaceAction(
                requested->position, requested->slope, requested->surface, requested->railings, kInvalidDirection,
                requested->flags);
            action.SetFlags(
                { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
                  GameActions::CommandFlag::allowDuringPaused });
            const auto result = GameActions::Execute(&action, state);
            // Native ghosts intentionally cannot obscure/convert an existing real path.
            if (result.error == GameActions::Status::ok)
                ghost = requested;
        }
        checkedPreview = requested;
        checkedTick = state.currentTicks;
        checkedCash = state.park.cash;
        checkedPause = gGamePaused;
    }

    void Begin(bool queue, bool remove)
    {
        ClearPreview();
        available = Windows::WindowFootpathSelectDefault();
        gFootpathSelection.isQueueSelected = queue;
        active = true;
        removing = remove;
        target.reset();
        FollowGround();
    }
    void End()
    {
        ClearPreview();
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
        ClearPreview();
        removing = !removing;
    }

    void FollowGround()
    {
        ClearPreview();
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
        ClearPreview();
        fixedHeight = next;
        if (!fixedSlope)
            fixedSlope = slope;
        return true;
    }
    bool CycleSlope()
    {
        if (!target)
            return false;
        ClearPreview();
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
        ClearPreview();
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
        ClearPreview();
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
        {
            ClearPreview();
            return;
        }
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
        const auto requested = RequestedPreview();
        if (!execute && requested && requested == checkedPreview && checkedTick == getGameState().currentTicks
            && checkedCash == getGameState().park.cash && checkedPause == gGamePaused)
            return previewResult;
        ClearPreview();
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
