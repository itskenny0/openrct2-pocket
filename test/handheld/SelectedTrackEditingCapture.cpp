// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace C = H::CustomTracks;
namespace W = Ui::Windows;
static void Check(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3 && argc != 4)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 0);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = argc != 4;
        auto env = CreatePlatformEnvironment();
        if (argc == 4)
            env->SetBasePath(DirBase::rct2, argv[3]);
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        if (argc == 4)
            Config::Get().general.rct2Path = argv[3];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise(), "Context failed");
        context->GetUiContext().CreateWindow();
        auto& state = getGameState();
        gameStateInitAll(state, { 64, 64 });
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "Fixture viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        state.park.cash = 10000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        for (int y = 32; y < 2016; y += 32)
            for (int x = 32; x < 2016; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto& objects = context->GetObjectManager();
        Check(
            objects.LoadObject("rct2.terrain_surface.grass") && objects.LoadObject("rct2.terrain_edge.rock")
                && objects.LoadObject("rct2.station.plain"),
            "Fixture terrain/station missing");
        const char* ids[] = { "rct2.ride.mono1", "rct2.ride.ding1", "rct2.ride.bmair", "rct2.ride.arrx" };
        const ride_type_t types[] = { RIDE_TYPE_MONORAIL, RIDE_TYPE_DINGHY_SLIDE, RIDE_TYPE_FLYING_ROLLER_COASTER, RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER };
        const CoordsXY positions[] = { { 640, 640 }, { 640, 1408 }, { 1408, 1408 }, { 1408, 640 } };
        std::array<ObjectEntryIndex, 4> indices{};
        // Object/research availability is fixture initialization, before actions.
        for (int n = 0; n < 4; ++n)
        {
            auto* o = objects.LoadObject(ids[n]);
            Check(o, "Family native object missing");
            indices[n] = objects.GetLoadedObjectEntryIndex(o);
            RideTypeSetInvented(types[n]);
            RideEntrySetInvented(indices[n]);
        }
        auto* normalPath = objects.LoadObject("rct2.footpath_surface.tarmac");
        auto* queuePath = objects.LoadObject("rct2.footpath_surface.queue_blue");
        auto* railings = objects.LoadObject("rct2.footpath_railings.wood");
        Check(normalPath && queuePath && railings, "Family native path fixture missing");
        CoordsXYZ captureFocus{ 656, 656, 112 };
        auto capture = [&](const char* name, bool menu) {
            const auto* output = std::getenv("POCKET_SELECTED_CAPTURES");
            if (!output || argc != 4 || std::string(name).find("selected-") != 0)
                return;
            std::filesystem::create_directories(output);
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            Viewport viewport{};
            viewport.width = 512;
            viewport.height = 384;
            viewport.zoom = ZoomLevel{ 1 };
            const auto focus = C::Endpoint() ? CoordsXYZ(*C::Endpoint()) + CoordsXYZ{ 16, 16, 0 } : captureFocus;
            const auto centre = Translate3DTo2DWithZ(0, focus);
            viewport.viewPos = centre - ScreenCoordsXY{ viewport.zoom.ApplyTo(310), viewport.zoom.ApplyTo(190) };
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (menu)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            else if (H::Paths::IsActive())
                H::Paths::DrawOverlay(rt, { 8, 8 });
            else
                C::DrawPanel(rt, { 512, 384 });
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            const auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(output) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };

        std::vector<RideId> built;
        for (int family = 0; family < 4; ++family)
        {
            auto place = [&](TrackElemType piece) {
                Check(C::Select(piece), "Family native piece unavailable");
                const auto q = C::Action(false);
                const auto cash = state.park.cash;
                if (q.error != GameActions::Status::ok)
                    std::cerr << q.getErrorMessage() << "\n";
                Check(
                    q.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
                    "Family placement failed");
                GameActions::ProcessQueue(state);
                Check(state.park.cash == cash - q.cost, "Family native accounting differs");
            };
            Check(H::BeginCustomTrack(indices[family], types[family]), "Family controller begin unavailable");
            GameActions::ProcessQueue(state);
            C::Update(positions[family]);
            captureFocus = { positions[family].x + 16, positions[family].y + 16, C::Endpoint()->z };
            if (family == 3) Check(C::SetPlacementSeat(9), "Native seat fixture unavailable");
            place(TrackElemType::endStation);
            for (int n = 0; n < 3; ++n)
                place(TrackElemType::endStation);
            if (family == 1)
            {
                Check(C::Select(TrackElemType::flat) && C::ToggleCovered(), "Native covered circuit segment absent");
                place(TrackElemType::flatCovered);
                Check(C::ToggleCovered(), "Native covered circuit did not return to open");
            }
            else
                place(TrackElemType::flat);
            place(TrackElemType::flat);
            place(TrackElemType::leftQuarterTurn3Tiles);
            for (int n = 0; n < 4; ++n)
                place(TrackElemType::flat);
            place(TrackElemType::leftQuarterTurn3Tiles);
            for (int n = 0; n < 6; ++n)
                place(TrackElemType::flat);
            place(TrackElemType::leftQuarterTurn3Tiles);
            for (int n = 0; n < 4; ++n)
                place(TrackElemType::flat);
            place(TrackElemType::leftQuarterTurn3Tiles);
            Check(C::Completed().has_value(), "Family native circuit did not close");
            auto circuit = *C::Completed();
            built.push_back(circuit.id);
            capture(
                family == 0       ? "advanced-10-monorail-circuit-complete.png"
                    : family == 1 ? "advanced-11-dinghy-covered-circuit-complete.png"
                                  : "advanced-12-flying-inverted-circuit-complete.png",
                false);
            H::EndTerrain();
            Check(H::FlatRides::Resume(circuit), "Family native access recovery missing");
            for (int access = 0; access < 2; ++access)
            {
                std::optional<CoordsXY> tile;
                for (int y = 32; y < 2016 && !tile; y += 32)
                    for (int x = 32; x < 2016 && !tile; x += 32)
                        if (H::FlatRides::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                            tile = CoordsXY{ x, y };
                Check(
                    tile && H::FlatRides::Action(tile, true).error == GameActions::Status::ok,
                    "Family native entrance/exit failed");
                GameActions::ProcessQueue(state);
            }
            H::EndTerrain();
            auto access = H::RideAccess::Inspect(circuit);
            Check(access.has_value(), "Family native entrances/exits not present");
            for (bool exit : { false, true })
            {
                const auto point = exit ? access->exit : access->entrance;
                namespace P = H::Paths;
                Check(H::RideAccess::BeginPaths(circuit, exit, true), "Connected access UI start failed");
                Check(P::TargetTile() == CoordsXY(point.outside) && !P::TurnConnected(), "Initial access geometry changed");
                auto departed = circuit;
                ++departed.generation;
                Check(!P::BeginConnectedAccess(departed, point.station, exit, point.outside), "Stale access ride accepted");
                const auto landingCash = state.park.cash;
                const auto landingQuery = P::Action(false);
                captureFocus = { point.outside.x + 16, point.outside.y + 16, point.outside.z };
                if (family == 0)
                    capture(exit ? "parity-09-connected-exit-start.png" : "parity-08-connected-queue-start.png", false);
                Check(
                    landingQuery.error == GameActions::Status::ok && P::Action(true).error == GameActions::Status::ok,
                    "Connected native access landing failed");
                Check(P::Action(true).error != GameActions::Status::ok, "Held access placement repeated");
                GameActions::ProcessQueue(state);
                Check(state.park.cash == landingCash - landingQuery.cost, "Access landing cost differs");
                if (family == 0)
                {
                    auto draft = P::BeginUndo();
                    Check(draft.has_value(), "Access landing undo unavailable");
                    H::ControllerInput controller;
                    H::Frame confirm;
                    confirm.actions = { H::Action::confirm, H::Action::confirm };
                    W::HandheldPathOptionsOpen();
                    W::HandheldConstructionMove(0, 7);
                    H::HandleMenuInput(controller, confirm);
                    capture("parity-10-connected-undo-review.png", true);
                    H::HandleMenuInput(controller, confirm); // Held confirm cannot remove.
                    Check(MapGetFootpathElement(CoordsXYZ(point.outside)), "Held undo confirmation removed path");
                    Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                    draft = P::BeginUndo();
                    Check(draft && P::Undo(*draft, true).error == GameActions::Status::ok, "Access landing native undo failed");
                    GameActions::ProcessQueue(state);
                    Check(P::TargetTile() == CoordsXY(point.outside), "Undo lost access landing context");
                    Check(P::Action(true).error == GameActions::Status::ok, "Access landing rebuild failed");
                    GameActions::ProcessQueue(state);
                    Check(P::Action(false).error == GameActions::Status::ok, "Connected extension query failed");
                    if (exit)
                    {
                        Check(P::SlopeConnected(1), "Connected rise unavailable");
                        const auto extension = P::Action(false);
                        Check(
                            extension.error == GameActions::Status::ok && P::Action(true).error == GameActions::Status::ok,
                            "Access elevated extension failed");
                        GameActions::ProcessQueue(state);
                        capture("parity-11-connected-elevated-extension.png", false);
                        W::HandheldPathOptionsOpen();
                        W::HandheldConstructionMove(0, 7);
                        capture("parity-12-connected-dense-options.png", true);
                        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                    }
                }
                P::End();
            }
            access = H::RideAccess::Inspect(circuit);
            Check(access && access->entrance.attached && access->exit.attached, "Family native access not attached");
            namespace E = H::TrackEditing;
            H::ControllerInput controller;
            H::Frame confirm{}, back{};
            confirm.actions = { H::Action::confirm, H::Action::confirm };
            back.actions = { H::Action::back };
            auto openBlock = [&](const E::Block& block) {
                Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                W::HandheldRideOpen(block.ride.id);
                W::HandheldMenuMove(3 + (H::RideAccess::Supported(block.ride) ? 1 : 0) + (C::CanResume(block.ride) ? 1 : 0));
                H::HandleMenuInput(controller, confirm);
                const auto blocks = E::Blocks(block.ride);
                auto found = std::find(blocks.begin(), blocks.end(), block);
                Check(found != blocks.end(), "Selected circuit block missing");
                W::HandheldMenuMove(static_cast<int>(found - blocks.begin()));
                H::HandleMenuInput(controller, confirm);
            };
            auto choose = [&](const std::string& label) {
                const auto model = W::HandheldRidesPresentation();
                const auto row = std::find(model.rows.begin(), model.rows.end(), label);
                Check(row != model.rows.end(), "Selected circuit control missing");
                W::HandheldMenuMove(static_cast<int>(row - model.rows.begin()) - static_cast<int>(model.selected));
                H::HandleMenuInput(controller, confirm);
            };
            if (GetRide(circuit.id)->getRideTypeDescriptor().flags.has(RtdFlag::supportsMultipleColourSchemes))
            {
                const auto blocks = E::Blocks(circuit);
                auto block = *std::find_if(blocks.begin(), blocks.end(), [](const auto& b) { return b.type == TrackElemType::middleStation; });
                H::RideManagement::PaintTarget paint{ circuit, block.origin, block.type, 0, block.colour };
                Check(H::RideManagement::Paint(paint, 2, true).error == GameActions::Status::ok, "Native station colour fixture failed");
                GameActions::ProcessQueue(state);
            }
            for (auto part : { TrackElemType::beginStation, TrackElemType::middleStation, TrackElemType::endStation })
            {
                circuit = *H::RideManagement::Identify(circuit.id);
                const auto beforeBlocks = E::Blocks(circuit);
                const auto found = std::find_if(beforeBlocks.begin(), beforeBlocks.end(), [&](const auto& b) { return b.type == part; });
                Check(found != beforeBlocks.end(), "Native station part unavailable");
                const auto block = *found;
                const auto prefix = "selected-" + std::to_string(family + 1) + "-" + std::to_string(static_cast<int>(part));
                captureFocus = CoordsXYZ(block.origin) + CoordsXYZ{ 16, 16, 0 };
                const auto impact = E::AccessRemoval(block);
                std::cout << "Native part " << static_cast<int>(part) << " deletes " << impact.first << "," << impact.second << " access\n";
                const auto beforeContext = E::Context(block);
                const auto beforeCash = state.park.cash;
                openBlock(block);
                if (part == TrackElemType::middleStation) capture((prefix + "-inspection.png").c_str(), true);
                choose("Inspect connections / station...");
                if (part == TrackElemType::middleStation) {
                    W::HandheldMenuMove(4);
                    capture((prefix + "-station-context.png").c_str(), true);
                }
                H::HandleMenuInput(controller, back);
                choose("Replace block...");
                Check(W::HandheldRidesPresentation().selected == 0, "Circuit replacement not Cancel-first");
                if (part == TrackElemType::middleStation) capture((prefix + "-cancel-first-quote.png").c_str(), true);
                H::HandleMenuInput(controller, confirm);
                Check(E::Context(block) == beforeContext && state.park.cash == beforeCash, "Circuit cancel mutated park");
                if (part == TrackElemType::middleStation) capture((prefix + "-cancelled-unchanged.png").c_str(), true);
                choose("Replace block...");
                const auto removal = E::RemoveForReplacement(block);
                Check(removal.error == GameActions::Status::ok, "Circuit removal query failed");
                W::HandheldMenuMove(1);
                H::HandleMenuInput(controller, confirm);
                GameActions::ProcessQueue(state);
                Check(C::Replacing() && C::Active() && state.park.cash == beforeCash - removal.cost, "Circuit station native removal handoff failed");
                Check(C::Endpoint() == block.origin && C::PlacementInverted() == block.inverted, "Circuit draft identity changed");
                const auto gapCash = state.park.cash;
                if (part == TrackElemType::middleStation)
                {
                    C::Update({});
                    capture((prefix + "-native-gap-ghost.png").c_str(), false);
                    const auto gapFile = (std::filesystem::path(argv[2]) / (prefix + "-gap.park")).string();
                    Check(ScenarioSave(state, gapFile, {}), "Native provisional gap save failed");
                    Check(C::Replacing() && state.park.cash == gapCash && C::Action(false).error == GameActions::Status::ok, "Provisional save damaged selected rebuild");
                    if (family == 1) Check(C::Select(TrackElemType::flat) && C::ToggleCovered(), "Covered choice unavailable");
                    W::HandheldTrackPiecePickerOpen();
                    const auto choices = C::Pieces();
                    const auto utility = std::find_if(choices.begin(), choices.end(), [&](const auto& choice) {
                        return choice.identity.key == (family == 1 ? "covered" : family == 2 ? "inverted" : family == 3 ? "seat-plus" : "camera");
                    });
                    Check(utility != choices.end(), "Native parameter/variant utility unavailable");
                    const auto selected = std::find_if(choices.begin(), choices.end(), [&](const auto& choice) {
                        return choice.identity.key == std::to_string(static_cast<int>(C::Selection()));
                    });
                    Check(selected != choices.end(), "Native selected piece missing");
                    const auto from = static_cast<size_t>(selected - choices.begin());
                    const auto to = static_cast<size_t>(utility - choices.begin());
                    for (size_t n = 0; n < from % 3; ++n) W::HandheldObjectPickerMove(-1, 0);
                    for (size_t n = from / 3; n < to / 3; ++n) W::HandheldObjectPickerMove(0, 1);
                    for (size_t n = 0; n < to % 3; ++n) W::HandheldObjectPickerMove(1, 0);
                    capture((prefix + "-native-parameters.png").c_str(), true);
                    H::HandleMenuInput(controller, back);
                    Check(C::Select(TrackElemType::endStation), "Native station draft return failed");
                    auto* loaded = objects.GetLoadedObject(ObjectType::ride, block.ride.object);
                    Check(loaded, "Circuit descriptor missing");
                    const auto version = loaded->GetVersion();
                    auto changed = version; ++std::get<2>(changed); loaded->SetVersion(changed);
                    C::Update({});
                    Check(C::Action(true).error != GameActions::Status::ok && state.park.cash == gapCash, "Stale descriptor accepted");
                    capture((prefix + "-stale-descriptor-recovery.png").c_str(), false);
                    loaded->SetVersion(version);
                }
                const auto quote = C::Action(false);
                if (quote.error != GameActions::Status::ok) std::cerr << quote.getErrorMessage() << '\n';
                Check(quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok, "Circuit native rebuild failed");
                GameActions::ProcessQueue(state);
                Check(state.park.cash == gapCash - quote.cost, "Circuit rebuild accounting differs");
                circuit = *H::RideManagement::Identify(circuit.id);
                const auto afterBlocks = E::Blocks(circuit);
                Check(afterBlocks.size() == beforeBlocks.size() && C::Completed(), "Circuit connectivity not restored");
                for (size_t n = 0; n < afterBlocks.size(); ++n)
                    Check(afterBlocks[n].origin == beforeBlocks[n].origin && afterBlocks[n].type == beforeBlocks[n].type
                        && afterBlocks[n].inverted == beforeBlocks[n].inverted && afterBlocks[n].colour == beforeBlocks[n].colour
                        && afterBlocks[n].seat == beforeBlocks[n].seat && afterBlocks[n].speed == beforeBlocks[n].speed
                        && afterBlocks[n].chain == beforeBlocks[n].chain, "Rebuilt native station parameters changed");
                if (part == TrackElemType::middleStation) capture((prefix + "-native-connected-result.png").c_str(), false);
                if (part == TrackElemType::beginStation) {
                    W::HandheldRideAccessOpen(circuit);
                    capture((prefix + "-native-access-state.png").c_str(), true);
                    Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                }
                H::UpdateExplorer(controller);
                if (H::FlatRides::CanResume(circuit)) Check(H::ResumeFlatRide(circuit.id), "Deleted-access recovery unavailable");
                while (H::FlatRides::CurrentStage() != H::FlatRides::Stage::inactive) {
                    std::optional<CoordsXY> tile;
                    for (int y = 32; y < 2016 && !tile; y += 32)
                        for (int x = 32; x < 2016 && !tile; x += 32)
                            if (H::FlatRides::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok) tile = CoordsXY{ x, y };
                    Check(tile.has_value(), "Native access recovery site missing");
                    const auto cost = H::FlatRides::Action(tile, false);
                    const auto accessCash = state.park.cash;
                    Check(H::FlatRides::Action(tile, true).error == GameActions::Status::ok, "Access recovery failed");
                    GameActions::ProcessQueue(state);
                    Check(state.park.cash == accessCash - cost.cost, "Recovered access cost differs");
                }
                H::EndTerrain();
                Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                access = H::RideAccess::Inspect(circuit);
                Check(access && access->entrance.attached && access->exit.attached, "Recovered access not attached");
                if (part == TrackElemType::middleStation) {
                    W::HandheldRideAccessOpen(circuit);
                    capture((prefix + "-native-access-return.png").c_str(), true);
                    Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                }
            }
            GameActions::RideSetStatusAction opening(circuit.id, RideStatus::open);
            const auto q = GameActions::Query(&opening, state);
            if (q.error != GameActions::Status::ok)
                std::cerr << q.getErrorMessage() << "\n";
            Check(
                q.error == GameActions::Status::ok && GameActions::Execute(&opening, state).error == GameActions::Status::ok,
                "Family native opening failed");
            GameActions::ProcessQueue(state);
            Check(GetRide(circuit.id)->status == RideStatus::open, "Family native opening did not persist");
            if (family == 0)
            {
                std::optional<CoordsXYZ> initial;
                bool moved = false;
                for (int tick = 0; tick < 4096; ++tick)
                {
                    gameStateUpdateLogic();
                    const auto* nativeRide = GetRide(circuit.id);
                    if (auto* vehicle = getGameState().entities.getEntity<Vehicle>(nativeRide->vehicles[0]))
                    {
                        const CoordsXYZ location{ vehicle->x, vehicle->y, vehicle->z };
                        if (initial && location != *initial)
                            moved = true;
                        if (!initial)
                            initial = location;
                    }
                }
                Check(moved, "Native empty Monorail train did not move");
                std::cout << "Native empty Monorail vehicle moved over 4096 simulation ticks; no guest-use claim\n";
            }
            W::HandheldRideOpen(circuit.id);
            capture(
                family == 0       ? "advanced-13-monorail-native-open.png"
                    : family == 1 ? "advanced-14-dinghy-native-open.png"
                                  : "advanced-15-flying-native-open.png",
                true);
            Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
            std::cout << ids[family] << " action-built circuit/access/native Open PASS\n";
        }
        const auto file = std::string(argv[2]) + "/selected-track-connected.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, file, flags) && context->LoadParkFromFile(file), "Family native save/reload failed");
        for (auto id : built)
            Check(GetRide(id) && GetRide(id)->status == RideStatus::open, "Family saved Open lost");
        Check(!C::Active(), "Family tool context survived load");
        std::cout << "Selected begin/middle/end station native circuit/access/open/accounting/save workflow passed; no sustained "
                     "guest-use claim\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
