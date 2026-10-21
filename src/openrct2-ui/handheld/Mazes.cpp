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
#include <openrct2/ride/AccessPlacementContext.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
namespace OpenRCT2::Ui::Handheld::Mazes
{
    namespace GA = GameActions;
    static bool active{}, busy{}, placed{}, removeChoice{}, createdHere{}, camera{};
    static uint64_t revision{};
    static int offset{};
    static int quadrant{};
    static std::optional<CoordsXY> hovered;
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
        CoordsXYZD location;
        std::vector<int64_t> context;
    };
    static std::optional<Review> review;
    struct PreviewRequest {RideId id;CoordsXYZD location;bool initial;GA::MazeBuildMode mode;
        std::vector<int64_t> context;money64 cost;};
    static std::optional<PreviewRequest> previewRequest,ghostRequest;
    static uint32_t previewDepth{};
    static void UpdatePreview();
    static void RemoveGhost()
    {
        if(ghostRequest){GA::MazeSetTrackAction action(ghostRequest->location,ghostRequest->initial,ghostRequest->id,ghostRequest->mode);
            action.SetPureGhost(true);action.SetFlags({GA::CommandFlag::ghost,GA::CommandFlag::noSpend,GA::CommandFlag::allowDuringPaused});
            action.Execute(getGameState(),getGameState().park);}
        ghostRequest.reset();
    }
    void ClearPreview(){previewRequest.reset();RemoveGhost();}
    void RemovePreviewTemporarily(){++previewDepth;RemoveGhost();}
    void RestorePreview(){if(!previewDepth || --previewDepth || !previewRequest)return;
        const auto old=*previewRequest;const auto* ride=GetRide(old.id);
        if(!ride || old.context!=AccessPlacementContext::Capture(*ride,old.location)){ClearPreview();return;}
        UpdatePreview();}

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
        ClearPreview();
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
        offset = quadrant = 0;
        hovered.reset();
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
        if (!review){ClearPreview();camera = !camera;UpdatePreview();}
    }
    bool Picking()
    {
        return camera && placed && !review;
    }
    static std::optional<CoordsXYZD> PickLocation()
    {
        if (!target || !CanResume(*target) || !hovered || !MapIsLocationValid(*hovered) || hovered->x % 32 || hovered->y % 32)
            return {};
        std::optional<CoordsXYZD> found;
        for (auto* e : TileElementsView<TrackElement>(*hovered))
            if (!e->isGhost() && e->getRideIndex() == target->id && e->getTrackType() == TrackElemType::maze)
            {
                found = CoordsXYZD{ hovered->x + (quadrant & 1) * 16, hovered->y + (quadrant / 2) * 16, e->getBaseZ(),
                                    direction };
                if (cursor && e->getBaseZ() == cursor->z)
                    break;
            }
        return found;
    }
    GA::Result SelectCell(bool execute)
    {
        const auto location = PickLocation();
        if (!Picking() || !location)
        {
            if (execute)
                feedback = "Point at this ride's maze";
            return Invalid("Point at this ride's maze");
        }
        if (execute)
        {
            ClearPreview();
            cursor = location;
            camera = false;
            feedback.clear();
        }
        return {};
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
        if(execute && previewRequest && previewRequest->location==loc && previewRequest->mode==mode)
            action.GuardContext(previewRequest->context,previewRequest->cost);
        auto q = GA::Query(&action, getGameState());
        if (!execute || q.error != GA::Status::ok)
        {
            if (execute){ClearPreview();feedback = q.getErrorMessage();}
            return q;
        }
        action.GuardContext(AccessPlacementContext::Capture(*RideManagement::Resolve(*target),loc),q.cost);
        ClearPreview();
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
            review = Review{ Snapshot(), q.cost, getGameState().park.flags.has(ParkFlag::noMoney),Next(),
                AccessPlacementContext::Capture(*RideManagement::Resolve(*target),Next()) };
            removeChoice = false;
            ClearPreview();UpdatePreview();
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
        ClearPreview();
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
        const auto* current=target?RideManagement::Resolve(*target):nullptr;
        if (!current || Next()!=baseline.location || AccessPlacementContext::Capture(*current,Next())!=baseline.context
            || Snapshot() != baseline.cells || baseline.noMoney != getGameState().park.flags.has(ParkFlag::noMoney)
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
            ClearPreview();direction = (direction + delta) & 3;
            feedback.clear();UpdatePreview();
        }
        else
            ChooseRemoval();
    }
    void Height(int delta)
    {
        ClearPreview();
        if (Picking())
        {
            quadrant = (quadrant + delta + 4) & 3;
            feedback.clear();
        }
        else if (!placed && !review)
            offset = std::clamp(offset + delta * 16, -256, 512);
        if(!placed && hovered)Update(hovered);else UpdatePreview();
    }
    void Update(std::optional<CoordsXY> tile)
    {
        if (!active)
            return;
        if(tile && !MapIsLocationValid(*tile))tile.reset();
        hovered = tile;
        if (!placed && !busy)
            cursor = tile ? std::optional<CoordsXYZD>{ { *tile, (MapGetHighestZ(*tile) + 15) / 16 * 16 + offset, direction } }
                          : std::nullopt;
        MapSelection::invalidate();
        gMapSelectFlags.unset(MapSelectFlag::enable, MapSelectFlag::enableArrow);
        const auto selected = Picking() ? PickLocation() : cursor ? std::optional<CoordsXYZD>{ Next() } : std::nullopt;
        if (selected)
        {
            const auto loc = *selected;
            gMapSelectPositionA = gMapSelectPositionB = loc.toTileStart();
            gMapSelectType = Picking() ? getMapSelectQuarter(MapGetTileQuadrant({ loc.x + 8, loc.y + 8 }) ^ 2)
                                       : MapSelectType::full;
            gMapSelectFlags.set(MapSelectFlag::enable, MapSelectFlag::enableArrow);
            gMapSelectArrowPosition = { loc.x + 8, loc.y + 8, loc.z };
            gMapSelectArrowDirection = direction;
            MapSelection::invalidate();
        }
        UpdatePreview();
    }
    static void UpdatePreview()
    {
        const auto* ride=target?RideManagement::Resolve(*target):nullptr;
        if(!active || busy || camera || !ride || !target || !CanResume(*target) || !cursor || (!placed && !hovered)){ClearPreview();return;}
        const auto loc=Next();const auto mode=review?GA::MazeBuildMode::fill:GA::MazeBuildMode::build;
        const auto context=AccessPlacementContext::Capture(*ride,loc);
        if(context.empty() || (review && (loc!=review->location || context!=review->context))){ClearPreview();return;}
        GA::MazeSetTrackAction action(loc,!placed,target->id,mode);auto q=GA::Query(&action,getGameState());
        if(q.error!=GA::Status::ok){ClearPreview();return;}
        feedback.clear();
        PreviewRequest next{target->id,loc,!placed,mode,context,q.cost};previewRequest.emplace(next);
        if(previewDepth)return;
        if(ghostRequest && ghostRequest->location==loc && ghostRequest->mode==mode && ghostRequest->context==context)return;
        RemoveGhost();action.GuardContext(context,q.cost);action.SetPureGhost();
        action.SetFlags({GA::CommandFlag::ghost,GA::CommandFlag::noSpend,GA::CommandFlag::allowDuringPaused});
        const auto result=GA::Execute(&action,getGameState());
        if(result.error==GA::Status::ok)ghostRequest.emplace(std::move(next));
        else {action.SetPureGhost(true);action.Execute(getGameState(),getGameState().park);previewRequest.reset();feedback=result.getErrorMessage();}
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
        DrawModernText(rt, p + ScreenCoordsXY{ 10, 86 }, "{WHITE}Direction " + std::string(directions[direction]));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 108 }, "{WHITE}Height " + (cursor ? std::to_string(cursor->z / 8) : "Unavailable"));
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 132 },
            camera          ? "{WHITE}D-pad Pan / X Rotate"
                : review    ? "{MUTED}L1/R1 Select option"
                : Picking() ? "{MUTED}Corner " + std::to_string(quadrant + 1) + "/4 L2/R2"
                            : "{MUTED}X Move | Y Remove");
        DrawModernText(
            rt, p + ScreenCoordsXY{ 10, 154 },
            camera          ? "{WHITE}A / B Return to build"
                : review    ? "{MUTED}A Apply | B Cancel"
                : Picking() ? "{MUTED}A Select maze cell"
                : placed    ? "{WHITE}B Return to ride"
                            : "{MUTED}L2/R2 Height");
        DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, 178 }, 144, "{WHITE}SELECT Camera");
        DrawActionStrip(rt, screen);
        std::vector<ControlHint> hints{ { "A",
                                          camera          ? "Build mode"
                                              : review    ? "Select"
                                              : Picking() ? "Pick"
                                                          : "Build" },
                                        { "B",
                                          review       ? "Cancel"
                                              : camera ? "Build"
                                                       : "Ride" },
                                        { "L1/R1",
                                          review       ? "Choice"
                                              : camera ? "Zoom"
                                                       : "Direction" } };
        if (!review)
        {
            hints.push_back({ "X", camera ? "Rotate" : "Move" });
            hints.push_back({ "START", "Ride" });
        }
        DrawFooter(rt, screen, hints);
        const auto q = review ? Apply(GA::MazeBuildMode::fill, false) : Picking() ? SelectCell(false) : Build(false);
        if (!feedback.empty())
            DrawContextMessage(rt, Layout::ContextAnchor(screen, kContextSize, true), feedback);
        else if (Picking() && q.error == GA::Status::ok)
            DrawContextMessage(rt, Layout::ContextAnchor(screen, kContextSize, true), "Choose native maze cell");
        else if (q.error == GA::Status::ok)
            DrawCostBadge(rt, Layout::ContextAnchor(screen, kContextSize, true), FormatMoney(q.cost));
        else
            DrawContextMessage(
                rt, Layout::ContextAnchor(screen, kContextSize, true), feedback.empty() ? q.getErrorMessage() : feedback);
    }
} // namespace OpenRCT2::Ui::Handheld::Mazes

