// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include "RideTabNavigation.h"

#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
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
        auto* object = objects.LoadObject("rct2.ride.arrx");
        Check(object, "Native seat-rotation vehicle missing");
        const auto index = objects.GetLoadedObjectEntryIndex(object);
        RideTypeSetInvented(RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER);
        RideEntrySetInvented(index);
        auto* dinghyObject = objects.LoadObject("rct2.ride.ding1");
        Check(dinghyObject, "Native covered-track vehicle missing");
        const auto dinghyIndex = objects.GetLoadedObjectEntryIndex(dinghyObject);
        RideTypeSetInvented(RIDE_TYPE_DINGHY_SLIDE);
        RideEntrySetInvented(dinghyIndex);
        auto* flyingObject = objects.LoadObject("rct2.ride.bmair");
        Check(flyingObject, "Native Flying vehicle missing");
        const auto flyingIndex = objects.GetLoadedObjectEntryIndex(flyingObject);
        RideTypeSetInvented(RIDE_TYPE_FLYING_ROLLER_COASTER);
        RideEntrySetInvented(flyingIndex);
        Check(H::BeginCustomTrack(index, RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER), "Native seat ride begin failed");
        GameActions::ProcessQueue(state);
        C::Update(CoordsXY{ 640, 640 });
        Check(C::SeatSupported() && C::PlacementSeat() == 4, "Native default seat rotation missing");
        Check(!C::SetPlacementSeat(16) && C::SetPlacementSeat(9), "Seat bounds not enforced");
        auto capture = [&](const char* name, bool menu) {
            const auto* output = std::getenv("POCKET_SEAT_CAPTURES");
            if (!output || argc != 4)
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
            const auto focus = C::Endpoint() ? CoordsXYZ(*C::Endpoint()) + CoordsXYZ{ 16, 16, 0 } : CoordsXYZ{ 656, 656, 32 };
            const auto centre = Translate3DTo2DWithZ(0, focus);
            viewport.viewPos = centre - ScreenCoordsXY{ viewport.zoom.ApplyTo(310), viewport.zoom.ApplyTo(190) };
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (menu)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
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
        const auto cash = state.park.cash;
        C::Update(CoordsXY{ 640, 640 });
        capture("authoring-08-seat-rotation-preview.png", false);
        Check(state.park.cash == cash, "Seat ghost charged cash");
        auto quote = C::Action(false);
        Check(
            quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
            "Native seat station query/execute failed");
        Check(!C::SetPlacementSeat(2), "Busy parameter changed");
        GameActions::ProcessQueue(state);
        const auto ride = *C::Target();
        bool stationSeat = false;
        for (auto* e : TileElementsView<TrackElement>(CoordsXY{ 640, 640 }))
            if (!e->isGhost() && e->getRideIndex() == ride.id)
                stationSeat = e->getSeatRotation() == 9;
        Check(stationSeat && state.park.cash == cash - quote.cost, "Native seat value/cost mismatch");
        Check(C::Select(TrackElemType::flat), "Seat straight unavailable");
        const auto endpoint = *C::Endpoint();
        auto invalid = GameActions::SequentialTrackPlaceAction(
            ride.id, TrackElemType::flat, endpoint, false, false, H::TrackEditing::Blocks(ride).front().origin,
            TrackElemType::endStation);
        invalid.SetPlacementSeat(10);
        Check(GameActions::Query(&invalid, state).error == GameActions::Status::ok, "Native valid seat baseline rejected");
        invalid.SetPlacementSeat(16);
        Check(GameActions::Query(&invalid, state).error != GameActions::Status::ok, "Native action accepted invalid seat");
        H::ControllerInput controller;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm, H::Action::confirm };
        W::HandheldTrackPiecePickerOpen();
        const auto pieces = C::Pieces();
        const auto option = std::find_if(
            pieces.begin(), pieces.end(), [](const auto& e) { return e.identity.key == "seat-plus"; });
        Check(option != pieces.end(), "Applicable seat control missing");
        const auto row = std::distance(pieces.begin(), option);
        for (int n = 0; n < row / 3; ++n)
            W::HandheldObjectPickerMove(0, 1);
        for (int n = 0; n < row % 3; ++n)
            W::HandheldObjectPickerMove(1, 0);
        capture("authoring-09-seat-rotation-picker.png", true);
        H::HandleMenuInput(controller, confirm);
        Check(
            C::PlacementSeat() == 10 && !Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),
            "Held-A seat utility failed or repeated");
        quote = C::Action(false);
        Check(
            quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok, "Seat straight failed");
        GameActions::ProcessQueue(state);
        Check(
            !TrackConstructionSupport::SeatRotation(RIDE_TYPE_WOODEN_ROLLER_COASTER, TrackElemType::flat)
                && !TrackConstructionSupport::SeatRotation(ride.type, TrackElemType::brakes),
            "Unsupported seat control exposed");
        const auto seatBlocks = H::TrackEditing::Blocks(ride);
        const auto seatBlock = *std::find_if(
            seatBlocks.begin(), seatBlocks.end(), [](const auto& b) { return b.type == TrackElemType::flat; });
        auto staleSeat = seatBlock;
        staleSeat.seat = 9;
        Check(
            !H::TrackEditing::Valid(staleSeat) && H::TrackEditing::Change(staleSeat, {}).error != GameActions::Status::ok,
            "Selected stale seat baseline accepted");
        GameActions::SequentialTrackPlaceAction staleRemoval(
            ride.id, seatBlock.type, seatBlock.origin, seatBlock.chain, false, seatBlock.origin, seatBlock.type);
        staleRemoval.UseSelectedRemoval(seatBlock.speed, seatBlock.inverted, 9);
        Check(GameActions::Query(&staleRemoval, state).error != GameActions::Status::ok, "Action stale seat baseline accepted");
        staleRemoval.UseSelectedRemoval(seatBlock.speed, seatBlock.inverted, 10);
        Check(
            GameActions::Query(&staleRemoval, state).error == GameActions::Status::ok, "Valid selected seat baseline rejected");
        H::EndTerrain();
        auto openReplacement = [&] {
            W::HandheldRideOpen(ride.id);
            FocusRideTabAction("Edit track...");
            H::HandleMenuInput(controller, confirm);
            const auto currentBlocks = H::TrackEditing::Blocks(ride);
            const auto selected = std::find_if(
                currentBlocks.begin(), currentBlocks.end(), [&](const auto& b) { return b.origin == seatBlock.origin; });
            Check(selected != currentBlocks.end(), "Replacement selected block missing");
            W::HandheldMenuMove(std::distance(currentBlocks.begin(), selected));
            H::HandleMenuInput(controller, confirm);
            const auto rows = W::HandheldRidesPresentation().rows;
            const auto replacement = std::find(rows.begin(), rows.end(), "Replace block...");
            Check(replacement != rows.end(), "Replacement control unavailable");
            W::HandheldMenuMove(std::distance(rows.begin(), replacement));
            H::HandleMenuInput(controller, confirm);
            Check(
                W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 0,
                "Replacement removal not Cancel-first");
        };
        const auto replacementCash = state.park.cash;
        openReplacement();
        capture("authoring-10-replacement-confirmation.png", true);
        H::HandleMenuInput(controller, confirm);
        Check(H::TrackEditing::Valid(seatBlock) && state.park.cash == replacementCash, "Cancelled replacement removed block");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        openReplacement();
        W::HandheldMenuMove(1);
        H::HandleMenuInput(controller, confirm);
        GameActions::ProcessQueue(state);
        Check(
            !H::TrackEditing::Valid(seatBlock) && C::Active() && C::PlacementSeat() == 10
                && H::TrackEditing::Blocks(ride).size() == seatBlocks.size() - 1,
            "Replacement removal/recovery/autobuild failed");
        C::Update({});
        capture("authoring-11-replacement-gap-preview.png", false);
        quote = C::Action(false);
        Check(
            quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
            "Explicit replacement build failed");
        GameActions::ProcessQueue(state);
        Check(H::TrackEditing::Valid(seatBlock), "Replacement did not preserve native seat/geometry");
        H::EndTerrain();
        W::HandheldRideOpen(ride.id);
        FocusRideTabAction("Edit track...");
        H::HandleMenuInput(controller, confirm);
        H::HandleMenuInput(controller, confirm);
        auto backwardRows = W::HandheldRidesPresentation().rows;
        const auto backwardOption = std::find(backwardRows.begin(), backwardRows.end(), "Build backward...");
        Check(backwardOption != backwardRows.end(), "Backward roster control missing");
        capture("advanced-01-endpoint-block-selection.png", true);
        W::HandheldMenuMove(std::distance(backwardRows.begin(), backwardOption));
        H::HandleMenuInput(controller, confirm);
        Check(
            C::Backward() && !Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu), "Held-A backward handoff failed");
        C::Update({});
        capture("advanced-02-backward-native-preview.png", false);
        const auto backwardCash = state.park.cash;
        const auto backwardCount = H::TrackEditing::Blocks(ride).size();
        quote = C::Action(false);
        Check(
            quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok
                && C::Action(true).error != GameActions::Status::ok,
            "Backward SDL/native build repeat failed");
        GameActions::ProcessQueue(state);
        Check(
            H::TrackEditing::Blocks(ride).size() == backwardCount + 1 && state.park.cash == backwardCash - quote.cost,
            "Backward native accounting failed");
        Check(C::Select(TrackElemType::leftQuarterTurn3Tiles), "Backward native turn unavailable");
        C::Update({});
        capture("advanced-03-backward-multitile-preview.png", false);
        Check(C::Action(true).error == GameActions::Status::ok, "Backward native turn execute failed");
        GameActions::ProcessQueue(state);
        Check(C::Undo().error == GameActions::Status::ok, "Backward native turn undo failed");
        GameActions::ProcessQueue(state);
        Check(C::Select(TrackElemType::down25ToFlat), "Backward native slope transition absent");
        Check(
            C::Action(false).error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
            "Backward native slope transition failed");
        GameActions::ProcessQueue(state);
        C::Update({});
        capture("advanced-04-backward-slope-preview.png", false);
        Check(C::Undo().error == GameActions::Status::ok, "Backward slope undo failed");
        GameActions::ProcessQueue(state);
        Check(C::Undo().error == GameActions::Status::ok, "Backward native straight undo failed");
        GameActions::ProcessQueue(state);
        H::EndTerrain();
        Check(!C::Backward() && H::TrackEditing::Valid(seatBlock), "Backward return-to-park damaged forward context");
        // A native action creates a disconnected inverted source. Resume must
        // preserve its native variant and apply actual bank connection semantics.
        SelectedLiftAndInverted invertedFlags{};
        invertedFlags.set(LiftHillAndInverted::inverted);
        const CoordsXYZD invertedOrigin{ 1408, 640, 160, 0 };
        GameActions::TrackPlaceAction invertedSource(
            ride.id, TrackElemType::flat, ride.type, invertedOrigin, 6, 0, 7, invertedFlags, false);
        Check(
            GameActions::Query(&invertedSource, state).error == GameActions::Status::ok
                && GameActions::Execute(&invertedSource, state).error == GameActions::Status::ok,
            "Native inverted fixture action failed");
        GameActions::ProcessQueue(state);
        Check(
            H::ResumeCustomTrackFrom(*H::RideManagement::Identify(ride.id), invertedOrigin, TrackElemType::flat)
                && C::PlacementInverted() && C::Select(TrackElemType::flat),
            "Inverted source resume failed");
        C::Update({});
        capture("advanced-05-inverted-native-preview.png", false);
        auto invertedEndpoint = *C::Endpoint();
        quote = C::Action(false);
        Check(
            quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
            "Inverted matching query/execute failed");
        GameActions::ProcessQueue(state);
        bool invertedStored = false;
        for (auto* e : TileElementsView<TrackElement>(invertedEndpoint))
            invertedStored |= !e->isGhost() && e->getRideIndex() == ride.id && e->isInverted();
        Check(invertedStored, "Native inverted flag missing");
        const auto variantPieces = C::Pieces();
        Check(
            std::none_of(variantPieces.begin(), variantPieces.end(), [](const auto& e) { return e.identity.key == "inverted"; })
                && !C::ToggleInverted() && C::PlacementInverted(),
            "Unavailable normal transition exposed or changed draft");
        Check(C::Undo().error == GameActions::Status::ok, "Inverted undo failed");
        GameActions::ProcessQueue(state);
        H::EndTerrain();
        Check(
            H::ResumeCustomTrackFrom(*H::RideManagement::Identify(ride.id), invertedOrigin, TrackElemType::flat, true)
                && C::Backward() && C::PlacementInverted(),
            "Inverted backward recovery failed");
        Check(
            C::Select(TrackElemType::flat) && C::Action(true).error == GameActions::Status::ok,
            "Inverted backward placement failed");
        GameActions::ProcessQueue(state);
        Check(C::Undo().error == GameActions::Status::ok, "Inverted backward undo failed");
        GameActions::ProcessQueue(state);
        H::EndTerrain();
        Check(H::BeginCustomTrack(dinghyIndex, RIDE_TYPE_DINGHY_SLIDE), "Covered native ride begin failed");
        GameActions::ProcessQueue(state);
        C::Update(CoordsXY{ 1408, 1408 });
        Check(C::Action(true).error == GameActions::Status::ok, "Covered native station failed");
        GameActions::ProcessQueue(state);
        const auto coveredRide = *C::Target();
        Check(C::Select(TrackElemType::flat), "Covered base unavailable");
        auto unsupportedVariant = GameActions::SequentialTrackPlaceAction(
            coveredRide.id, TrackElemType::flat, *C::Endpoint(), false, false,
            H::TrackEditing::Blocks(coveredRide).front().origin, TrackElemType::endStation);
        unsupportedVariant.SetPlacementInverted(true);
        Check(
            GameActions::Query(&unsupportedVariant, state).error != GameActions::Status::ok,
            "Strict action accepted unavailable inverted variant");
        W::HandheldTrackPiecePickerOpen();
        auto coveredPieces = C::Pieces();
        auto coveredOption = std::find_if(
            coveredPieces.begin(), coveredPieces.end(), [](const auto& e) { return e.identity.key == "covered"; });
        Check(coveredOption != coveredPieces.end(), "Covered utility absent");
        auto coveredRow = std::distance(coveredPieces.begin(), coveredOption);
        for (int n = 0; n < coveredRow / 3; ++n)
            W::HandheldObjectPickerMove(0, 1);
        for (int n = 0; n < coveredRow % 3; ++n)
            W::HandheldObjectPickerMove(1, 0);
        capture("advanced-06-covered-native-picker.png", true);
        H::HandleMenuInput(controller, confirm);
        Check(
            C::Selection() == TrackElemType::flatCovered && !Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),
            "Held-A covered utility failed");
        C::Update({});
        capture("advanced-07-covered-native-preview.png", false);
        const auto coveredOrigin = *C::Endpoint();
        quote = C::Action(false);
        Check(
            quote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
            "Covered native placement failed");
        GameActions::ProcessQueue(state);
        Check(!C::ToggleInverted(), "Unsupported inverted variant exposed");
        Check(C::ToggleCovered() && C::Selection() == TrackElemType::flat, "Covered draft did not return to native open piece");
        Check(C::Undo().error == GameActions::Status::ok, "Covered native removal failed");
        GameActions::ProcessQueue(state);
        Check(C::ToggleCovered() && C::Action(true).error == GameActions::Status::ok, "Covered persistence rebuild failed");
        GameActions::ProcessQueue(state);
        H::EndTerrain();
        Check(H::BeginCustomTrack(flyingIndex, RIDE_TYPE_FLYING_ROLLER_COASTER), "Native inverted-start ride unavailable");
        GameActions::ProcessQueue(state);
        C::Update(CoordsXY{ 1408, 1024 });
        Check(C::PlacementInverted(), "Native inverted-start descriptor ignored");
        capture("advanced-08-flying-inverted-station.png", false);
        Check(C::Action(true).error == GameActions::Status::ok, "Native inverted-start station failed");
        GameActions::ProcessQueue(state);
        Check(C::Select(TrackElemType::flat) && C::PlacementInverted(), "Inverted-start forward geometry unavailable");
        C::Update({});
        W::HandheldTrackPiecePickerOpen();
        capture("advanced-09-flying-native-piece-picker.png", true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Check(C::Action(true).error == GameActions::Status::ok, "Flying native inverted straight failed");
        GameActions::ProcessQueue(state);
        H::EndTerrain();
        Check(H::BeginCustomTrack(GetRide(ride.id)->subtype, ride.type), "Invalid placement fixture begin failed");
        GameActions::ProcessQueue(getGameState());
        C::Update(CoordsXY{ 0, 0 });
        Check(
            C::Action(false).error != GameActions::Status::ok && C::Action(true).error != GameActions::Status::ok,
            "Native invalid bounds accepted");
        capture("authoring-12-native-invalid-placement.png", false);
        H::EndTerrain();
        GameActions::ProcessQueue(getGameState());
        const auto file = std::string(argv[2]) + "/seat-rotation.park";
        SaveFlags flags{};
        Check(ScenarioSave(state, file, flags) && context->LoadParkFromFile(file), "Seat save/reload failed");
        bool savedSeat = false;
        for (auto* e : TileElementsView<TrackElement>(endpoint))
            if (!e->isGhost() && e->getRideIndex() == ride.id)
                savedSeat = e->getSeatRotation() == 10;
        Check(savedSeat, "Native seat parameter not persisted");
        bool savedCovered = false, savedInverted = false;
        for (auto* e : TileElementsView<TrackElement>(coveredOrigin))
            savedCovered |= !e->isGhost() && e->getRideIndex() == coveredRide.id
                && e->getTrackType() == TrackElemType::flatCovered;
        for (auto* e : TileElementsView<TrackElement>(invertedOrigin))
            savedInverted |= !e->isGhost() && e->getRideIndex() == ride.id && e->isInverted() && e->getSeatRotation() == 7;
        Check(savedCovered && savedInverted, "Native variants did not persist");
        std::cout << "Native seat rotation bounds/query/ghost/execute/controller/held-A/cost/persistence passed\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
