// SPDX-License-Identifier: GPL-3.0-only
#include "Paths.h"

#include "ConstructionPanel.h"
#include "RideAccess.h"

#include <algorithm>
#include <cstring>
#include <openrct2-ui/interface/Window.h>
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
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/EntranceElement.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>

namespace OpenRCT2::Ui::Handheld::Paths
{
    static bool active{}, removing{}, available{};
    static bool connected{}, connectedBusy{};
    static std::optional<CoordsXYZ> endpoint;
    static FootpathSlope endpointSlope;
    struct AccessStart
    {
        RideManagement::Target ride;
        StationIndex station;
        bool exit;
        CoordsXYZD outside;
    };
    static std::optional<AccessStart> accessStart;
    static bool firstLanding{};
    struct BuiltPath
    {
        CoordsXYZ position;
        std::array<uint8_t, 16> baseline;
        std::optional<CoordsXYZ> previous;
        FootpathSlope previousSlope;
    };
    static std::vector<BuiltPath> history;
    static std::optional<std::array<uint8_t, 16>> PathBaseline(CoordsXYZ position)
    {
        const auto* path = MapGetFootpathElement(position);
        if (!path || path->isGhost())
            return {};
        static_assert(sizeof(PathElement) == 16);
        std::array<uint8_t, 16> result;
        std::memcpy(result.data(), path, result.size());
        return result;
    }
    static bool HistoryValid()
    {
        return std::all_of(
            history.begin(), history.end(), [](const auto& built) { return PathBaseline(built.position) == built.baseline; });
    }
    static void RefreshHistoryBaselines()
    {
        for (auto& built : history)
        {
            const auto baseline = PathBaseline(built.position);
            if (!baseline)
            {
                history.clear();
                return;
            }
            built.baseline = *baseline;
        }
    }
    static bool AccessValid()
    {
        return !accessStart
            || RideAccess::PointValid(accessStart->ride, accessStart->station, accessStart->exit, accessStart->outside);
    }
    static Direction connectedDirection{};
    static int connectedGrade{};
    static std::optional<CoordsXYZ> target;
    static FootpathSlope slope;
    static bool targetIsPath{};
    static bool heightPlane{};
    static std::optional<CoordsXY> accessSource;
    static uint64_t accessRevision{};
    static std::optional<int32_t> fixedHeight;
    static std::optional<FootpathSlope> fixedSlope;

    struct Preview
    {
        CoordsXYZ position;
        FootpathSlope slope;
        ObjectEntryIndex surface, railings;
        PathConstructFlags flags;
        Direction incoming;
        bool operator==(const Preview&) const = default;
    };
    static std::optional<Preview> ghost, suspendedGhost, checkedPreview;
    static GameActions::Result previewResult;
    static uint32_t checkedTick{};
    static money64 checkedCash{};
    static uint8_t checkedPause{};

