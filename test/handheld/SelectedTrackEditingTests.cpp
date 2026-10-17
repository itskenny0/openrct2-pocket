// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include "RideTabNavigation.h"
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
#include <openrct2/actions/cheats/CheatSetAction.h>
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
            FocusRideTabAction("Edit track blocks...");
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
        GameActions::CheatSetAction freeMode(CheatType::noMoney, 1);
        Check(GameActions::Execute(&freeMode, state).error == GameActions::Status::ok, "Native money-mode probe failed");
        GameActions::ProcessQueue(state);
        const auto freeContext = E::Context(current);
        Check(freeContext != baseline && E::Change(current, {}, kMoney64Undefined, true, baseline).error != GameActions::Status::ok
            && E::Context(current) == freeContext && state.park.cash == beforeFailure,
            "Changed money mode accepted an old refund review");
        GameActions::CheatSetAction paidMode(CheatType::noMoney, 0);
        Check(GameActions::Execute(&paidMode, state).error == GameActions::Status::ok, "Native money-mode restore failed");
        GameActions::ProcessQueue(state);
        Check(E::Context(current) == baseline, "Native money-mode restoration changed track context");
        auto stale = baseline;
        stale.back() ^= 1;
        guard.GuardEditingContext(stale);
        Check(GameActions::Execute(&guard, state).error != GameActions::Status::ok
            && E::Context(current) == baseline && state.park.cash == beforeFailure, "Native stale guard mutated state");
        // Native action fixtures: solitary station, conventional rear gap,
        // covered Dinghy and inverted Flying. Every mutation uses native actions.
        for (int family = 0; family < 3; ++family)
        {
            const ObjectEntryIndex idx = family == 0 ? index : family == 1 ? dinghyIndex : flyingIndex;
            const ride_type_t type = family == 0 ? RIDE_TYPE_MULTI_DIMENSION_ROLLER_COASTER
                : family == 1 ? RIDE_TYPE_DINGHY_SLIDE : RIDE_TYPE_FLYING_ROLLER_COASTER;
            Check(H::BeginCustomTrack(idx, type), "Replacement family begin failed");
            GameActions::ProcessQueue(state);
            C::Update(CoordsXY{ 960, 384 + family * 384 });
            Check(C::Action(true).error == GameActions::Status::ok, "Replacement station failed");
            GameActions::ProcessQueue(state);
            auto target = *C::Target();
            auto block = E::Blocks(target).front();
            H::EndTerrain();
            Check(E::CanReplace(block) && !E::Previous(block), "Solitary station replacement unavailable");
            if (family == 0)
            {
                W::HandheldRideOpen(target.id);
                FocusRideTabAction("Edit track blocks...");
                H::HandleMenuInput(controller, confirm);
                H::HandleMenuInput(controller, confirm);
                auto choices = W::HandheldRidesPresentation().rows;
                const auto replaceRow = std::find(choices.begin(), choices.end(), "Replace block...");
                Check(replaceRow != choices.end(), "No-predecessor station control missing");
                W::HandheldMenuMove(static_cast<int>(replaceRow - choices.begin()));
                H::HandleMenuInput(controller, confirm);
                Check(W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 0,
                    "Station review not Cancel-first");
                const auto cancelCash = state.park.cash;
                const auto cancelContext = E::Context(block);
                SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
                Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "SDL controller init failed");
                H::ControllerInput virtualInput;
                const auto device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
                    SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
                Check(device >= 0, "SDL virtual attach failed");
                auto* joystick = SDL_JoystickOpen(device);
                Check(joystick, "SDL virtual open failed");
                auto pump = [&] { SDL_Event event; while (SDL_PollEvent(&event)) virtualInput.handleEvent(event); };
                pump(); virtualInput.update(true, 100); pump(); virtualInput.update(true, 110);
                Check(SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1) == 0, "SDL A failed");
                SDL_JoystickUpdate(); pump();
                const auto frame = virtualInput.update(true, 120);
                Check(frame.actions.size() == 1 && frame.actions.front() == H::Action::confirm
                    && H::HandleMenuInput(virtualInput, frame), "SDL cancel not consumed");
                Check(virtualInput.update(true, 130).actions.empty() && E::Context(block) == cancelContext
                    && state.park.cash == cancelCash, "Held SDL A removed selected station");
                SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
                SDL_JoystickUpdate(); pump(); virtualInput.update(true, 140);
                SDL_JoystickClose(joystick); SDL_JoystickDetachVirtual(device);
                Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
            }
            auto ctx = E::Context(block);
            const auto refund = E::RemoveForReplacement(block);
            const auto startCash = state.park.cash;
            Check(refund.error == GameActions::Status::ok && E::Context(block) == ctx && startCash == state.park.cash,
                "Replacement query impure");
            Check(E::RemoveForReplacement(block, refund.cost + 1, true, {}, ctx).error != GameActions::Status::ok
                && E::Valid(block), "Changed replacement quote removed station");
            Check(E::RemoveForReplacement(block, refund.cost, true, {}, ctx).error == GameActions::Status::ok,
                "Native solitary station removal failed");
            GameActions::ProcessQueue(state);
            Check(state.park.cash == startCash - refund.cost && !E::Valid(block)
                && E::RemoveForReplacement(block, refund.cost, true, {}, ctx).error != GameActions::Status::ok,
                "Station refund/repeat guard failed");
            Check(H::ResumeCustomTrackReplacement(block.ride,
                {block.origin, block.type, block.speed, block.seat, block.chain, block.inverted}, block.colour),
                "Solitary station native handoff failed");
            Check(C::Replacing() && C::PlacementInverted() == block.inverted && C::Endpoint() == block.origin,
                "Replacement variant/origin lost");
            C::Update(CoordsXY{ 320, 320 });
            Check(C::Endpoint() == block.origin, "Selection movement moved reviewed origin");
            const auto rebuildCash = state.park.cash;
            auto rebuild = C::Action(false);
            Check(rebuild.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok
                && C::Action(true).error != GameActions::Status::ok, "Solitary station rebuild/repeat failed");
            GameActions::ProcessQueue(state);
            target = *H::RideManagement::Identify(block.ride.id);
            auto rebuilt = E::Blocks(target);
            Check(rebuilt.size() == 1 && rebuilt.front().origin == block.origin && rebuilt.front().inverted == block.inverted
                && rebuilt.front().colour == block.colour && state.park.cash == rebuildCash - rebuild.cost,
                "Station replacement native accounting/geometry wrong");
            Check(C::Active() && !C::Replacing(), "Station rebuild did not recover construction");
            Check(C::Select(TrackElemType::flat), "Rear-gap straight unsupported");
            if (family == 1) Check(C::ToggleCovered(), "Native covered option unavailable");
            Check(C::Action(true).error == GameActions::Status::ok, "Rear-gap straight failed");
            GameActions::ProcessQueue(state);
            target = *H::RideManagement::Identify(block.ride.id);
            rebuilt = E::Blocks(target);
            auto straight = *std::find_if(rebuilt.begin(), rebuilt.end(), [](const auto& b) {
                return b.type != TrackElemType::endStation && b.type != TrackElemType::beginStation && b.type != TrackElemType::middleStation;
            });
            H::EndTerrain();
            // Remove only the station natively to expose an actual no-predecessor straight.
            auto stationBlock = *std::find_if(rebuilt.begin(), rebuilt.end(), [](const auto& b) { return b.type == TrackElemType::endStation; });
            Check(E::Change(stationBlock, {}, kMoney64Undefined, true).error == GameActions::Status::ok,
                "Rear-gap fixture station removal failed");
            GameActions::ProcessQueue(state);
            target = *H::RideManagement::Identify(block.ride.id);
            straight = E::Blocks(target).front();
            Check(!E::Previous(straight) && E::CanReplace(straight), "Native rear-gap replacement unavailable");
            ctx = E::Context(straight);
            const auto removeQuote = E::RemoveForReplacement(straight);
            Check(removeQuote.error == GameActions::Status::ok, "Rear-gap removal quote failed");
            Check(E::RemoveForReplacement(straight, removeQuote.cost, true, {}, ctx).error == GameActions::Status::ok,
                "Rear-gap native removal failed");
            GameActions::ProcessQueue(state);
            Check(H::ResumeCustomTrackReplacement(straight.ride,
                {straight.origin, straight.type, straight.speed, straight.seat, straight.chain, straight.inverted}, straight.colour),
                "Rear-gap replacement handoff failed");
            Check(C::Selection() == straight.type && C::PlacementInverted() == straight.inverted,
                "Rear-gap covered/inverted parameters changed");
            const auto gapCash = state.park.cash;
            auto gapContext = TrackConstructionSupport::EditingContext(*GetRide(straight.ride.id));
            auto failed = GameActions::SequentialTrackPlaceAction(straight.ride.id, straight.type, straight.origin, straight.chain,
                false, {}, TrackElemType::flat);
            failed.SetPlacementInverted(straight.inverted);
            failed.UseDetached(gapContext);
            failed.SetPlacementColour(4);
            Check(GameActions::Execute(&failed, state).error != GameActions::Status::ok && state.park.cash == gapCash
                && TrackConstructionSupport::EditingContext(*GetRide(straight.ride.id)) == gapContext,
                "Failed native placement changed gap/cash");
            if (family == 0)
            {
                C::Update({});
                const auto cancelGeometry = TrackConstructionSupport::EditingContext(*GetRide(straight.ride.id));
                H::HandleExplorerInput(controller, back);
                Check(!C::Active() && state.park.cash == gapCash
                    && TrackConstructionSupport::EditingContext(*GetRide(straight.ride.id)) == cancelGeometry,
                    "B cancelled rebuild by removing another block");
                Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
                Check(H::ResumeCustomTrackReplacement(straight.ride,
                    {straight.origin, straight.type, straight.speed, straight.seat, straight.chain, straight.inverted}, straight.colour),
                    "Native explicit rebuild return failed");
            }
            GameActions::CheatSetAction exhaustedFunds(CheatType::setMoney, 0);
            Check(GameActions::Execute(&exhaustedFunds, state).error == GameActions::Status::ok, "Native funds probe failed");
            GameActions::ProcessQueue(state);
            const auto unfundedContext = TrackConstructionSupport::EditingContext(*GetRide(straight.ride.id));
            Check(C::Action(true).error != GameActions::Status::ok && state.park.cash == 0
                && TrackConstructionSupport::EditingContext(*GetRide(straight.ride.id)) == unfundedContext && C::Replacing(),
                "Unfunded native placement changed the gap or lost recovery");
            GameActions::CheatSetAction restoreFunds(CheatType::setMoney, gapCash);
            Check(GameActions::Execute(&restoreFunds, state).error == GameActions::Status::ok, "Native funds restore failed");
            GameActions::ProcessQueue(state);
            const auto gapQuote = C::Action(false);
            Check(gapQuote.error == GameActions::Status::ok && C::Action(true).error == GameActions::Status::ok,
                "Rear-gap explicit rebuild failed");
            GameActions::ProcessQueue(state);
            target = *H::RideManagement::Identify(straight.ride.id);
            auto resultBlock = E::Blocks(target).front();
            Check(resultBlock.origin == straight.origin && resultBlock.type == straight.type && resultBlock.inverted == straight.inverted
                && state.park.cash == gapCash - gapQuote.cost, "Rear-gap geometry/accounting changed");
            H::EndTerrain();
        }
        const auto file = (std::filesystem::path(argv[2]) / "selected-track-roundtrip.park").string();
        const auto savedCash = state.park.cash;
        auto targets = H::RideManagement::Roster();
        std::vector<std::vector<E::Block>> savedBlocks;
        for (const auto& t : targets) savedBlocks.push_back(E::Blocks(t));
        Check(ScenarioSave(state, file, {}) && context->LoadParkFromFile(file), "Selected track native roundtrip failed");
        Check(!C::Active() && state.park.cash == savedCash, "Load kept transient rebuild/cash changed");
        for (size_t n = 0; n < targets.size(); ++n)
        {
            const auto now = H::RideManagement::Identify(targets[n].id);
            Check(now.has_value(), "Saved ride missing");
            const auto restored = E::Blocks(*now);
            Check(restored.size() == savedBlocks[n].size(), "Saved block count changed");
            for (size_t j = 0; j < restored.size(); ++j)
                Check(restored[j].origin == savedBlocks[n][j].origin && restored[j].type == savedBlocks[n][j].type
                    && restored[j].seat == savedBlocks[n][j].seat && restored[j].speed == savedBlocks[n][j].speed
                    && restored[j].inverted == savedBlocks[n][j].inverted && restored[j].colour == savedBlocks[n][j].colour,
                    "Native saved block parameters changed");
        }
        std::cout << "Selected-track inspection/context native/SDL PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
