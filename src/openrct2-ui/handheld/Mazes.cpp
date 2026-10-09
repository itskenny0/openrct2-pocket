// SPDX-License-Identifier: GPL-3.0-only
#include "Mazes.h"

#include "Currency.h"
#include "Panel.h"

#include <openrct2/Context.h>
#include <openrct2/actions/ride/MazeSetTrackAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/object/RideObject.h>
#include <openrct2/ride/FlatRidePlacement.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
namespace OpenRCT2::Ui::Handheld::Mazes
{
    namespace GA = GameActions;
    static bool active{}, busy{}, placed{}, removeChoice{}, createdHere{}, camera{};
    static uint64_t revision{};
    static int offset{};
    static Direction direction{};
    static std::optional<RideManagement::Target> target;
    static std::optional<CoordsXYZD> cursor;
    static std::string feedback;
    struct Cell
    {
        CoordsXYZ location;
        uint16_t walls;
        bool operator==(const Cell&) const = default;
    };
    struct Review
    {
        std::vector<Cell> cells;
        money64 cost;
        bool noMoney;
    };
    static std::optional<Review> review;
    static GA::Result Invalid(const char* text)
    {
        GA::Result r;
        r.error = GA::Status::invalidParameters;
        r.errorMessage = text;
        return r;
    }
    bool Eligible(ObjectEntryIndex index)
    {
        const auto* entry = GetRideEntryByIndex(index);
        return entry
            && std::find(std::begin(entry->ride_type), std::end(entry->ride_type), RIDE_TYPE_MAZE) != std::end(entry->ride_type)
            && FlatRidePlacement::StationObject() != kObjectEntryIndexNull
            && (getGameState().cheats.ignoreResearchStatus
                || (RideTypeIsInvented(RIDE_TYPE_MAZE) && RideEntryIsInvented(index)));
    }
    std::vector<PickerEntry> Entries()
    {
        std::vector<PickerEntry> result;
        for (ObjectEntryIndex i = 0; i < kMaxRideObjects; ++i)
            if (Eligible(i))
                if (const auto* object = GetContext()->GetObjectManager().GetLoadedObject<RideObject>(i))
                    result.push_back(
                        { { ObjectType::ride, i, "maze:" + std::to_string(i) },
                          "Maze / " + object->GetName(),
                          ZoomLevel{ 1 } });
        return result;
    }
    void End()
    {
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        if (createdHere && ride && ride->mazeTiles == 0 && !busy)
        {
            GA::RideDemolishAction remove(ride->id, GA::RideModifyType::demolish);
            remove.SetFlags({ GA::CommandFlag::allowDuringPaused });
            GA::Execute(&remove, getGameState());
        }
        createdHere = camera = false;
        MapSelection::invalidate();
        gMapSelectFlags.unset(MapSelectFlag::enable, MapSelectFlag::enableArrow);
        active = busy = placed = false;
        target.reset();
        cursor.reset();
        review.reset();
        offset = 0;
        direction = 0;
        feedback.clear();
        ++revision;
    }
    bool Active()
    {
        return active;
    }
    bool Busy()
    {
        return busy;
    }
    bool Reviewing()
    {
        return review.has_value();
    }
    bool CameraMode()
    {
        return camera && !review;
    }
    void ToggleCamera()
    {
        if (!review)
            camera = !camera;
    }
    std::optional<RideManagement::Target> Target()
    {
        return target;
    }
    std::optional<CoordsXYZD> Cursor()
    {
        return cursor;
    }
    bool CanResume(const RideManagement::Target& t)
    {
        const auto* ride = RideManagement::Resolve(t);
        return ride && ride->type == RIDE_TYPE_MAZE && ride->status == RideStatus::closed && Eligible(ride->subtype)
            && !ride->flags.has(RideFlag::indestructibleTrack);
    }
    bool Resume(const RideManagement::Target& t)
    {
        if (!CanResume(t))
            return false;
        End();
        target = t;
        active = true;
        // Retain values rather than map pointers. Native wall masks remain opaque.
        for (int y = 32; y < getGameState().mapSize.y * 32 - 32; y += 32)
            for (int x = 32; x < getGameState().mapSize.x * 32 - 32; x += 32)
                for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                    if (!e->isGhost() && e->getRideIndex() == t.id && e->getTrackType() == TrackElemType::maze)
                    {
                        cursor = CoordsXYZD{ x, y, e->getBaseZ(), direction };
                        placed = true;
                        return true;
                    }
        return true;
    }
    bool Begin(ObjectEntryIndex index)
    {
        if (!Eligible(index))
            return false;
        End();
        GA::RideCreateAction action(
            RIDE_TYPE_MAZE, index, 0, 0, FlatRidePlacement::StationObject(), RideInspection::every30Minutes);
        if (GA::Query(&action, getGameState()).error != GA::Status::ok)
            return false;
        active = busy = createdHere = true;
        const auto token = revision;
        action.SetCallback([token](const auto*, const GA::Result* r) {
            if (token != revision)
            {
                if (r->error == GA::Status::ok)
                {
                    GA::RideDemolishAction remove(r->template getData<RideId>(), GA::RideModifyType::demolish);
                    remove.SetFlags({ GA::CommandFlag::allowDuringPaused });
                    GA::Execute(&remove, getGameState());
                }
                return;
            }
            busy = false;
            if (r->error == GA::Status::ok)
                target = RideManagement::Identify(r->template getData<RideId>());
            else
            {
                active = false;
                feedback = r->getErrorMessage();
            }
        });
        GA::Execute(&action, getGameState());
        return true;
    }
    static CoordsXYZD Next()
    {
        auto loc = *cursor;
        if (placed)
        {
            loc.x += CoordsDirectionDelta[direction].x / 2;
            loc.y += CoordsDirectionDelta[direction].y / 2;
        }
        loc.direction = direction;
        return loc;
    }
    static std::vector<Cell> Snapshot()
    {
        std::vector<Cell> cells;
        if (!cursor || !target)
            return cells;
        // Fill can close walls in each neighbouring tile; review all affected native entries.
        const auto tile = cursor->toTileStart();
        for (int y = -32; y <= 32; y += 32)
            for (int x = -32; x <= 32; x += 32)
                if (MapIsLocationValid(tile + CoordsXY{ x, y }))
                    for (auto* e : TileElementsView<TrackElement>(tile + CoordsXY{ x, y }))
                        if (!e->isGhost() && e->getRideIndex() == target->id && e->getTrackType() == TrackElemType::maze)
                            cells.push_back({ { tile + CoordsXY{ x, y }, e->getBaseZ() }, e->getMazeEntry() });
        return cells;
    }
    static GA::Result Apply(GA::MazeBuildMode mode, bool execute)
    {
        if (!active || busy || !target || !CanResume(*target) || !cursor || (mode != GA::MazeBuildMode::build && !placed))
            return Invalid("Maze selection unavailable");
        auto loc = Next();
        GA::MazeSetTrackAction action(loc, !placed, target->id, mode);
        auto q = GA::Query(&action, getGameState());
        if (!execute || q.error != GA::Status::ok)
        {
            if (execute)
                feedback = q.getErrorMessage();
            return q;
        }
        busy = true;
        const auto token = revision;
        action.SetCallback([token, loc](const auto*, const GA::Result* result) {
            if (token != revision)
                return;
            busy = false;
            feedback = result->error == GA::Status::ok ? "" : result->getErrorMessage();
            if (result->error == GA::Status::ok)
            {
                target = RideManagement::Identify(target->id);
                cursor = loc;
                placed = true;
            }
        });
        return GA::Execute(&action, getGameState());
    }
    GA::Result Build(bool execute)
    {
        return review ? Invalid("Finish removal review") : Apply(GA::MazeBuildMode::build, execute);
    }
    GA::Result Move()
    {
        return review ? Invalid("Finish removal review") : Apply(GA::MazeBuildMode::move, true);
    }
    GA::Result ReviewRemoval()
    {
        const auto q = Apply(GA::MazeBuildMode::fill, false);
        if (q.error == GA::Status::ok)
        {
            review = Review{ Snapshot(), q.cost, getGameState().park.flags.has(ParkFlag::noMoney) };
            removeChoice = false;
        }
        feedback = q.error == GA::Status::ok ? "" : q.getErrorMessage();
        return q;
    }
    void ChooseRemoval()
    {
        if (review)
            removeChoice = !removeChoice;
    }
    void Cancel()
    {
        review.reset();
        removeChoice = false;
        feedback.clear();
    }
    GA::Result Confirm()
    {
        if (!review)
            return Build(true);
        if (!removeChoice)
        {
            Cancel();
            return {};
        }
        const auto baseline = *review;
        const auto q = Apply(GA::MazeBuildMode::fill, false);
        if (Snapshot() != baseline.cells || baseline.noMoney != getGameState().park.flags.has(ParkFlag::noMoney)
            || q.error != GA::Status::ok || q.cost != baseline.cost)
        {
            Cancel();
            feedback = "Maze changed; review again";
            return Invalid("Maze changed; review again");
        }
        Cancel();
        return Apply(GA::MazeBuildMode::fill, true);
    }
    void Turn(int delta)
    {
        if (!review)
        {
            direction = (direction + delta) & 3;
            feedback.clear();
        }
        else
            ChooseRemoval();
    }
    void Height(int delta)
    {
        if (!placed && !review)
            offset = std::clamp(offset + delta * 16, -256, 512);
    }
    void Update(std::optional<CoordsXY> tile)
    {
        if (!active)
            return;
        if (!placed && !busy)
            cursor = tile ? std::optional<CoordsXYZD>{ { *tile, (MapGetHighestZ(*tile) + 15) / 16 * 16 + offset, direction } }
                          : std::nullopt;
        MapSelection::invalidate();
        gMapSelectFlags.unset(MapSelectFlag::enable, MapSelectFlag::enableArrow);
        if (cursor)
        {
            const auto loc = Next();
            gMapSelectPositionA = gMapSelectPositionB = loc.toTileStart();
            gMapSelectType = MapSelectType::full;
            gMapSelectFlags.set(MapSelectFlag::enable, MapSelectFlag::enableArrow);
            gMapSelectArrowPosition = { loc.x + 8, loc.y + 8, loc.z };
            gMapSelectArrowDirection = direction;
            MapSelection::invalidate();
        }
    }
    void DrawPanel(Drawing::RenderTarget& rt, ScreenSize screen)
    {
        using namespace Concept;
        const ScreenCoordsXY p{ 8, 36 };
        Box(rt, p, { 164, 206 });
        Heading(rt, p + ScreenCoordsXY{ 10, 12 }, review ? "Remove maze cell?" : placed ? "Edit maze" : "Place maze");
        Rule(rt, p + ScreenCoordsXY{ 8, 38 }, 148);
        Box(rt, p + ScreenCoordsXY{ 8, 48 }, { 148, 28 }, true);
        DrawStrongText(
            rt, p + ScreenCoordsXY{ 14, 57 },
            review ? (removeChoice ? "{BLACK}Remove cell" : "{BLACK}Cancel")
                   : (camera ? "{BLACK}Camera controls" : "{BLACK}Half-tile segments"),
            136);
        static const char* directions[]{ "West", "South", "East", "North" };
        DrawModernText(rt, p + ScreenCoordsXY{ 10, 86 }, "{MUTED}Direction " + std::string(directions[direction]));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 108 }, "{MUTED}Height " + (cursor ? std::to_string(cursor->z / 8) : "Unavailable"));
        DrawModernText(rt, p + ScreenCoordsXY{ 10, 132 }, review ? "{MUTED}L/R Select option" : "{MUTED}X Move | Y Remove");
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 154 },
            review       ? "{MUTED}A Apply | B Cancel"
                : placed ? "{MUTED}B Finish / access"
                         : "{MUTED}L2/R2 Height");
        DrawModernText(rt, p + ScreenCoordsXY{ 10, 178 }, "{MUTED}SEL Camera | START");
        DrawActionStrip(rt, screen);
        DrawCommand(rt, { 8, screen.height - 21 }, 17, "A", review ? "Select" : "Build");
        DrawCommand(rt, { 112, screen.height - 21 }, 17, "B", review ? "Cancel" : "Leave");
        DrawCommand(rt, { 220, screen.height - 21 }, 30, "L/R", review ? "Choice" : (camera ? "Zoom" : "Direction"));
        if (!review)
            DrawCommand(rt, { 374, screen.height - 21 }, 17, "X", camera ? "Rotate" : "Move");
        const auto q = review ? Apply(GA::MazeBuildMode::fill, false) : Build(false);
        if (!feedback.empty())
            DrawContextMessage(rt, Layout::ContextAnchor(screen, kContextSize, true), feedback);
        else if (q.error == GA::Status::ok)
            DrawCostBadge(rt, Layout::ContextAnchor(screen, kContextSize, true), FormatMoney(q.cost));
        else
            DrawContextMessage(
                rt, Layout::ContextAnchor(screen, kContextSize, true), feedback.empty() ? q.getErrorMessage() : feedback);
    }
} // namespace OpenRCT2::Ui::Handheld::Mazes
