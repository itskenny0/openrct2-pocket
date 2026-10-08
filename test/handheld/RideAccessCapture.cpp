// SPDX-License-Identifier: GPL-3.0-only
// Native private scenario, production picker/tool/shell rendering; no fixture objects or research.
#include <filesystem>
#include <iostream>
#include <map>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <queue>
#include <set>
#include <stdexcept>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace F = H::FlatRides;
static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 6)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[5];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = false;
        auto env = CreatePlatformEnvironment();
        env->SetBasePath(DirBase::rct2, argv[2]);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), Ui::CreateDummyUiContext());
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[2];
        Check(context->Initialise() && context->LoadParkFromFile(argv[3], false, true), "Scenario load failed");
        gGamePaused = 0;
        auto& state = getGameState();
        // Reproducible native randomness; no guest, ride or path state is manufactured.
        ScenarioRandSeed(0x1234567F, 0x789FABCD);
        const auto initialCash = state.park.cash;
        std::cout << "Scenario guests " << state.park.numGuestsInPark << "; park open "
                  << state.park.flags.has(ParkFlag::parkOpen) << "\n";
        const auto entries = F::Entries();
        Check(!entries.empty(), "No eligible native flat rides");
        for (const auto& e : entries)
            std::cout << "Eligible: " << e.name << "\n";
        auto selected = entries.front().identity.index;
        for (const auto& e : entries)
            if (FlatRidePlacement::Type(e.identity.index) == RIDE_TYPE_MERRY_GO_ROUND)
                selected = e.identity.index;
        Check(H::BeginFlatRide(selected), "Begin flat ride failed");
        CoordsXY tile{};
        int best = -1;
        const auto& ted = TrackMetadata::GetTrackElementDescriptor(
            GetRideTypeDescriptor(FlatRidePlacement::Type(selected)).StartTrackPiece);
        for (int y = 128; y < (state.mapSize.y - 4) * 32; y += 32)
            for (int x = 128; x < (state.mapSize.x - 4) * 32; x += 32)
            {
                const CoordsXY p{ x, y };
                bool clear = true;
                const auto base = MapGetSurfaceElementAt(p)->getBaseZ();
                // Keep all native footprint cells clear; construction may not clear scenery for this capture.
                for (uint8_t i = 0; i < ted.sequenceData.numSequences; ++i)
                {
                    const auto& b = ted.sequenceData.sequences[i].clearance;
                    const auto xy = p + CoordsXY{ b.x, b.y };
                    auto* s = MapGetSurfaceElementAt(xy);
                    if (!s || !s->getOwnership().has(OwnershipFlag::landOwned) || s->getWaterHeight() || s->getSlope()
                        || s->getBaseZ() != base)
                    {
                        clear = false;
                        break;
                    }
                    for (auto* e : TileElementsView(xy))
                        if (e->getType() != TileElementType::surface)
                            clear = false;
                }
                if (!clear || F::Action(p, false).error != GameActions::Status::ok)
                    continue;
                int score = -1;
                for (int dy = -128; dy <= 128; dy += 32)
                    for (int dx = -128; dx <= 128; dx += 32)
                        if (MapIsLocationValid(p + CoordsXY{ dx, dy }))
                            for (auto* path : TileElementsView<PathElement>(p + CoordsXY{ dx, dy }))
                                if (!path->isGhost() && !path->isQueue() && !path->isSloped() && path->getBaseZ() == base)
                                    score = std::max(score, 1000 - std::abs(dx) - std::abs(dy));
                if (score <= best)
                    continue;
                // Select a site with two clear station approaches that reach existing level normal paths.
                std::set<std::pair<int, int>> footprint;
                for (uint8_t i = 0; i < ted.sequenceData.numSequences; ++i)
                {
                    const auto& b = ted.sequenceData.sequences[i].clearance;
                    footprint.emplace(x + b.x, y + b.y);
                }
                auto reachable = [&](CoordsXY start) {
                    std::queue<CoordsXY> pending;
                    std::set<std::pair<int, int>> seen;
                    pending.push(start);
                    while (!pending.empty())
                    {
                        auto xy = pending.front();
                        pending.pop();
                        if (!seen.emplace(xy.x, xy.y).second || !MapIsLocationValid(xy) || footprint.contains({ xy.x, xy.y })
                            || std::abs(xy.x - x) + std::abs(xy.y - y) > 320)
                            continue;
                        auto* surface = MapGetSurfaceElementAt(xy);
                        if (!surface || surface->getSlope() || surface->getBaseZ() != base)
                            continue;
                        bool clearTile = true;
                        for (auto* e : TileElementsView(xy))
                        {
                            if (e->getType() == TileElementType::path)
                            {
                                auto* path = e->asPath();
                                if (!path->isGhost() && !path->isQueue() && !path->isSloped() && path->getBaseZ() == base)
                                    return true;
                            }
                            else if (e->getType() != TileElementType::surface)
                                clearTile = false;
                        }
                        if (!clearTile || !surface->getOwnership().has(OwnershipFlag::landOwned))
                            continue;
                        for (auto d : kAllDirections)
                            pending.push(xy + CoordsDirectionDelta[d]);
                    }
                    return false;
                };
                int sides = 0;
                for (uint8_t i = 0; i < ted.sequenceData.numSequences; ++i)
                {
                    const auto& seq = ted.sequenceData.sequences[i];
                    const CoordsXY track{ x + seq.clearance.x, y + seq.clearance.y };
                    for (auto d : kAllDirections)
                    {
                        if (!(seq.getEntranceConnectionSides() & (1 << ((d - 2) & 3))))
                            continue;
                        const auto entrance = track - CoordsDirectionDelta[d];
                        auto* surface = MapGetSurfaceElementAt(entrance);
                        bool empty = true;
                        for (auto* e : TileElementsView(entrance))
                            if (e->getType() != TileElementType::surface)
                                empty = false;
                        if (empty && surface && !surface->getSlope() && surface->getBaseZ() == base
                            && surface->getOwnership().has(OwnershipFlag::landOwned)
                            && reachable(entrance - CoordsDirectionDelta[d]))
                            ++sides;
                    }
                }
                if (sides < 2)
                    continue;
                if (score > best)
                {
                    best = score;
                    tile = p;
                }
            }
        Check(best >= 0, "No native owned flat ride footprint");
        std::cout << "Site " << tile.x << "," << tile.y << "," << MapGetHighestZ(tile) << std::endl;
        std::filesystem::create_directories(argv[4]);
        H::ObjectPicker picker;
        picker.setEntries(entries, H::PickerIdentity{ ObjectType::ride, selected });
        auto capture = [&](const char* name, CoordsXYZ focus, int mode, money64 cost = 0) {
            Viewport viewport{};
            viewport.width = 512;
            viewport.height = 384;
            const auto centre = Translate3DTo2DWithZ(0, focus);
            viewport.viewPos = { centre.x - 300, centre.y - 180 };
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (mode == 0)
                H::DrawObjectPicker(rt, { 512, 384 }, picker, "Flat rides");
            else if (mode == 6)
            {
                H::Dashboard::State dashboard;
                dashboard.section = H::Dashboard::Section::rides;
                dashboard.focus = H::Dashboard::Focus::content;
                auto target = H::RideManagement::Identify(GetRide(F::Target()->id)->id);
                H::Dashboard::Draw(rt, { 512, 384 }, dashboard, H::RideAccess::Presentation(*target, 1), true);
            }
            else if (mode == 8)
            {
                H::Dashboard::State dashboard;
                dashboard.section = H::Dashboard::Section::rides;
                dashboard.focus = H::Dashboard::Focus::content;
                auto identity = H::RideManagement::Identify(F::Target()->id);
                auto draft = H::RideManagement::Begin(*identity, RideStatus::open);
                Check(draft.has_value(), "Opening draft missing");
                H::Dashboard::Draw(
                    rt, { 512, 384 }, dashboard,
                    H::RideManagement::Presentation(H::RideManagement::View::confirmation, 1, {}, identity, draft), true);
            }
            else if (mode == 7)
            {
                H::DrawPathOverlay(
                    rt, H::Concept::kOrigin, H::Paths::SurfaceName(), H::Paths::IsQueue(), false, H::Paths::PlacementName());
                H::DrawPathFooter(rt, { 512, 384 }, "", false);
                H::DrawCostBadge(rt, { 332, 272 }, H::FormatMoney(cost));
            }
            else if (mode == 5)
            {
                auto roster = H::RideManagement::Roster();
                auto target = F::Target();
                size_t row = 0;
                if (target)
                    for (size_t i = 0; i < roster.size(); ++i)
                        if (roster[i].id == target->id)
                            row = i;
                H::Dashboard::State dashboard;
                dashboard.section = H::Dashboard::Section::rides;
                dashboard.focus = H::Dashboard::Focus::content;
                H::Dashboard::Draw(
                    rt, { 512, 384 }, dashboard, H::RideManagement::Presentation(H::RideManagement::View::roster, row, roster),
                    true);
            }
            else if (mode < 4)
            {
                F::DrawPanel(rt, { 512, 384 });
                H::DrawCostBadge(rt, { 332, 272 }, H::FormatMoney(cost));
            }
            else
            {
                H::DrawGameplayFooter(rt, { 512, 384 }, "", false, false);
            }
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };
        const auto z = MapGetHighestZ(tile);
        const CoordsXYZ focus{ tile.x + 32, tile.y + 32, z };
        capture("flat-picker.png", focus, 0);
        const auto rideCost = F::Action(tile, false).cost;
        F::UpdatePreview(tile);
        capture("flat-ghost.png", focus, 1, rideCost);
        Check(state.park.cash == initialCash, "Preview spent cash");
        Check(F::Action(tile, true).error == GameActions::Status::ok, "Native flat ride placement failed");
        GameActions::ProcessQueue(state);
        Check(
            F::CurrentStage() == F::Stage::entrance && state.park.cash == initialCash - rideCost,
            "Flat ride cash/stage failed");
        auto target = F::Target();
        Check(target.has_value(), "New native ride identity missing");
        std::vector<CoordsXY> locations;
        for (int y = tile.y - 160; y <= tile.y + 160; y += 32)
            for (int x = tile.x - 160; x <= tile.x + 160; x += 32)
                if (F::EntranceLocation(CoordsXY{ x, y })
                    && F::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                    locations.push_back({ x, y });
        auto distanceToPath = [&](CoordsXY xy) {
            auto loc = *F::EntranceLocation(xy);
            const auto outside = xy - CoordsDirectionDelta[loc.direction];
            int distance = 100000;
            for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                    for (auto* path : TileElementsView<PathElement>(CoordsXY{ x, y }))
                        if (!path->isGhost() && !path->isQueue() && !path->isSloped() && path->getBaseZ() == loc.z)
                            distance = std::min(distance, std::abs(x - outside.x) + std::abs(y - outside.y));
            return distance;
        };
        std::sort(locations.begin(), locations.end(), [&](auto a, auto b) { return distanceToPath(a) < distanceToPath(b); });
        Check(locations.size() >= 2, "No valid native entrance/exit sides");
        auto indicate = [&](CoordsXY xy) {
            auto loc = *F::EntranceLocation(xy);
            gMapSelectPositionA = gMapSelectPositionB = xy;
            gMapSelectType = MapSelectType::full;
            gMapSelectArrowPosition = loc;
            gMapSelectArrowDirection = loc.direction;
            gMapSelectFlags.set(MapSelectFlag::enable, MapSelectFlag::enableArrow);
            MapSelection::invalidate();
        };
        indicate(locations[0]);
        const auto entranceCost = F::Action(locations[0], false).cost;
        capture("flat-entrance.png", focus, 2, entranceCost);
        Check(F::Action(locations[0], true).error == GameActions::Status::ok, "Entrance failed");
        GameActions::ProcessQueue(state);
        Check(F::CurrentStage() == F::Stage::exit, "Entrance handoff failed");
        // The exit must have its own level route, not share an entrance queue tile.
        auto reachesPath = [&](CoordsXY entrance) {
            auto loc = F::EntranceLocation(entrance);
            if (!loc)
                return false;
            const auto outside = entrance - CoordsDirectionDelta[loc->direction];
            std::queue<CoordsXY> pending;
            std::set<std::pair<int, int>> seen;
            pending.push(outside);
            seen.emplace(outside.x, outside.y);
            while (!pending.empty())
            {
                auto xy = pending.front();
                pending.pop();
                for (auto* path : TileElementsView<PathElement>(xy))
                    if (!path->isQueue() && !path->isSloped() && path->getBaseZ() == loc->z)
                        return true;
                for (auto d : kAllDirections)
                {
                    auto next = xy + CoordsDirectionDelta[d];
                    if (!MapIsLocationValid(next) || !seen.emplace(next.x, next.y).second)
                        continue;
                    auto* surface = MapGetSurfaceElementAt(next);
                    if (!surface || surface->getSlope() || surface->getBaseZ() != loc->z)
                        continue;
                    bool clear = true;
                    for (auto* e : TileElementsView(next))
                        if (e->getType() != TileElementType::surface && e->getType() != TileElementType::path)
                            clear = false;
                    if (clear && surface->getOwnership().has(OwnershipFlag::landOwned))
                        pending.push(next);
                }
            }
            return false;
        };
        auto exitPosition = std::find_if(locations.begin() + 1, locations.end(), reachesPath);
        Check(exitPosition != locations.end(), "No exit with legal level route");
        std::swap(locations[1], *exitPosition);
        indicate(locations[1]);
        const auto exitCost = F::Action(locations[1], false).cost;
        capture("flat-exit.png", focus, 3, exitCost);
        Check(F::Action(locations[1], true).error == GameActions::Status::ok, "Exit failed");
        GameActions::ProcessQueue(state);
        gMapSelectFlags.unset(MapSelectFlag::enable, MapSelectFlag::enableArrow);
        MapSelection::invalidate();
        Check(state.park.cash == initialCash - rideCost - entranceCost - exitCost, "Exact native construction cash failed");
        auto access = H::RideAccess::Inspect(*target);
        Check(access.has_value(), "Access missing");
        auto route = [&](CoordsXYZ start) {
            std::cout << "Route from " << start.x << "," << start.y << "," << start.z << std::endl;
            std::queue<CoordsXY> pending;
            std::map<std::pair<int, int>, CoordsXY> parent;
            pending.push(start);
            parent[{ start.x, start.y }] = start;
            std::optional<CoordsXY> goal;
            while (!pending.empty() && !goal)
            {
                auto xy = pending.front();
                pending.pop();
                for (auto* path : TileElementsView<PathElement>(xy))
                    if (!path->isGhost() && !path->isQueue() && !path->isSloped() && path->getBaseZ() == start.z)
                        goal = xy;
                if (goal)
                    break;
                for (auto d : kAllDirections)
                {
                    const auto next = xy + CoordsDirectionDelta[d];
                    if (parent.contains({ next.x, next.y }) || !MapIsLocationValid(next))
                        continue;
                    const auto* surface = MapGetSurfaceElementAt(next);
                    if (!surface || surface->getSlope() || surface->getBaseZ() != start.z)
                        continue;
                    bool existingNormal = false;
                    for (auto* path : TileElementsView<PathElement>(next))
                        if (!path->isGhost() && !path->isQueue() && !path->isSloped() && path->getBaseZ() == start.z)
                            existingNormal = true;
                    auto type = gFootpathSelection.getSelectedSurface();
                    PathConstructFlags flags = H::Paths::IsQueue() ? PathConstructFlag::IsQueue : 0;
                    if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
                    {
                        type = gFootpathSelection.legacyPath;
                        flags |= PathConstructFlag::IsLegacyPathObject;
                    }
                    auto action = GameActions::FootpathPlaceAction(
                        { next, start.z }, { FootpathSlopeType::flat, 0 }, type, gFootpathSelection.railings, kInvalidDirection,
                        flags);
                    if (!existingNormal && GameActions::Query(&action, state).error != GameActions::Status::ok)
                        continue;
                    parent[{ next.x, next.y }] = xy;
                    pending.push(next);
                }
            }
            Check(goal.has_value(), "No level native connection route");
            std::vector<CoordsXY> result;
            auto cursor = *goal;
            while (cursor != CoordsXY(start))
            {
                cursor = parent.at({ cursor.x, cursor.y });
                result.push_back(cursor);
            }
            std::reverse(result.begin(), result.end());
            return result;
        };
        // Automation chooses legal tiles; each committed tile uses the production Paths query/action.
        auto connect = [&](CoordsXYZ start, bool queue) {
            H::Paths::Begin(queue);
            const auto tiles = route(start);
            Check(!tiles.empty(), "Connection route empty");
            std::cout << (queue ? "Queue" : "Exit") << " connection: " << tiles.size() << " native tiles\n";
            for (const auto xy : tiles)
            {
                InteractionInfo info;
                info.Loc = xy;
                info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(xy));
                info.interactionType = ViewportInteractionItem::terrain;
                H::Paths::UpdateTargetFromInfo(info);
                const auto query = H::Paths::Action(false);
                const auto cash = state.park.cash;
                Check(query.error == GameActions::Status::ok, "Connection query rejected");
                H::Paths::UpdatePreview();
                Check(state.park.cash == cash, "Path preview spent cash");
                if (xy == tiles.front())
                    capture(queue ? "access-queue-ghost.png" : "access-exit-ghost.png", focus, 7, query.cost);
                Check(H::Paths::Action(true).error == GameActions::Status::ok, "Connection placement rejected");
                GameActions::ProcessQueue(state);
                Check(state.park.cash == cash - query.cost, "Connection exact path cash failed");
            }
            H::Paths::End();
        };
        capture("access-review.png", focus, 6);
        connect(access->entrance.outside, true);
        connect(access->exit.outside, false);
        access = H::RideAccess::Inspect(*target);
        Check(access && access->entrance.reachesNormalPath && access->exit.attached, "Real access not connected");
        capture("access-connected.png", focus, 6);
        capture("access-connected-park.png", focus, 4);
        capture("access-opening-confirm.png", focus, 8);
        auto draft = H::RideManagement::Begin(*target, RideStatus::open);
        Check(draft && H::RideManagement::Action(*draft, true).error == GameActions::Status::ok, "Native opening failed");
        GameActions::ProcessQueue(state);
        // Opening the scenario park uses its native action; guest choices, needs and ride state are never forced.
        if (!state.park.flags.has(ParkFlag::parkOpen))
        {
            auto action = GameActions::ParkSetParameterAction(GameActions::ParkParameter::open);
            Check(GameActions::Execute(&action, state).error == GameActions::Status::ok, "Native park opening failed");
            GameActions::ProcessQueue(state);
        }
        const auto id = target->id;
        std::set<uint32_t> queued, riding;
        bool queueCapture = false, rideCapture = false, leftCapture = false;
        for (int tick = 0; tick < 100000 && !leftCapture; ++tick)
        {
            gameStateUpdateLogic();
            for (auto* guest : EntityList<Guest>())
            {
                const bool sameRide = guest->currentRide == id;
                if (sameRide && (guest->state == PeepState::queuing || guest->state == PeepState::queuingFront))
                {
                    queued.insert(guest->peepId);
                    if (!queueCapture)
                    {
                        capture("access-guests-queue.png", focus, 4);
                        queueCapture = true;
                    }
                }
                if (sameRide && guest->state == PeepState::onRide && queued.contains(guest->peepId))
                {
                    riding.insert(guest->peepId);
                    if (!rideCapture)
                    {
                        capture("access-guests-riding.png", focus, 4);
                        rideCapture = true;
                    }
                }
                if (guest->state == PeepState::walking && guest->hasRidden(*GetRide(id)) && riding.contains(guest->peepId))
                {
                    capture("access-guests-exited.png", focus, 4);
                    leftCapture = true;
                    break;
                }
            }
            if (tick % 10000 == 0)
                std::cout << "Tick " << tick << ", guests " << state.park.numGuestsInPark << ", queued " << queued.size()
                          << ", riding " << riding.size() << ", customers " << GetRide(id)->totalCustomers << std::endl;
        }
        Check(
            queueCapture && rideCapture && leftCapture && GetRide(id)->totalCustomers > 0,
            "Natural guest queue/ride/exit not observed");
        capture("access-dashboard.png", focus, 5);
        std::cout << "Observed native queue -> ride -> walking exit; " << GetRide(id)->totalCustomers << " customers"
                  << std::endl;
        const auto finalCash = state.park.cash;
        H::EndTerrain();
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto save = (std::filesystem::path(argv[5]) / "flat-capture.park").string();
        Check(
            ScenarioSave(state, save, flags) != 0 && context->LoadParkFromFile(save), "Native construction save/reload failed");
        auto* ride = GetRide(target->id);
        auto restored = H::RideManagement::Identify(target->id);
        Check(
            restored && H::RideAccess::Inspect(*restored)->entrance.reachesNormalPath
                && H::RideAccess::Inspect(*restored)->exit.attached && ride->totalCustomers > 0,
            "Saved native paths/customers missing");
        Check(ride && ride->status == RideStatus::open && state.park.cash == finalCash, "Saved flat ride/cash missing");
        Check(
            !ride->getStation(StationIndex::FromUnderlying(0)).entrance.isNull()
                && !ride->getStation(StationIndex::FromUnderlying(0)).exit.isNull(),
            "Saved entrance/exit missing");
        std::cout << entries.size() << " eligible flat rides; " << F::Name() << " placed at " << tile.x << "," << tile.y
                  << "; exact cash, native access, guest use and save/reload passed; 14 captures at 512x384" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
