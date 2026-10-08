// SPDX-License-Identifier: GPL-3.0-only
// Real SDL/window lifecycle with public engine data; graphics-disabled native park fixture.
#include <SDL.h>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/RideManagement.h>
#include <openrct2-ui/interface/Window.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
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
        Require(!top(), "Build Back skipped its existing pause-menu level");
        W::HandheldMenuBack();
        H::UpdateExplorer(input);
        Require(top(), "Build cancellation lost HUD");
        H::EndTerrain();
        // Metadata-only fixture for type-specific management; no fabricated operating track or guests.
        namespace R = H::RideManagement;
        auto* object = context->GetObjectManager().LoadObject("rct2.ride.ptct1");
        Require(object, "Public wooden object metadata missing");
        auto* ride = RideAllocateAtIndex(RideId::FromUnderlying(0));
        Require(ride, "Ride UI fixture allocation failed");
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
        std::cout << "Native wooden mode / lift bounds / coupled waiting / no-money passed" << std::endl;
        W::HandheldDashboardOpen();
        W::HandheldDashboardMove(0, 1);
        press(H::Action::confirm);
        press(H::Action::confirm);
        auto model = W::HandheldRidesPresentation();
        W::HandheldDashboardMove(0, model.rows.size() - 4);
        press(H::Action::confirm);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Dispatch", "Dispatch route lost");
        press(H::Action::openBuild);
        Require(W::HandheldDashboardRailFocused(), "Dispatch Select failed");
        press(H::Action::openBuild);
        Require(W::HandheldRidesPresentation().breadcrumb == "Rides / Dispatch", "Dispatch resume lost");
        const auto dispatchBefore = ride->departFlags;
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
        W::HandheldDashboardMove(0, model.rows.size() - 5);
        press(H::Action::confirm);
        Require(
            W::HandheldRidesPresentation().breadcrumb == "Rides / Vehicles" && W::HandheldRidesPresentation().rows.size() == 1,
            "Trackless vehicle page fabricated editable counts");
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        W::HandheldDashboardMove(0, model.rows.size() - 3);
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
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        W::HandheldDashboardMove(0, model.rows.size() - 1);
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
        W::HandheldDashboardMove(0, model.rows.size() - 2);
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
        press(H::Action::back);
        model = W::HandheldRidesPresentation();
        W::HandheldDashboardMove(0, model.rows.size() - 1);
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
        W::HandheldDashboardMove(0, model.rows.size() - 1);
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
        W::HandheldDashboardMove(0, model.rows.size() - 1);
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
