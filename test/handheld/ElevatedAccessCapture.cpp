// SPDX-License-Identifier: GPL-3.0-only
// Native private scenario, production picker/tool/shell rendering; no fixture objects or research.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Dashboard.h>
#include <openrct2-ui/handheld/Designs.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRegistry.h>
#include <openrct2/actions/ResultWithMessage.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/footpath/FootpathRemoveAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideEntranceExitRemoveAction.h>
#include <openrct2/actions/track/SavedTrackDesignPlaceAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/TrackDesignRepository.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
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
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[2];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise() && context->LoadParkFromFile(argv[3], false, true), "Scenario load failed");
        context->GetUiContext().CreateWindow();
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::error);
        // Dismiss the native scenario objective page before controller gameplay.
        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
        gGamePaused = 0;
        auto& state = getGameState();
        std::filesystem::create_directories(argv[4]);
        ScenarioRandSeed(0x1234567F, 0x789FABCD);
        std::optional<H::RideManagement::Target> target;
        for (auto& ride : RideManager(state))
            if (ride.type == RIDE_TYPE_WOODEN_ROLLER_COASTER)
            {
                auto t = H::RideManagement::Identify(ride.id);
                if (t && H::RideAccess::Inspect(*t))
                {
                    target = t;
                    break;
                }
            }
        Check(target.has_value(), "No supported native scenario coaster");
        const auto id = target->id;
        auto* ride = GetRide(id);
        if (ride->status != RideStatus::closed)
        {
            auto d = H::RideManagement::Begin(*target, RideStatus::closed);
            Check(d && H::RideManagement::Action(*d, true).error == GameActions::Status::ok, "Native closing failed");
            GameActions::ProcessQueue(state);
        }
        auto access = *H::RideAccess::Inspect(*target);
        const auto station = access.entrance.station;
        const auto entry = access.entrance.outside, exit = access.exit.outside;
        std::cout << "Native coaster " << ride->getName() << " station " << int(station.ToUnderlying()) + 1 << " start "
                  << entry.x << "," << entry.y << "," << entry.z << std::endl;
        // Remove only this station's native queue, through normal actions, to rebuild a
        // legal connection. Track/station geometry, ownership and guests are untouched.
        std::vector<CoordsXYZ> oldQueue;
        for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
            for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                for (auto* p : TileElementsView<PathElement>(CoordsXY{ x, y }))
                    if (!p->isGhost() && p->isQueue() && p->getRideIndex() == id && p->getStationIndex() == station)
                        oldQueue.emplace_back(x, y, p->getBaseZ());
        for (auto p : oldQueue)
        {
            GameActions::FootpathRemoveAction a(p);
            auto q = GameActions::Query(&a, state);
            auto before = state.park.cash;
            Check(
                q.error == GameActions::Status::ok && GameActions::Execute(&a, state).error == GameActions::Status::ok,
                "Native queue removal failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == before - q.cost, "Queue removal accounting failed");
        }
        H::ControllerInput controller;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm, H::Action::confirm };
        H::Frame back;
        back.actions = { H::Action::back, H::Action::back };
        auto focus = [&](CoordsXYZ p) {
            auto* main = WindowGetMain();
            const CoordsXYZ centre{ p.x + 16, p.y + 16, p.z };
            auto cursor = ScreenCoordsXY{ 256, 192 } - main->viewport->pos;
            main->savedViewPos = Translate3DTo2DWithZ(main->viewport->rotation, centre)
                - ScreenCoordsXY{ main->viewport->zoom.ApplyTo(cursor.x), main->viewport->zoom.ApplyTo(cursor.y) };
            main->viewport->viewPos = main->savedViewPos;
            main->flags.unset(WindowFlag::scrollingToLocation);
            main->invalidate();
        };
        auto capture = [&](const char* name, CoordsXYZ p, bool panel = false, bool modal = false, int rotation = 0) {
            Viewport vp{};
            vp.width = 512;
            vp.height = 384;
            vp.zoom = ZoomLevel{ 0 };
            vp.rotation = rotation;
            vp.viewPos = Translate3DTo2DWithZ(rotation, CoordsXYZ{ p.x + 16, p.y + 16, p.z }) - ScreenCoordsXY{ 320, 190 };
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &vp);
            if (modal)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            else if (panel)
            {
                H::Dashboard::State ds;
                ds.section = H::Dashboard::Section::rides;
                ds.focus = H::Dashboard::Focus::content;
                H::Dashboard::Draw(rt, { 512, 384 }, ds, H::RideAccess::Presentation(*target, 0), true);
            }
            else if (H::Paths::IsActive())
            {
                H::DrawPathFooter(rt, { 512, 384 }, "", H::Paths::IsRemoving());
                H::DrawPathOverlay(
                    rt, H::Concept::kOrigin, H::Paths::SurfaceName(), H::Paths::IsQueue(), H::Paths::IsRemoving(),
                    H::Paths::PlacementName());
                auto q = H::Paths::Action(false);
                if (q.error == GameActions::Status::ok)
                    H::DrawCostBadge(rt, { 370, 300 }, H::FormatMoney(q.cost));
            }
            else
                H::DrawGameplayFooter(rt, { 512, 384 }, "", false, false);
            Image im;
            im.Width = 512;
            im.Height = 384;
            im.Depth = 8;
            im.Stride = rt.LineStride();
            im.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            im.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), im, ImageFormat::png);
            engine.EndDraw();
        };
        auto ghostCount = [&]() {
            size_t count = 0;
            for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                    for (auto* path : TileElementsView<PathElement>(CoordsXY{ x, y }))
                        count += path->isGhost();
            return count;
        };
        // Native fixed-height projection must select the same tile at every camera rotation/zoom.
        Check(H::RideAccess::BeginPaths(*target, true), "Projection handoff failed");
        auto* main = WindowGetMain();
        const auto initialRotation = main->viewport->rotation;
        const auto initialZoom = main->viewport->zoom;
        Check(H::Paths::SetPlacementHeight(exit.z + 16), "Elevated plane rejected");
        for (int rotation = 0; rotation < 4; ++rotation)
            for (int zoom = static_cast<int8_t>(ZoomLevel::min()); zoom <= static_cast<int8_t>(ZoomLevel::max()); ++zoom)
            {
                main->viewport->rotation = rotation;
                main->viewport->zoom = ZoomLevel{ static_cast<int8_t>(zoom) };
                focus({ exit.x, exit.y, exit.z + 16 });
                H::Paths::UpdateTarget({ 256, 192 });
                Check(H::Paths::TargetTile() == CoordsXY(exit), "Rotation/zoom elevated target changed");
            }
        main->viewport->rotation = initialRotation;
        main->viewport->zoom = initialZoom;
        focus({ exit.x, exit.y, exit.z + 16 });
        H::Paths::UpdateTarget({ 256, 192 });
        H::Paths::ChangeHeight(1);
        H::Paths::UpdateTarget({ 256, 192 });
        Check(H::Paths::TargetTile() == CoordsXY(exit), "Height option drifted to another tile");
        focus({ 0, 0, exit.z + 16 });
        H::Paths::UpdateTarget({ 256, 192 });
        H::Paths::UpdatePreview();
        Check(
            ghostCount() == 0 && H::Paths::Action(false).error != GameActions::Status::ok,
            "Invalid plane target retained a ghost");
        H::EndTerrain();
        size_t elevatedPieces = 0, slopedPieces = 0;
        std::optional<CoordsXYZ> bridge;
        struct Piece
        {
            CoordsXYZ p;
            FootpathSlope slope;
        };
        auto route = [&](CoordsXYZD start, bool queue) {
            H::Paths::Begin(queue);
            std::vector<CoordsXYZ> goals;
            for (int y = 32; y < state.mapSize.y * 32 - 32; y += 32)
                for (int x = 32; x < state.mapSize.x * 32 - 32; x += 32)
                    for (auto* p : TileElementsView<PathElement>(CoordsXY{ x, y }))
                        if (!p->isGhost() && !p->isQueue()
                            && H::RideAccess::NormalPathReachesParkEntrance({ x, y, p->getBaseZ() }))
                            goals.emplace_back(x, y, p->getBaseZ());
            Check(!goals.empty(), "No native park-connected paths");
            auto heuristic = [&](CoordsXYZ p) {
                int best = INT32_MAX;
                for (auto g : goals)
                    best = std::min(best, (std::abs(p.x - g.x) + std::abs(p.y - g.y)) / 32 + std::abs(p.z - g.z) / 16);
                return best;
            };
            struct Node
            {
                CoordsXYZ pos;
                Direction incoming;
                int parent, g;
                Piece piece;
            };
            using Key = std::tuple<int, int, int, int>;
            std::vector<Node> nodes;
            std::map<Key, int> visited;
            std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>>
                pending;
            nodes.push_back({ start, DirectionReverse(start.direction), -1, 0, {} });
            pending.emplace(heuristic(start), 0);
            std::optional<int> goal;
            int queries = 0;
            while (!pending.empty() && !goal && queries < 100000)
            {
                auto index = pending.top().second;
                pending.pop();
                const auto node = nodes[index];
                const Key key{ node.pos.x, node.pos.y, node.pos.z, node.incoming };
                if (visited.contains(key))
                    continue;
                visited[key] = index;
                for (auto* p : TileElementsView<PathElement>(node.pos))
                    if (!p->isGhost() && !p->isQueue() && FootpathIsZAndDirectionValid(*p, node.pos.z / 8, node.incoming)
                        && H::RideAccess::NormalPathReachesParkEntrance({ node.pos.x, node.pos.y, p->getBaseZ() }))
                        goal = index;
                if (goal)
                    break;
                for (Direction d : kAllDirections)
                    for (int dz : { 0, 16, -16 })
                    {
                        if (index == 0 && dz)
                            continue;
                        if (dz && d != node.incoming)
                            continue;
                        if (d == DirectionReverse(node.incoming) && index)
                            continue;
                        const auto base = node.pos.z + std::min(dz, 0);
                        if (base < kFootpathMinHeight || base > kFootpathMaxHeight)
                            continue;
                        FootpathSlope slope{ dz ? FootpathSlopeType::sloped : FootpathSlopeType::flat,
                                             dz > 0 ? d : DirectionReverse(d) };
                        auto type = gFootpathSelection.getSelectedSurface();
                        PathConstructFlags flags = queue ? PathConstructFlag::IsQueue : 0;
                        if (gFootpathSelection.legacyPath != kObjectEntryIndexNull)
                        {
                            type = gFootpathSelection.legacyPath;
                            flags |= PathConstructFlag::IsLegacyPathObject;
                        }
                        // Prefer an unobstructed corridor. Native queries permit some
                        // overlapping path forms which are not a usable walk route.
                        bool usable = true;
                        for (auto* existing : TileElementsView<PathElement>(node.pos))
                            if (!existing->isGhost())
                            {
                                if (queue || existing->isQueue() || existing->getBaseZ() != base
                                    || existing->isSloped() != (slope.type == FootpathSlopeType::sloped)
                                    || (existing->isSloped() && existing->getSlopeDirection() != slope.direction))
                                    usable = false;
                            }
                        if (!usable)
                            continue;
                        GameActions::FootpathPlaceAction action(
                            { node.pos.x, node.pos.y, base }, slope, type, gFootpathSelection.railings, node.incoming, flags);
                        ++queries;
                        if (GameActions::Query(&action, state).error != GameActions::Status::ok)
                            continue;
                        const CoordsXYZ next{ CoordsXY(node.pos) + CoordsDirectionDelta[d], node.pos.z + dz };
                        if (!MapIsLocationValid(next))
                            continue;
                        nodes.push_back({ next, d, index, node.g + 1, { { node.pos.x, node.pos.y, base }, slope } });
                        pending.emplace(node.g + 1 + heuristic(next), nodes.size() - 1);
                    }
            }
            std::cout << (queue ? "Queue" : "Exit") << " search " << queries << " queries; goal " << bool(goal) << std::endl;
            Check(goal.has_value(), "No legal native elevated route");
            std::vector<Piece> result;
            for (int i = *goal; nodes[i].parent >= 0; i = nodes[i].parent)
                result.push_back(nodes[i].piece);
            std::reverse(result.begin(), result.end());
            H::Paths::End();
            return result;
        };
        auto build = [&](CoordsXYZD start, bool queue) {
            const auto pieces = route(start, queue);
            Check(!pieces.empty(), "Route empty");
            std::cout << (queue ? "Queue" : "Exit") << " pieces " << pieces.size() << std::endl;
            Check(H::RideAccess::BeginPaths(*target, !queue), "Native access handoff failed");
            for (size_t i = 0; i < pieces.size(); ++i)
            {
                const auto& part = pieces[i];
                Check(H::Paths::SetPlacementHeight(part.p.z), "Native height rejected");
                focus(part.p);
                H::Paths::UpdateTarget({ 256, 192 });
                Check(H::Paths::TargetTile() == CoordsXY(part.p), "Height-plane targeting missed real tile");
                if (part.slope.type == FootpathSlopeType::sloped)
                {
                    H::Paths::CycleSlope();
                    for (int n = 0; n < 4
                         && H::Paths::PlacementName().find(
                                std::array<const char*, 4>{ "NW", "NE", "SE", "SW" }[part.slope.direction])
                             == std::string::npos;
                         ++n)
                        H::Paths::RotateSlope();
                }
                const auto q = H::Paths::Action(false);
                const auto cash = state.park.cash;
                Check(q.error == GameActions::Status::ok, "Route native query rejected");
                H::Paths::UpdatePreview();
                Check(state.park.cash == cash, "Route ghost spent cash");
                if (i == 0 || part.slope.type == FootpathSlopeType::sloped)
                    capture(queue ? "elevated-queue-ghost.png" : "elevated-exit-ghost.png", part.p);
                H::HandleExplorerInput(controller, confirm);
                GameActions::ProcessQueue(state);
                if (state.park.cash != cash - q.cost)
                {
                    std::cerr << "Route tile " << i << " " << part.p.x << "," << part.p.y << "," << part.p.z << " query "
                              << q.cost << " charge " << cash - state.park.cash << " input " << int(InputGetState())
                              << " paused " << int(gGamePaused) << std::endl;
                    for (auto& window : gWindowList)
                        std::cerr << "Window " << int(window->classification) << " dead " << window->flags.has(WindowFlag::dead)
                                  << std::endl;
                }
                Check(state.park.cash == cash - q.cost, "Route A charged incorrectly");
                if (part.p.z > MapGetHighestZ(part.p))
                {
                    ++elevatedPieces;
                    bridge = part.p;
                }
                if (part.slope.type == FootpathSlopeType::sloped)
                    ++slopedPieces;
                for (auto* path : TileElementsView<PathElement>(part.p))
                    if (!path->isGhost() && path->getBaseZ() == part.p.z)
                        std::cout << "PLACED " << queue << " " << part.p.x << "," << part.p.y << "," << part.p.z << " slope "
                                  << int(part.slope.type) << ":" << int(part.slope.direction) << " edges "
                                  << int(path->getEdges()) << " ride " << path->getRideIndex().ToUnderlying() << " station "
                                  << int(path->getStationIndex().ToUnderlying()) << std::endl;
                if (i == pieces.size() / 2)
                {
                    H::HandleExplorerInput(controller, back);
                    Check(
                        !H::Paths::IsActive() && Ui::Windows::HandheldRidesIsOpen(),
                        "Unfinished route B did not return to access");
                    // Leave access completely, save the unfinished route, then resume via ordinary inspection.
                    H::HandleMenuInput(controller, confirm);
                    Check(!Ui::Windows::HandheldRidesIsOpen(), "Access Back did not return to park");
                    const auto savedCash = state.park.cash;
                    if (!queue)
                    {
                        const auto unfinished = (std::filesystem::path(argv[4]) / "unfinished-access.park").string();
                        SaveFlags flags{};
                        flags.set(SaveFlag::exportObjects);
                        Check(
                            ScenarioSave(state, unfinished, flags) && context->LoadParkFromFile(unfinished),
                            "Unfinished route save/reload failed");
                        target = H::RideManagement::Identify(id);
                        Check(target && state.park.cash == savedCash, "Unfinished route identity/cash lost");
                        InputSetState(InputState::normal);
                        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
                    }
                    Ui::Windows::HandheldRideOpen(id);
                    Ui::Windows::HandheldMenuMove(3);
                    H::HandleMenuInput(controller, confirm);
                    Check(Ui::Windows::HandheldRidesIsOpen(), "Later inspection access unavailable");
                    Ui::Windows::HandheldMenuMove(queue ? 1 : 2);
                    H::HandleMenuInput(controller, confirm);
                    Check(H::Paths::IsActive() && state.park.cash == savedCash, "Inspection route resume placed/spent");
                }
            }
            H::EndTerrain();
        };
        build(exit, false);
        build(entry, true);
        Check(elevatedPieces > 0 && slopedPieces > 0 && bridge, "Scenario route did not exercise elevation and slope");
        capture("elevated-connected-bridge.png", *bridge);
        access = *H::RideAccess::Inspect(*target);
        std::cout << "Final entry station " << int(access.entrance.station.ToUnderlying()) << " attached "
                  << access.entrance.attached << " normal " << access.entrance.reachesNormalPath << " park "
                  << access.entrance.reachesParkEntrance << " exit station " << int(access.exit.station.ToUnderlying())
                  << " attached " << access.exit.attached << " park " << access.exit.reachesParkEntrance << std::endl;
        Check(
            access.entrance.reachesParkEntrance && access.exit.reachesParkEntrance, "Native complete park connection missing");
        capture("elevated-connected-review.png", entry, true);
        capture("elevated-connected-park.png", entry);
        Ui::Windows::HandheldRideAccessOpen(*target);
        for (int n = 0; n < 3; ++n)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, confirm);
        Check(
            Ui::Windows::HandheldRidesPresentation().confirmation && Ui::Windows::HandheldRidesPresentation().selected == 1,
            "Opening not Cancel-first");
        capture("elevated-opening-confirmation.png", entry, false, true);
        const auto cash = state.park.cash;
        H::HandleMenuInput(controller, confirm);
        Check(GetRide(id)->status == RideStatus::closed && state.park.cash == cash, "Opening Cancel mutated park");
        Ui::Windows::HandheldRideAccessOpen(*target);
        for (int n = 0; n < 3; ++n)
            Ui::Windows::HandheldMenuMove(1);
        H::HandleMenuInput(controller, confirm);
        Ui::Windows::HandheldMenuMove(-1);
        H::HandleMenuInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(GetRide(id)->status == RideStatus::open && state.park.cash == cash, "Opening confirmation/accounting failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        gGamePaused = 0;
        if (!state.park.flags.has(ParkFlag::parkOpen))
        {
            GameActions::ParkSetParameterAction a(GameActions::ParkParameter::open);
            Check(GameActions::Execute(&a, state).error == GameActions::Status::ok, "Native park opening failed");
            GameActions::ProcessQueue(state);
        }
        std::set<uint32_t> queued, riding;
        bool done = false, capturedMovingVehicle = false;
        for (int tick = 0; tick < 160000 && !done; ++tick)
        {
            gameStateUpdateLogic();
            for (auto* guest : EntityList<Guest>())
            {
                if (guest->currentRide == id && guest->currentRideStation == station
                    && (guest->state == PeepState::queuing || guest->state == PeepState::queuingFront))
                {
                    if (queued.empty())
                        capture("elevated-guests-queue.png", CoordsXYZ{ guest->x, guest->y, guest->z }, false, false, 2);
                    queued.insert(guest->peepId);
                }
                if (guest->currentRide == id && guest->state == PeepState::onRide && queued.contains(guest->peepId))
                {
                    if (!capturedMovingVehicle)
                    {
                        auto* train = state.entities.getEntity<Vehicle>(GetRide(id)->vehicles[guest->currentTrain]);
                        auto* car = train ? train->GetCar(guest->currentCar) : nullptr;
                        Check(car && MapIsLocationValid(CoordsXY{ car->x, car->y }), "Boarded guest has no native car");
                        // Boarded guests have null map coordinates; follow their actual native vehicle.
                        if (car->status == Vehicle::Status::travelling
                            && std::abs(car->x - entry.x) + std::abs(car->y - entry.y) > 192)
                        {
                            capture("elevated-guests-riding.png", CoordsXYZ{ car->x, car->y, car->z }, false, false, 2);
                            capturedMovingVehicle = true;
                        }
                    }
                    riding.insert(guest->peepId);
                }
                if (capturedMovingVehicle && guest->state == PeepState::walking && guest->hasRidden(*GetRide(id))
                    && riding.contains(guest->peepId))
                {
                    capture("elevated-guests-exited.png", CoordsXYZ{ guest->x, guest->y, guest->z }, false, false, 2);
                    done = true;
                    break;
                }
            }
            if (tick % 10000 == 0)
                std::cout << "Tick " << tick << " queued " << queued.size() << " riding " << riding.size() << " customers "
                          << GetRide(id)->totalCustomers << std::endl;
        }
        Check(done && !queued.empty() && !riding.empty(), "Natural native queue/ride/exit not observed");
        const auto finalCash = state.park.cash;
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        auto save = (std::filesystem::path(argv[4]) / "elevated-access.park").string();
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Native save/reload failed");
        target = H::RideManagement::Identify(id);
        access = *H::RideAccess::Inspect(*target);
        Check(
            access.entrance.reachesParkEntrance && access.exit.reachesParkEntrance && state.park.cash == finalCash
                && GetRide(id)->status == RideStatus::open,
            "Saved access/status/cash lost");
        // A native external entrance move/removal invalidates the old guided landing before A.
        const auto savedRide = *target;
        access = *H::RideAccess::Inspect(savedRide);
        Check(H::RideAccess::BeginPaths(savedRide, false), "Stale target setup failed");
        const auto point = access.entrance;
        auto* currentRide = H::RideManagement::Resolve(savedRide);
        auto close = H::RideManagement::Begin(savedRide, RideStatus::closed);
        Check(close && H::RideManagement::Action(*close, true).error == GameActions::Status::ok, "Stale test close failed");
        GameActions::ProcessQueue(state);
        const auto entrancePosition = currentRide->getStations()[point.station.ToUnderlying()].entrance.toCoordsXYZD();
        GameActions::RideEntranceExitRemoveAction remove(entrancePosition, id, point.station, false);
        Check(GameActions::Execute(&remove, state).error == GameActions::Status::ok, "Stale native entrance removal failed");
        GameActions::ProcessQueue(state);
        const auto staleCash = state.park.cash;
        H::HandleExplorerInput(controller, confirm);
        Check(
            !H::Paths::IsActive() && ghostCount() == 0 && state.park.cash == staleCash,
            "Stale access target accepted A or retained ghost");
        Check(context->LoadParkFromFile(save) && state.park.cash == finalCash, "Stale test restore failed");
        std::cout << "Native elevated access / accounting / cancellation-resume / opening / natural guests / save-reload passed"
                  << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
