#include "PeopleTabNavigation.h"
// SPDX-License-Identifier: GPL-3.0-only
// Real SDL/window lifecycle with public engine data; graphics-disabled native park fixture.
#include "RideTabNavigation.h"

#include <SDL.h>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/AreaTools.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideManagement.h>
#include <openrct2-ui/handheld/RightsTools.h>
#include <openrct2-ui/handheld/SceneryTools.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2-ui/handheld/StaffPatrol.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/peep/StaffHireNewAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <stdexcept>

using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        auto env = CreatePlatformEnvironment();
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Require(context->Initialise(), "Public-data UI fixture initialise failed");
        context->GetUiContext().CreateWindow();
        auto& state = getGameState();
        gameStateInitAll(state, { 16, 16 });
        gLegacyScene = LegacyScene::playing;
        Require(W::MainOpen(), "Fixture main viewport missing");
        InputSetState(InputState::normal);
        // Graphics-disabled context initialization leaves its native progress window pending a UI tick.
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        // UI metrics can use public G2 fonts without any original park/world sprite assets.
        gOpenRCT2NoGraphics = false;
        GfxLoadG2PalettesFontsTracks();
        FontSpriteInitialiseCharacters();
        auto* wm = Ui::GetWindowManager();
        H::ControllerInput input;
        auto top = [&] { return wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 3 }); };
        H::UpdateExplorer(input);
        Require(top(), "Exploration HUD window missing");
        Require(
            top()->windowPos == ScreenCoordsXY{ H::Layout::margin, H::Layout::hudTop } && top()->height == H::Layout::hudHeight,
            "HUD window/safe area disagrees");
        const auto cash = state.park.cash;
        const auto cashLabel = H::NativeHud().cash;
        H::BeginWater();
        H::UpdateExplorer(input);
        Require(top(), "Construction lost persistent HUD");
        const auto* tools = wm->FindByNumber(WindowClass::handheldHud, WindowNumber{ 1 });
        Require(tools && tools->windowPos == H::Layout::toolOrigin, "Construction panel didn't use shared inset");
        W::HandheldDashboardOpen();
        H::UpdateExplorer(input);
        Require(!top(), "Dashboard retained colliding HUD window");
        W::HandheldMenuBack();
        H::UpdateExplorer(input);
        Require(top(), "Returning to park did not recreate HUD");
        W::HandheldBuildOpen();
        H::UpdateExplorer(input);
        Require(!top(), "Build menu retained HUD");
        W::HandheldMenuBack();
        H::UpdateExplorer(input);
        // Published slate navigation returns root Build directly to the park.
        Require(
            !Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu) && top(),
            "Root Build cancellation lost park/HUD return");
        H::EndTerrain();
        // Metadata-only fixture for type-specific management; no fabricated operating track or guests.
        namespace R = H::RideManagement;
        auto* object = context->GetObjectManager().LoadObject("rct2.ride.ptct1");
        Require(object, "Public wooden object metadata missing");
        auto* ride = RideAllocateAtIndex(RideId::FromUnderlying(0));
        Require(ride, "Ride UI fixture allocation failed");
        // Bare allocation does not perform RideCreateAction's station initialization.
        // This metadata-only fixture has no track or access; zero coordinates are real map positions.
        for (auto& station : ride->getStations())
        {
            station.start.setNull();
            station.entrance.setNull();
            station.exit.setNull();
        }
        ride->type = RIDE_TYPE_WOODEN_ROLLER_COASTER;
        ride->subtype = context->GetObjectManager().GetLoadedObjectEntryIndex(object);
        ride->status = RideStatus::closed;
        auto target = *R::Identify(ride->id);
        auto settings = R::Settings(*ride);
        auto lift = std::find_if(settings.begin(), settings.end(), [](const auto& s) {
            return s.key == GameActions::RideSetSetting::liftHillSpeed;
        });
        Require(lift != settings.end(), "Wooden lift missing");
        auto liftDraft = *R::BeginSetting(target, *lift);
        for (int i = 0; i < 300; ++i)
            R::AdjustPrice(liftDraft, 1);
        Require(liftDraft.value == ride->getRideTypeDescriptor().LiftData.maximum_speed, "Native lift bound wrong");
        const bool originalNoMoney = state.park.flags.has(ParkFlag::noMoney);
        state.park.flags.unset(ParkFlag::noMoney);
        const auto settingsCash = state.park.cash;
        auto waiting = std::find_if(settings.begin(), settings.end(), [](const auto& s) {
            return s.key == GameActions::RideSetSetting::minWaitingTime;
        });
        Require(waiting != settings.end(), "Coaster waits missing");
        auto waitingDraft = *R::BeginSetting(target, *waiting);
        R::AdjustPrice(waitingDraft, 1);
        state.park.flags.set(ParkFlag::noMoney); // Reversible mode fixture, action still authoritative.
        Require(R::Action(waitingDraft, true).error != GameActions::Status::ok, "Stale no-money draft accepted");
        waitingDraft = *R::BeginSetting(target, *waiting);
        R::AdjustPrice(waitingDraft, 1);
        Require(R::Action(waitingDraft, true).error == GameActions::Status::ok, "Paused no-money wait failed");
        GameActions::ProcessQueue(state);
        Require(
            ride->minWaitingTime == waitingDraft.value && ride->maxWaitingTime >= ride->minWaitingTime
                && state.park.cash == settingsCash,
            "Native coupled wait/no-money accounting wrong");
        Require(R::Action(waitingDraft, true).error != GameActions::Status::ok, "Changed settings snapshot accepted");
        state.park.flags.unset(ParkFlag::noMoney);
        settings = R::Settings(*ride);
        const auto modes = std::find_if(
            settings.begin(), settings.end(), [](const auto& s) { return s.key == GameActions::RideSetSetting::mode; });
        Require(modes != settings.end(), "Wooden modes missing");
        auto modeDraft = *R::BeginSetting(target, *modes);
        R::AdjustPrice(modeDraft, modeDraft.value == modes->values.back() ? -1 : 1);
        GameActions::RideSetSettingAction modeNative(target.id, modes->key, modeDraft.value);
        Require(
            R::Action(modeDraft, false).error == GameActions::Query(&modeNative, state).error,
            "Native available-mode query differs");
        Require(R::Action(modeDraft, true).error == GameActions::Status::ok, "Native closed mode change failed");
        GameActions::ProcessQueue(state);
        Require(
            static_cast<uint8_t>(ride->mode) == modeDraft.value && state.park.cash == settingsCash, "Native mode not applied");
        auto beforeLoad = *R::BeginSetting(target, R::DispatchSettings(*ride).front());
        R::AdjustPrice(beforeLoad, beforeLoad.value == beforeLoad.setting->values.back() ? -1 : 1);
        // Exercise the actual WindowInitAll reset hook on this metadata-only fixture.
        // Save/reload of real native placements is covered by VehicleDispatchTests.
        wm->CloseAllExceptFlags({});
        Require(
            !R::Resolve(target) && R::Action(beforeLoad, true).error != GameActions::Status::ok,
            "Real WindowInitAll allowed pre-load ride draft");
        Require(W::MainOpen(), "Reset fixture viewport missing");
        ride = GetRide(target.id);
        target = *R::Identify(target.id);
        // Controller paths reuse the real menu handler and duplicate-action boundary.
        auto press = [&](H::Action action) {
            H::Frame frame;
            frame.actions = { action, action };
            Require(H::HandleMenuInput(input, frame), "Ride depth semantic handoff failed");
        };
        const auto focusRideAction = [&](const std::string& label) { FocusRideTabAction(label); };
        std::cout << "Native wooden mode / lift bounds / coupled waiting / no-money passed" << std::endl;
        W::HandheldDashboardOpen();
        W::HandheldDashboardMove(0, 1);
        press(H::Action::confirm);
        press(H::Action::confirm);
        auto model = W::HandheldRidesPresentation();
        W::HandheldDashboardPage(1);
        W::HandheldDashboardPage(1);
        W::HandheldDashboardMove(0, 2);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().breadcrumb == "Rides / Graphs" && ride->measurement
                && W::HandheldRidesPresentation().graph.empty(),
            "Native graph registration/waiting route absent");
        // Isolated conversion/render fixture, explicitly synthetic, not an operating coaster.
        auto nativeMeasurement = std::move(ride->measurement);
        ride->measurement = std::make_unique<RideMeasurement>();
        ride->measurement->flags.set(RideMeasurementFlag::running, RideMeasurementFlag::gForces);
        ride->measurement->num_items = 2;
        ride->measurement->velocity[0] = 20;
        ride->measurement->altitude[0] = 28;
        ride->measurement->vertical[0] = -10;
        const auto graphSpeed = R::Presentation(R::View::graphs, 0, {}, target, {}, {}, 0);
        const auto graphAltitude = R::Presentation(R::View::graphs, 0, {}, target, {}, {}, 1);
        const auto graphForce = R::Presentation(R::View::graphs, 0, {}, target, {}, {}, 2);
        Require(
            graphSpeed.graph.front() == 9 && graphAltitude.graph.front() == 10 && graphForce.graph.front() == -80,
            "Native graph unit conversions differ");
        Require(
            R::Presentation(R::View::graphs, 0, {}, target, {}, {}, 0, 99999).graph.size() == 1,
            "Graph scroll reads beyond samples");
        ride->measurement = std::move(nativeMeasurement);
        press(H::Action::back);
        focusRideAction("Rename...");
        press(H::Action::confirm);
        const auto beforeName = ride->getName();
        Require(
            W::HandheldRidesPresentation().columns == 8 && W::HandheldRidesPresentation().rows.size() == 48,
            "Name keyboard grid absent");
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().title == beforeName + "A" && ride->getName() == beforeName,
            "Duplicate A inserted/applied more than one key");
        press(H::Action::openBuild);
        Require(!W::HandheldDashboardRailFocused(), "Name keyboard escaped to rail");
        W::HandheldDashboardMove(6, 5);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1,
            "Name review not Cancel-first");
        press(H::Action::confirm);
        Require(
            ride->getName() == beforeName && W::HandheldRidesPresentation().columns == 8,
            "Name cancel changed name/lost keyboard");
        press(H::Action::back);
        Require(W::HandheldRidesPresentation().breadcrumb == "Park dashboard / Rides", "Name back lost inspector");
        focusRideAction("Appearance...");
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Appearance", "Appearance route lost");
        press(H::Action::back);
        focusRideAction("Music...");
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Music", "Music route lost");
        press(H::Action::openBuild);
        Require(W::HandheldDashboardRailFocused(), "Music rail switch lost");
        press(H::Action::openBuild);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Music", "Music resume lost");
        press(H::Action::back);
        focusRideAction("Dispatch...");
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Dispatch", "Dispatch route lost");
        press(H::Action::openBuild);
        Require(W::HandheldDashboardRailFocused(), "Dispatch Select failed");
        press(H::Action::openBuild);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Dispatch", "Dispatch resume lost");
        const auto dispatchBefore = ride->departFlags;
        // The offscreen SDL test window cannot acquire foreground device focus.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        Require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Dispatch SDL init failed");
        {
            H::ControllerInput controller;
            controller.update(true, 9000);
            const auto device = SDL_JoystickAttachVirtual(
                SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
            auto* joystick = SDL_JoystickOpen(device);
            Require(joystick, "Dispatch virtual controller missing");
            auto pump = [&] {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                    controller.handleEvent(event);
            };
            pump();
            controller.update(true, 9010);
            pump();
            controller.update(true, 9020);
            SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, true);
            SDL_JoystickUpdate();
            pump();
            const auto frame = controller.update(true, 9030);
            Require(frame.actions.size() == 1 && H::HandleMenuInput(controller, frame), "Dispatch selecting A lost");
            Require(
                controller.update(true, 9040).actions.empty() && controller.update(true, 10040).actions.empty()
                    && W::HandheldRidesPresentation().editing && ride->departFlags == dispatchBefore,
                "Held selecting A adjusted/applied dispatch");
            SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, false);
            SDL_JoystickUpdate();
            pump();
            Require(controller.update(true, 10050).actions.empty(), "Dispatch release acted");
            SDL_JoystickClose(joystick);
            SDL_JoystickDetachVirtual(device);
        }
        Require(W::HandheldRidesPresentation().editing, "Dispatch selecting A spilled into action");
        W::HandheldDashboardMove(1, 0);
        W::HandheldDashboardMove(0, 2);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1,
            "Dispatch review not Cancel-first");
        press(H::Action::confirm);
        Require(
            ride->departFlags == dispatchBefore && state.park.cash == settingsCash
                && W::HandheldRidesPresentation().breadcrumb == "Rides / Dispatch",
            "Cancel changed dispatch/parent");
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        focusRideAction("Vehicles...");
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().breadcrumb == "Rides / Vehicles" && W::HandheldRidesPresentation().rows.size() == 1,
            "Trackless vehicle page fabricated editable counts");
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        focusRideAction("Operations...");
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Operations", "Operations route lost");
        press(H::Action::openBuild);
        Require(W::HandheldDashboardRailFocused(), "Operations Select did not return to rail");
        press(H::Action::openBuild);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Operations", "Operations resume lost");
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().editing, "Operations entry spilled into action");
        press(H::Action::openBuild);
        Require(!W::HandheldDashboardRailFocused(), "Setting draft escaped to rail");
        W::HandheldDashboardMove(1, 0);
        W::HandheldDashboardMove(0, 2);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1,
            "Operations review not Cancel-first");
        press(H::Action::back);
        Require(W::HandheldRidesPresentation().editing, "Review B did not restore settings draft");
        press(H::Action::back);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Operations", "Draft B did not return to Operations");
        GameActions::RideSetSettingAction continuousMode(
            target.id, GameActions::RideSetSetting::mode, static_cast<uint8_t>(RideMode::continuousCircuit));
        Require(GameActions::Execute(&continuousMode, state).error == GameActions::Status::ok, "Circuit UI native mode failed");
        GameActions::ProcessQueue(state);
        GameActions::RideSetSettingAction initialCircuits(target.id, GameActions::RideSetSetting::numCircuits, 1);
        Require(GameActions::Execute(&initialCircuits, state).error == GameActions::Status::ok, "Circuit UI baseline failed");
        GameActions::ProcessQueue(state);
        const auto operationSettings = R::Settings(*ride);
        const auto circuitSetting = std::find_if(operationSettings.begin(), operationSettings.end(), [](const auto& s) {
            return s.key == GameActions::RideSetSetting::numCircuits;
        });
        Require(circuitSetting != operationSettings.end(), "Circuit controller setting absent");
        const auto circuitRow = circuitSetting - operationSettings.begin();
        W::HandheldDashboardMove(0, circuitRow);
        {
            H::ControllerInput controller;
            controller.update(true, 11000);
            const auto device = SDL_JoystickAttachVirtual(
                SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
            auto* joystick = SDL_JoystickOpen(device);
            Require(joystick, "Mode virtual controller missing");
            auto pump = [&] {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                    controller.handleEvent(event);
            };
            pump();
            controller.update(true, 11010);
            pump();
            controller.update(true, 11020);
            SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, true);
            SDL_JoystickUpdate();
            pump();
            const auto frame = controller.update(true, 11030);
            Require(frame.actions.size() == 1 && H::HandleMenuInput(controller, frame), "Mode selecting A lost");
            Require(
                controller.update(true, 11040).actions.empty() && controller.update(true, 12040).actions.empty()
                    && W::HandheldRidesPresentation().editing && ride->numCircuits == 1,
                "Held mode selecting A changed/applied setting");
            SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, false);
            SDL_JoystickUpdate();
            pump();
            Require(controller.update(true, 12050).actions.empty(), "Mode A release acted");
            SDL_JoystickClose(joystick);
            SDL_JoystickDetachVirtual(device);
        }
        W::HandheldDashboardMove(1, 0);
        W::HandheldDashboardMove(0, 2);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1,
            "Mode parameter review not Cancel-first");
        press(H::Action::confirm);
        Require(
            ride->numCircuits == 1 && state.park.cash == settingsCash
                && W::HandheldRidesPresentation().breadcrumb == "Rides / Operations",
            "Mode cancellation changed state/parent");
        W::HandheldDashboardMove(0, circuitRow);
        press(H::Action::confirm);
        W::HandheldDashboardMove(1, 0);
        W::HandheldDashboardMove(0, 2);
        press(H::Action::confirm);
        W::HandheldDashboardMove(0, -1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(
            ride->numCircuits == 2 && state.park.cash == settingsCash
                && W::HandheldRidesPresentation().breadcrumb == "Rides / Operations",
            "Circuit confirm/input return failed");
        std::cout << "SDL mode-specific Operations / held select / Cancel-first / one native confirm / parent return passed"
                  << std::endl;
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        focusRideAction("Demolish...");
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1,
            "Demolition default/input handoff wrong");
        Require(GameIsNotPaused(), "Dashboard-owned pause blocked demolition review");
        press(H::Action::confirm);
        Require(
            R::Resolve(target) && state.park.cash == settingsCash && GameIsPaused(),
            "Cancel demolished/charged or lost Dashboard pause");
        model = W::HandheldRidesPresentation();
        focusRideAction("Maintenance...");
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Maintenance", "Maintenance route lost");
        press(H::Action::confirm);
        W::HandheldDashboardMove(1, 0);
        W::HandheldDashboardMove(0, 2);
        press(H::Action::confirm);
        W::HandheldDashboardMove(0, -1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(
            W::HandheldRidesPresentation().breadcrumb == "Rides / Maintenance"
                && ride->inspectionInterval != RideInspection::every10Minutes && state.park.cash == settingsCash,
            "Confirmed maintenance did not return safely");
        auto maintenance = W::HandheldRidesPresentation();
        W::HandheldDashboardMove(0, maintenance.rows.size() - 2);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1
                && W::HandheldRidesPresentation().breadcrumb == "Rides / Refurbish",
            "Refurbishment review not Cancel-first");
        Require(GameIsNotPaused(), "Dashboard pause blocked refurbishment review");
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().breadcrumb == "Rides / Maintenance" && R::Resolve(target)
                && state.park.cash == settingsCash && GameIsPaused(),
            "Refurbish cancel changed state/pause/parent");
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        focusRideAction("Demolish...");
        press(H::Action::confirm);
        press(H::Action::openMenu);
        press(H::Action::back);
        Require(R::Resolve(target) && state.park.cash == settingsCash, "Start applied destructive draft");
        std::cout << "SDL operations / resume / draft guards / Cancel-first demolition passed" << std::endl;
        auto stale = *R::BeginDemolition(target);
        R::Invalidate(target.id);
        Require(R::Action(stale, true).error != GameActions::Status::ok, "Reused identity accepted demolition");
        W::HandheldDashboardOpen();
        W::HandheldDashboardMove(0, 1);
        press(H::Action::confirm);
        press(H::Action::confirm);
        model = W::HandheldRidesPresentation();
        focusRideAction("Demolish...");
        press(H::Action::confirm);
        W::HandheldDashboardMove(0, -1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(
            W::HandheldRidesPresentation().rows.size() == 1 && GameIsPaused(),
            "Confirmed demolition did not return to paused roster");
        press(H::Action::back);
        press(H::Action::back);
        Require(!GetRide(target.id), "Fixture record survived cleanup");
        // Pre-existing player pause is not owned by Dashboard and must never be bypassed.
        ride = RideAllocateAtIndex(target.id);
        Require(ride, "Paused identity fixture allocation failed");
        ride->type = RIDE_TYPE_WOODEN_ROLLER_COASTER;
        ride->subtype = target.object;
        ride->status = RideStatus::closed;
        gGamePaused = GAME_PAUSED_NORMAL;
        W::HandheldDashboardOpen();
        W::HandheldDashboardMove(0, 1);
        press(H::Action::confirm);
        press(H::Action::confirm);
        model = W::HandheldRidesPresentation();
        focusRideAction("Demolish...");
        press(H::Action::confirm);
        W::HandheldDashboardMove(0, -1);
        press(H::Action::confirm);
        Require(
            GetRide(target.id) && gGamePaused == GAME_PAUSED_NORMAL && W::HandheldRidesPresentation().confirmation,
            "Pre-existing pause bypassed demolition");
        press(H::Action::back);
        press(H::Action::back);
        press(H::Action::back);
        press(H::Action::back);
        Require(gGamePaused == GAME_PAUSED_NORMAL, "Dashboard cleared player pause");
        gGamePaused = 0; // End reversible pause fixture, then remove via native action.
        GameActions::RideDemolishAction remove(target.id, GameActions::RideModifyType::demolish);
        Require(GameActions::Execute(&remove, state).error == GameActions::Status::ok, "Paused fixture cleanup failed");
        GameActions::ProcessQueue(state);
        state.park.flags.set(ParkFlag::noMoney, originalNoMoney);
        std::cout << "Native stale ride / cleanup passed" << std::endl;
        // A shop enters naming outside Dashboard; the keyboard must take content focus.
        state.park.flags.set(ParkFlag::noMoney);
        auto& objects = context->GetObjectManager();
        auto* shopObject = objects.LoadObject("rct2.ride.burgb");
        Require(shopObject, "Shop keyboard fixture object missing");
        const auto shopIndex = objects.GetLoadedObjectEntryIndex(shopObject);
        ResearchInsertRideEntry(shopIndex, true);
        RideEntrySetInvented(shopIndex);
        RideTypeSetInvented(ShopPlacement::Type(shopIndex));
        const CoordsXY shopTile{ 64, 352 };
        MapGetSurfaceElementAt(shopTile)->setOwnership({ OwnershipFlag::landOwned });
        Require(
            H::Shops::Action(shopTile, shopIndex, 0, true).error == GameActions::Status::ok, "Shop keyboard placement failed");
        GameActions::ProcessQueue(state);
        const auto shopId = (*TileElementsView<TrackElement>(shopTile).begin())->getRideIndex();
        W::HandheldDashboardOpen();
        Require(W::HandheldDashboardRailFocused(), "Shop keyboard rail fixture missing");
        W::HandheldRideOpen(shopId);
        W::HandheldMenuMove(3);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().columns == 8 && !W::HandheldDashboardRailFocused(),
            "Shop Rename failed to take keyboard content focus");
        const auto shopName = GetRide(shopId)->getName();
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().title == shopName + "A" && GetRide(shopId)->getName() == shopName,
            "Shop keyboard A did not type exactly once into its draft");
        press(H::Action::back);
        Require(W::HandheldRidesPresentation().columns != 8, "Shop keyboard B failed to return");
        press(H::Action::back);
        GameActions::RideDemolishAction removeShop(shopId, GameActions::RideModifyType::demolish);
        Require(GameActions::Execute(&removeShop, state).error == GameActions::Status::ok, "Shop keyboard cleanup failed");
        GameActions::ProcessQueue(state);
        state.park.flags.set(ParkFlag::noMoney, originalNoMoney);
        Require(state.park.cash == cash, "Shop keyboard fixture changed accounting");
        std::cout << "SDL shop keyboard / rail handoff / typing-cancel passed" << std::endl;
        auto* wallObject = objects.LoadObject("rct2.scenery_wall.wbrg");
        Require(wallObject, "Scenery SDL wall fixture missing");
        ScenerySetInvented({ SCENERY_TYPE_WALL, objects.GetLoadedObjectEntryIndex(wallObject) });
        W::HandheldBuildOpen();
        W::HandheldMenuMove(10);
        press(H::Action::confirm);
        Require(!W::HandheldObjectPickerIsOpen(), "Duplicate A skipped scenery category");
        W::HandheldMenuMove(2);
        press(H::Action::confirm);
        Require(W::HandheldObjectPickerIsOpen(), "Scenery category failed to open picker");
        press(H::Action::confirm);
        Require(H::IsSceneryToolActive() && !W::HandheldObjectPickerIsOpen(), "Scenery selection failed to return to park");
        W::HandheldSceneryOptionsOpen();
        const auto wallDirection = H::SceneryTools::Current().direction;
        W::HandheldMenuMove(2);
        press(H::Action::confirm);
        Require(H::SceneryTools::Current().direction == (wallDirection + 1) % 4, "Duplicate A rotated scenery twice");
        press(H::Action::back);
        Require(H::IsSceneryToolActive(), "Scenery options cancel lost tool");
        H::EndTerrain();
        std::cout << "SDL scenery category / picker / duplicate A / options-return passed" << std::endl;
        H::BeginAreaTools();
        W::HandheldAreaOptionsOpen();
        W::HandheldMenuMove(2);
        const auto brushWidth = H::AreaTools::settings.width;
        press(H::Action::confirm);
        Require(H::AreaTools::settings.width == brushWidth + 1, "Duplicate A resized brush twice");
        W::HandheldConstructionMove(-1, 0);
        Require(H::AreaTools::settings.width == brushWidth, "Brush left adjustment failed");
        press(H::Action::back);
        H::EndTerrain();
        std::cout << "SDL area brush / duplicate A / bounded adjustment / return passed" << std::endl;
        const CoordsXY rightsTile{ 352, 352 };
        MapGetSurfaceElementAt(rightsTile)->setOwnership({ OwnershipFlag::landForSale });
        const auto rightsCash = state.park.cash;
        state.park.flags.set(ParkFlag::noMoney);
        H::RightsTools::settings = {};
        H::BeginRightsTools();
        W::HandheldRightsReviewOpen(rightsTile);
        Require(W::HandheldConstructionMenuIsOpen() && GameIsNotPaused(), "Rights review blocked by owned menu pause");
        press(H::Action::confirm);
        Require(
            state.park.cash == rightsCash && !MapGetSurfaceElementAt(rightsTile)->hasOwnership(OwnershipFlag::landOwned),
            "Rights review not Cancel-first");
        W::HandheldRightsReviewOpen(rightsTile);
        W::HandheldMenuMove(-1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(MapGetSurfaceElementAt(rightsTile)->hasOwnership(OwnershipFlag::landOwned), "SDL rights confirmation failed");
        // Reversible UI fixture finishes with original cash via no-money native purchase.
        state.park.flags.set(ParkFlag::noMoney, originalNoMoney);
        Require(state.park.cash == rightsCash, "SDL rights no-money accounting differs");
        H::EndTerrain();
        std::cout << "SDL land review / owned-pause handoff / Cancel-first / single native confirmation passed" << std::endl;
        W::HandheldBuildOpen();
        W::HandheldMenuMove(2);
        press(H::Action::confirm);
        W::HandheldMenuMove(4);
        press(H::Action::confirm);
        Require(H::Paths::IsConnected() && !H::Paths::ConnectedReady(), "Connected UI skipped endpoint selection");
        W::HandheldPathOptionsOpen();
        W::HandheldMenuMove(1);
        press(H::Action::confirm);
        press(H::Action::back);
        Require(H::Paths::IsConnected() && !H::Paths::ConnectedReady(), "Connected options cancel lost context");
        H::EndTerrain();
        Require(!H::Paths::IsActive(), "Connected park return leaked tool");
        std::cout << "SDL connected picker / held-A handoff / options-return / cleanup passed" << std::endl;
        namespace S = H::StaffManagement;
        gOpenRCT2NoGraphics = true;
        Require(
            objects.LoadObject("rct2.peep_animations.entertainer_elephant")
                && objects.LoadObject("rct2.peep_animations.entertainer_tiger"),
            "SDL costume metadata missing");
        const auto oldEntrances = state.park.entrances;
        state.park.entrances.push_back({ 64, 64, 16, 0 });
        const auto employeeCount = S::Roster().size();
        W::HandheldStaffOpen();
        // Hire is the persistent first toolbar action.
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().title == "Choose staff role", "Duplicate A skipped role picker");
        W::HandheldMenuMove(3);
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().selected == 1, "Hire did not default to Cancel");
        press(H::Action::confirm);
        Require(S::Roster().size() == employeeCount, "Cancel hired entertainer");
        W::HandheldStaffOpen();
        // Hire is the persistent first toolbar action.
        press(H::Action::confirm);
        W::HandheldMenuMove(3);
        press(H::Action::confirm);
        W::HandheldMenuMove(-1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        auto employee = S::Roster().back();
        Require(employee.type == StaffType::entertainer, "SDL entertainer hire failed");
        const auto oldCostume = S::Resolve(employee)->animationObjectIndex;
        FocusPersonAction(true, "Costume...");
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().breadcrumb == "Staff / Entertainer costume", "Costume picker route failed");
        const auto costumes = S::Costumes();
        const auto chosen = costumes.front().first == oldCostume ? 1 : 0;
        W::HandheldMenuMove(chosen);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(
            S::Resolve(employee)->animationObjectIndex == costumes[chosen].first && state.park.cash == cash,
            "Costume action/held A accounting failed");
        FocusPersonAction(true, "Patrol area...");
        press(H::Action::confirm);
        Require(H::StaffPatrol::Active() && !W::HandheldStaffIsOpen(), "Patrol inspector route failed");
        Require(
            H::StaffPatrol::Action(CoordsXY{ 320, 320 }, true).error == GameActions::Status::ok,
            "SDL native patrol seed failed");
        GameActions::ProcessQueue(state);
        W::HandheldStaffPatrolOptionsOpen();
        W::HandheldMenuMove(2);
        press(H::Action::confirm);
        Require(H::StaffPatrol::settings.width == 2, "Duplicate A changed patrol width twice");
        W::HandheldMenuMove(2);
        press(H::Action::confirm);
        press(H::Action::confirm);
        Require(H::StaffPatrol::Snapshot(employee).size() == 1, "Cancel-first cleared patrol");
        W::HandheldMenuMove(4);
        press(H::Action::confirm);
        W::HandheldMenuMove(-1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(H::StaffPatrol::Snapshot(employee).empty(), "SDL native patrol clear failed");
        press(H::Action::back);
        Require(H::StaffPatrol::Active(), "Patrol options B lost context");
        H::EndTerrain();
        Require(!H::StaffPatrol::Active(), "Patrol cleanup leaked");
        std::cout << "SDL patrol inspector / width-held-A / Cancel-first clear / options recovery passed" << std::endl;
        wm->CloseByClass(WindowClass::handheldMenu);
        W::HandheldStaffOpen(employee);
        FocusPersonAction(true, "Name...");
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().columns == 8, "Staff keyboard route failed");
        const auto oldName = S::Resolve(employee)->getName();
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().title == oldName + "A", "Held A typed twice");
        W::HandheldMenuMove(46);
        press(H::Action::confirm);
        Require(
            W::HandheldStaffPresentation().confirmation && W::HandheldStaffPresentation().selected == 1,
            "Name review not Cancel-first");
        press(H::Action::confirm);
        Require(
            S::Resolve(employee)->getName() == oldName && W::HandheldStaffPresentation().columns == 8,
            "Name cancellation applied action");
        W::HandheldMenuMove(46);
        press(H::Action::confirm);
        W::HandheldMenuMove(-1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(S::Resolve(employee)->getName() == oldName + "A", "SDL native staff name failed");
        wm->CloseByClass(WindowClass::handheldMenu);
        Require(objects.LoadObject("rct2.peep_animations.handyman"), "SDL handyman metadata missing");
        std::optional<S::Target> handyman;
        Require(
            S::Hire(
                StaffType::handyman, true,
                [&](const auto*, const auto* response) {
                    if (response->error == GameActions::Status::ok)
                        handyman = S::Identify(
                            response->template getData<GameActions::StaffHireNewActionResult>().StaffEntityId);
                }).error
                == GameActions::Status::ok,
            "SDL uniform employee hire failed");
        GameActions::ProcessQueue(state);
        Require(handyman.has_value(), "SDL uniform target absent");
        W::HandheldStaffOpen(*handyman);
        FocusPersonAction(true, "Uniform colour...");
        press(H::Action::confirm);
        const auto oldColour = StaffGetColour(StaffType::handyman);
        press(H::Action::confirm);
        Require(StaffGetColour(StaffType::handyman) == oldColour, "Uniform draft applied prematurely");
        W::HandheldMenuMove(1);
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Require(
            EnumValue(StaffGetColour(StaffType::handyman)) == (EnumValue(oldColour) + 1) % 72,
            "SDL uniform held-A/native apply failed");
        W::HandheldStaffOpen();
        W::HandheldMenuMove(1);
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().rows[1] == "Filter: Handyman", "SDL role filtering failed");
        W::HandheldMenuMove(2);
        press(H::Action::confirm);
        Require(W::HandheldStaffPresentation().stats.size() == 5, "SDL role wages/counts absent");
        wm->CloseByClass(WindowClass::handheldMenu);
        Require(S::Dismiss(*handyman, true).error == GameActions::Status::ok, "SDL uniform cleanup failed");
        GameActions::ProcessQueue(state);
        Require(S::Dismiss(employee, true).error == GameActions::Status::ok, "SDL staff cleanup failed");
        GameActions::ProcessQueue(state);
        state.park.entrances = oldEntrances;
        std::cout << "SDL staff roles / held-A handoff / Cancel-first hire / costume native action passed" << std::endl;
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/hud-lifecycle.park";
        Require(ScenarioSave(state, save, flags), "HUD fixture save failed");
        gOpenRCT2NoGraphics = true;
        Require(context->LoadParkFromFile(save), "HUD fixture reload failed");
        gOpenRCT2NoGraphics = false;
        wm->CloseByClass(WindowClass::progressWindow);
        InputSetState(InputState::normal);
        H::UpdateExplorer(input);
        Require(
            H::NativeHud().cash == cashLabel && state.park.cash == cash,
            "HUD/save mutated native accounting or cached stale cash");
        H::BeginTerrain(true);
        H::UpdateExplorer(input);
        context.reset(); // Exercise the confirmed pre-unload cleanup with UI/tool windows alive.
        std::cout << "Pocket SDL HUD windows / construction inset / Dashboard-Build yield-return / native save-reload / "
                     "shutdown passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
