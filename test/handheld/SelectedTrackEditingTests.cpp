// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
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
        namespace E = H::TrackEditing;
        Check(H::BeginCustomTrack(index, RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER), "Begin failed");
        GameActions::ProcessQueue(state);
        C::Update(CoordsXY{ 640, 640 });
        Check(C::Action(true).error == GameActions::Status::ok, "Station failed");
        GameActions::ProcessQueue(state);
        auto ride = *C::Target();
        auto station = E::Blocks(ride).front();
        auto baseline = E::Context(station);
        auto cash = state.park.cash;
        auto info = E::Inspection(station);
        Check(!E::Previous(station), "No-predecessor fixture connected unexpectedly");
        Check(std::find(info.begin(), info.end(), "Previous: disconnected end") != info.end(), "Missing native endpoint detail");
        Check(std::any_of(info.begin(), info.end(), [](const auto& row) { return row.find("Station 1") == 0; }), "Missing native station identity");
        Check(E::Change(station, {}).error == GameActions::Status::ok && E::Context(station) == baseline
            && state.park.cash == cash, "Inspection/query mutated native state");
        H::EndTerrain();
        H::ControllerInput controller;
        H::Frame confirm{}, back{};
        confirm.actions.push_back(H::Action::confirm);
        back.actions.push_back(H::Action::back);
        auto openBlock = [&] {
            W::HandheldRideOpen(ride.id);
            W::HandheldMenuMove(3 + (H::RideAccess::Supported(ride) ? 1 : 0) + (C::CanResume(ride) ? 1 : 0));
            H::HandleMenuInput(controller, confirm);
            H::HandleMenuInput(controller, confirm);
        };
        openBlock();
        auto rows = W::HandheldRidesPresentation().rows;
        auto inspect = std::find(rows.begin(), rows.end(), "Inspect connections / station...");
        Check(inspect != rows.end(), "Context control missing");
        W::HandheldMenuMove(static_cast<int>(inspect - rows.begin()));
        H::HandleMenuInput(controller, confirm);
        Check(W::HandheldRidesPresentation().title == "NATIVE TRACK CONTEXT", "Native context page missing");
        H::HandleMenuInput(controller, back);
        H::HandleMenuInput(controller, back);
        Check(W::HandheldRidesPresentation().selected == 0 && E::Context(station) == baseline
            && state.park.cash == cash, "Context cancel changed selection/map/cash");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Check(C::ResumeFrom(ride, std::pair{station.origin, station.type}), "Resume failed");
        Check(C::Select(TrackElemType::endStation), "Station extension missing");
        Check(C::Action(true).error == GameActions::Status::ok, "Station extension failed");
        GameActions::ProcessQueue(state);
        H::EndTerrain();
        const auto beforeFailure = state.park.cash;
        Check(E::Change(station, {}, kMoney64Undefined, true, baseline).error != GameActions::Status::ok
            && state.park.cash == beforeFailure, "Changed station context accepted");
        ride = *H::RideManagement::Identify(ride.id);
        const auto blocks = E::Blocks(ride);
        Check(blocks.size() == 2, "Native station block count wrong");
        auto current = blocks.front();
        baseline = E::Context(current);
        GameActions::SequentialTrackPlaceAction guard(ride.id, current.type, current.origin, current.chain,
            false, current.origin, current.type);
        guard.UseSelectedRemoval(current.speed, current.inverted, current.seat);
        guard.GuardEditingContext(baseline);
        Check(GameActions::Query(&guard, state).error == GameActions::Status::ok, "Exact guard rejected valid station");
        auto stale = baseline;
        stale.back() ^= 1;
        guard.GuardEditingContext(stale);
        Check(GameActions::Execute(&guard, state).error != GameActions::Status::ok
            && E::Context(current) == baseline && state.park.cash == beforeFailure, "Native stale guard mutated state");
        std::cout << "Selected-track inspection/context native/SDL PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