    static Direction ConnectionDirection(CoordsXYZ position, FootpathSlope selectedSlope)
    {
        if (!heightPlane)
            return kInvalidDirection;
        auto connects = [&](Direction d) {
            if (selectedSlope.type == FootpathSlopeType::sloped && d != selectedSlope.direction
                && DirectionReverse(d) != selectedSlope.direction)
                return false;
            const auto z = position.z
                + (selectedSlope.type == FootpathSlopeType::sloped && DirectionReverse(d) == selectedSlope.direction
                       ? kPathHeightStep
                       : 0);
            const CoordsXY source = CoordsXY(position) - CoordsDirectionDelta[d];
            if (!MapIsLocationValid(source))
                return false;
            for (auto* path : TileElementsView<PathElement>(source))
                if (!path->isGhost() && FootpathIsZAndDirectionValid(*path, z / kCoordsZStep, DirectionReverse(d)))
                    return true;
            for (auto* entrance : TileElementsView<EntranceElement>(source))
                if (!entrance->isGhost() && entrance->getEntranceType() != EntranceType::parkEntrance
                    && entrance->getBaseZ() == z && DirectionReverse(entrance->getDirection()) == d)
                    return true;
            return false;
        };
        if (accessSource)
            for (Direction d : kAllDirections)
                if (CoordsXY(position) - CoordsDirectionDelta[d] == *accessSource && connects(d))
                    return d;
        for (Direction d : kAllDirections)
            if (connects(d))
                return d;
        return kInvalidDirection;
    }
    static std::optional<Preview> RequestedPreview()
    {
        if (!active || removing || !target || !available || (connected && ((!endpoint && !firstLanding) || connectedBusy)))
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
        const auto selectedSlope = fixedSlope.value_or(slope);
        return Preview{ position, selectedSlope,
                        type,     gFootpathSelection.railings,
                        flags,    ConnectionDirection(position, selectedSlope) };
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
                requested->position, requested->slope, requested->surface, requested->railings, requested->incoming,
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
        connected = connectedBusy = false;
        endpoint.reset();
        history.clear();
        accessStart.reset();
        firstLanding = false;
        heightPlane = false;
        accessSource.reset();
        ++accessRevision;
        removing = remove;
        target.reset();
        FollowGround();
    }
    void End()
    {
        ClearPreview();
        active = false;
        connected = connectedBusy = false;
        endpoint.reset();
        history.clear();
        accessStart.reset();
        firstLanding = false;
        heightPlane = false;
        accessSource.reset();
        ++accessRevision;
        target.reset();
    }
    void BeginConnected(bool queue)
    {
        Begin(queue);
        connected = true;
        connectedDirection = 0;
        connectedGrade = 0;
    }
    bool IsConnected()
    {
        return active && connected;
    }
    bool ConnectedReady()
    {
        return IsConnected() && (endpoint.has_value() || firstLanding);
    }
    static bool RefreshConnected()
    {
        if (!AccessValid())
        {
            target.reset();
            return false;
        }
        if (firstLanding && accessStart)
        {
            target = CoordsXYZ(accessStart->outside);
            fixedHeight = target->z;
            fixedSlope = FootpathSlope{};
            accessSource = CoordsXY(*target) + CoordsDirectionDelta[accessStart->outside.direction];
            heightPlane = true;
            return true;
        }
        if (!endpoint)
            return false;
        const auto* path = MapGetFootpathElement(*endpoint);
        if (!path || path->isGhost() || path->isSloped() != (endpointSlope.type == FootpathSlopeType::sloped)
            || (path->isSloped() && path->getSlopeDirection() != endpointSlope.direction))
        {
            target.reset();
            return false;
        }
        auto z = endpoint->z;
        if (path->isSloped() && path->getSlopeDirection() == connectedDirection)
            z += kPathHeightStep;
        const CoordsXY next = CoordsXY(*endpoint) + CoordsDirectionDelta[connectedDirection];
        if (!MapIsLocationValid(next) || next.x < 32 || next.y < 32 || next.x > GetMapSizeMaxXY().x
            || next.y > GetMapSizeMaxXY().y
            || !FootpathIsZAndDirectionValid(*path, z / kCoordsZStep, DirectionReverse(connectedDirection)))
        {
            target.reset();
            return false;
        }
        target = CoordsXYZ{ next, z - (connectedGrade < 0 ? kPathHeightStep : 0) };
        fixedHeight = target->z;
        fixedSlope = FootpathSlope{ connectedGrade == 0 ? FootpathSlopeType::flat : FootpathSlopeType::sloped,
                                    connectedGrade < 0 ? DirectionReverse(connectedDirection) : connectedDirection };
        accessSource = CoordsXY(*endpoint);
        heightPlane = true;
        return true;
    }
    bool BeginConnectedAccess(const RideManagement::Target& ride, StationIndex station, bool exit, CoordsXYZD outside)
    {
        if (!RideAccess::PointValid(ride, station, exit, outside))
            return false;
        BeginConnected(!exit);
        accessStart = AccessStart{ ride, station, exit, outside };
        firstLanding = true;
        connectedDirection = DirectionReverse(outside.direction);
        return RefreshConnected();
    }
    std::optional<UndoDraft> BeginUndo()
    {
        ClearPreview();
        if (!IsConnected() || connectedBusy || history.empty() || !AccessValid() || !HistoryValid())
            return {};
        const auto& last = history.back();
        if (PathBaseline(last.position) != last.baseline)
            return {};
        GameActions::FootpathRemoveAction action(last.position);
        const auto q = GameActions::Query(&action, getGameState());
        if (q.error != GameActions::Status::ok)
            return {};
        return UndoDraft{ last.position, last.baseline, accessRevision, q.cost, getGameState().park.cash };
    }
    GameActions::Result Undo(const UndoDraft& d, bool execute)
    {
        ClearPreview();
        GameActions::Result invalid;
        invalid.error = GameActions::Status::invalidParameters;
        invalid.errorMessage = "Path changed; reopen undo review";
        if (!IsConnected() || connectedBusy || history.empty() || !AccessValid() || !HistoryValid()
            || d.revision != accessRevision || history.back().position != d.position || PathBaseline(d.position) != d.baseline
            || getGameState().park.cash != d.cash)
            return invalid;
        GameActions::FootpathRemoveAction action(d.position);
        auto q = GameActions::Query(&action, getGameState());
        if (q.error != GameActions::Status::ok || q.cost != d.cost)
            return q.error == GameActions::Status::ok ? invalid : q;
        if (!execute)
            return q;
        connectedBusy = true;
        action.SetCallback([revision = d.revision](const auto*, const GameActions::Result* result) {
            if (!active || !connected || accessRevision != revision)
                return;
            connectedBusy = false;
            if (result->error != GameActions::Status::ok || history.empty())
                return;
            const auto previous = history.back();
            history.pop_back();
            endpoint = previous.previous;
            endpointSlope = previous.previousSlope;
            firstLanding = !endpoint && accessStart.has_value();
            ++accessRevision;
            RefreshHistoryBaselines();
            RefreshConnected();
        });
        const auto result = GameActions::Execute(&action, getGameState());
        if (result.error != GameActions::Status::ok)
            connectedBusy = false;
        return result;
    }
    bool TurnConnected()
    {
        if (!IsConnected() || connectedBusy || firstLanding)
            return false;
        ClearPreview();
        connectedDirection = DirectionNext(connectedDirection);
        ++accessRevision;
        RefreshConnected();
        return true;
    }
    bool SlopeConnected(int delta)
    {
        if (!ConnectedReady() || connectedBusy || firstLanding)
            return false;
        ClearPreview();
        connectedGrade = std::clamp(connectedGrade + delta, -1, 1);
        ++accessRevision;
        RefreshConnected();
        return true;
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

    void UseHeightPlane(bool enabled, std::optional<CoordsXY> source)
    {
        ClearPreview();
        heightPlane = enabled;
        accessSource = source;
        ++accessRevision;
    }
    static void RefocusHeight()
    {
        auto* main = WindowGetMain();
        if (!heightPlane || !target || !fixedHeight || !main || !main->viewport)
            return;
        const CoordsXYZ centre{ target->x + 16, target->y + 16, *fixedHeight };
        const auto cursor = ScreenCoordsXY{ ContextGetWidth() / 2, ContextGetHeight() / 2 } - main->viewport->pos;
        main->savedViewPos = Translate3DTo2DWithZ(main->viewport->rotation, centre)
            - ScreenCoordsXY{ main->viewport->zoom.ApplyTo(cursor.x), main->viewport->zoom.ApplyTo(cursor.y) };
        main->viewport->viewPos = main->savedViewPos;
        main->flags.unset(WindowFlag::scrollingToLocation);
        main->invalidate();
    }
    void FollowGround()
    {
        ClearPreview();
        fixedHeight.reset();
        fixedSlope.reset();
    }
    bool SetPlacementHeight(int32_t z)
    {
        if (!active || z < kFootpathMinHeight || z > kFootpathMaxHeight || z % kCoordsZStep)
            return false;
        ClearPreview();
        fixedHeight = z;
        fixedSlope = FootpathSlope{ FootpathSlopeType::flat, 0 };
        return true;
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
        RefocusHeight();
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
        const char* directions[] = { "NW", "NE", "SE", "SW" };
        if (IsConnected())
            return firstLanding ? "A: build landing"
                : !endpoint     ? "Choose existing path / A start"
                                : std::string(directions[connectedDirection])
                    + (connectedGrade > 0       ? " / Rise"
                           : connectedGrade < 0 ? " / Descend"
                                                : " / Flat")
                    + " / Y turn";
        if (removing)
            return "Remove selected path";
        if (!fixedHeight)
            return "Ground / auto slope";
        const auto selectedSlope = fixedSlope.value_or(slope);
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
        if (ConnectedReady())
        {
            RefreshConnected();
            return;
        }
        if (heightPlane && fixedHeight)
        {
            const auto xy = ScreenGetMapXYWithZ(cursor, *fixedHeight);
            if (!xy)
            {
                target.reset();
                ClearPreview();
                return;
            }
            const CoordsXY tile{ xy->x & ~31, xy->y & ~31 };
            InteractionInfo info;
            info.Loc = tile;
            info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(tile));
            info.interactionType = ViewportInteractionItem::terrain;
            if (removing)
                for (auto* path : TileElementsView<PathElement>(tile))
                    if (!path->isGhost() && path->getBaseZ() == *fixedHeight)
                    {
                        info.Element = reinterpret_cast<TileElement*>(path);
                        info.interactionType = ViewportInteractionItem::footpath;
                        break;
                    }
            UpdateTargetFromInfo(info);
            return;
        }
        UpdateTargetFromInfo(
            GetMapCoordinatesFromPos(cursor, { ViewportInteractionItem::terrain, ViewportInteractionItem::footpath }));
    }
    void UpdateTargetFromInfo(const InteractionInfo& info)
    {
        if (ConnectedReady())
        {
            RefreshConnected();
            return;
        }
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
        if (IsConnected())
        {
            ClearPreview();
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            if (connectedBusy)
            {
                result.errorMessage = "Path request pending";
                return result;
            }
            if (!HistoryValid())
            {
                result.errorMessage = "Built path changed; B to restart";
                return result;
            }
            if (firstLanding && !RefreshConnected())
            {
                result.errorMessage = "Ride access changed; restart from inspection";
                return result;
            }
            if (!endpoint && (!firstLanding || MapGetFootpathElement(*target)))
            {
                if (firstLanding)
                {
                    targetIsPath = true;
                    slope = FootpathSlope{};
                }
                result.errorMessage = "A: choose an existing path endpoint";
                if (!target || !targetIsPath)
                    return result;
                const auto* path = MapGetFootpathElement(*target);
                if (!path || path->isGhost())
                    return result;
                result.error = GameActions::Status::ok;
                if (execute)
                {
                    ClearPreview();
                    endpoint = *target;
                    endpointSlope = { path->isSloped() ? FootpathSlopeType::sloped : FootpathSlopeType::flat,
                                      path->getSlopeDirection() };
                    firstLanding = false;
                    history.clear();
                    ++accessRevision;
                    RefreshConnected();
                }
                return result;
            }
            if (!RefreshConnected())
            {
                ClearPreview();
                result.errorMessage = "Endpoint changed or direction unavailable; B to restart";
                return result;
            }
        }

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
            placement, fixedSlope.value_or(slope), type, gFootpathSelection.railings,
            ConnectionDirection(placement, fixedSlope.value_or(slope)), flags);
        if (execute && heightPlane)
        {
            const auto revision = accessRevision;
            const auto selectedSlope = fixedSlope.value_or(slope);
            const bool wasConnected = connected;
            const bool existed = PathBaseline(placement).has_value();
            const auto previous = endpoint;
            const auto previousSlope = endpointSlope;
            if (wasConnected)
                connectedBusy = true;
            action.SetCallback([revision, placement, selectedSlope, wasConnected, existed, previous,
                                previousSlope](const auto*, const GameActions::Result* result) {
                if (active && heightPlane && revision == accessRevision && result->error == GameActions::Status::ok)
                {
                    accessSource = placement;
                    if (wasConnected && connected)
                    {
                        const auto baseline = PathBaseline(placement);
                        if (!existed && baseline)
                            history.push_back({ placement, *baseline, previous, previousSlope });
                        else
                            history.clear(); // Joining/converting existing paths is never undone by deleting them.
                        RefreshHistoryBaselines();
                        firstLanding = false;
                        endpoint = placement;
                        endpointSlope = selectedSlope;
                        RefreshConnected();
                        RefocusHeight();
                    }
                }
                if (revision == accessRevision && wasConnected)
                    connectedBusy = false;
            });
        }
        const auto result = execute ? GameActions::Execute(&action, getGameState())
                                    : GameActions::Query(&action, getGameState());
        if (execute && result.error != GameActions::Status::ok && connected)
            connectedBusy = false;
        return result;
    }
    void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY position)
    {
        if (!IsConnected())
        {
            DrawPathOverlay(rt, position, SurfaceName(), IsQueue(), IsRemoving(), PlacementName());
            return;
        }
        DrawConstructionOverlay(
            rt, position, "Connected paths", SurfaceName(),
            firstLanding ? (IsQueue() ? "Queue / A build" : "Normal / A build")
                         : (IsQueue() ? "Queue / A extend" : "Normal / A extend"),
            PlacementName(),
            ConnectedReady() && target ? "Height " + std::to_string(target->z / kCoordsZStep) : "A: choose existing path");
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
