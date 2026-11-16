// SPDX-License-Identifier: GPL-3.0-only
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2/Input.h>
#include "RideTabNavigation.h"
#include <openrct2/interface/Window.h>
static void TestRideAccess(
    IContext& context, const std::string& profile, bool realUi, const Ui::Handheld::RideManagement::Target& target)
{
    namespace H = Ui::Handheld;
    namespace A = H::RideAccess;
    namespace P = H::Paths;
    namespace W = Ui::Windows;
    auto& state = getGameState();
    for (const auto* name :
         { "rct2.footpath_surface.tarmac", "rct2.footpath_surface.queue_blue", "rct2.footpath_railings.wood" })
        Require(context.GetObjectManager().LoadObject(name), "Access path fixture object missing");
    if (realUi)
    {
        context.GetUiContext().CreateWindow();
        auto* main = WindowGetMain();
        main->onResize();
        InputSetState(InputState::normal); // Native event loop normally leaves reset before gameplay input.
        context.GetUiContext().GetWindowManager()->CloseByClass(WindowClass::error);
    }
    if (realUi)
    {
        H::ControllerInput input;
        H::UpdateExplorer(input);
        Require(
            W::HandheldDashboardIsOpen() && W::HandheldRidesPresentation().selected == 0,
            "Completed ride did not return to modern management");
        W::HandheldMenuBack();
        Require(!W::HandheldDashboardIsOpen() && !P::IsActive(), "Completion handoff trapped construction");
    }
    H::EndTerrain();
    const auto access = A::Inspect(target);
    Require(access && !access->entrance.attached && !access->exit.attached, "Missing access incorrectly connected");
    const auto initialCash = state.park.cash;
    if (realUi)
    {
        W::HandheldRideAccessOpen(target);
        Require(W::HandheldRidesPresentation().selected == 0, "Optional access did not default to park");
        H::ControllerInput input;
        H::Frame frame;
        frame.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(input, frame);
        Require(
            !W::HandheldDashboardIsOpen() && !P::IsActive() && state.park.cash == initialCash,
            "Optional access could not cancel");
        W::HandheldRideAccessOpen(target);
        W::HandheldMenuMove(1);
        H::HandleMenuInput(input, frame);
        Require(P::IsActive() && P::IsQueue() && state.park.cash == initialCash, "Access A spilled into path placement");
        P::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
        Require(P::TargetTile() == CoordsXY(access->entrance.outside), "Entrance did not focus native path target");
        // Native pixel hit testing at every supported construction camera orientation/zoom.
        auto* main = WindowGetMain();
        for (Direction rotation : kAllDirections)
            for (int zoom = static_cast<int8_t>(ZoomLevel::min()); zoom <= static_cast<int8_t>(ZoomLevel::max()); ++zoom)
            {
                main->viewport->rotation = rotation;
                main->viewport->zoom = ZoomLevel{ static_cast<int8_t>(zoom) };
                Require(A::BeginPaths(target, false), "Rotated access handoff failed");
                P::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
                Require(P::TargetTile() == CoordsXY(access->entrance.outside), "Rotated/zoomed native center target changed");
            }
        main->viewport->rotation = 0;
        main->viewport->zoom = ZoomLevel{ 0 };
        Require(A::BeginPaths(target, false), "Access reset failed");
        P::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
        P::UpdatePreview();
        H::Frame pause;
        pause.actions = { H::Action::openMenu, H::Action::confirm };
        H::HandleExplorerInput(input, pause);
        Require(
            W::HandheldDashboardIsOpen() && P::IsActive() && !MapGetFootpathElement(access->entrance.outside),
            "Pause handoff retained ghost or acted twice");
        W::HandheldMenuBack();
        P::UpdatePreview();
        W::HandheldPathOptionsOpen();
        Require(!MapGetFootpathElement(access->entrance.outside), "Options retained access path ghost");
        W::HandheldMenuBack();
        H::Frame cancel;
        cancel.actions = { H::Action::back, H::Action::confirm };
        H::HandleExplorerInput(input, cancel);
        Require(
            W::HandheldDashboardIsOpen() && !P::IsActive() && W::HandheldRidesPresentation().selected == 0,
            "Guided path B did not recover access / leaked confirmation");
        W::HandheldMenuBack();
        W::HandheldRideOpen(target.id);
        FocusRideTabAction("Access / paths...");
        W::HandheldMenuConfirm();
        Require(W::HandheldRidesPresentation().rows.size() == 7, "Park ride inspection cannot recover access");
        for (int n = 0; n < 5; ++n)
            W::HandheldMenuMove(1);
        H::HandleMenuInput(input, frame);
        Require(
            P::IsConnected() && P::TargetTile() == CoordsXY(access->entrance.outside) && state.park.cash == initialCash
                && !MapGetFootpathElement(access->entrance.outside),
            "Connected access start placed without explicit intent or lost geometry");
        H::HandleExplorerInput(input, cancel);
        Require(W::HandheldDashboardIsOpen() && !P::IsActive(), "Connected access cancel lost menu recovery");
        for (int n = 0; n < 5 && W::HandheldDashboardIsOpen(); ++n)
            W::HandheldMenuBack();
        Require(!W::HandheldDashboardIsOpen() && gGamePaused == 0, "Access inspection did not return to unpaused park");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        Require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Access controller init failed");
        {
            H::ControllerInput controller;
            controller.update(true, 1000);
            const auto device = SDL_JoystickAttachVirtual(
                SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
            Require(device >= 0, "Access controller attach failed");
            auto* joystick = SDL_JoystickOpen(device);
            Require(joystick, "Access controller missing");
            auto pump = [&]() {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                    controller.handleEvent(event);
            };
            pump();
            controller.update(true, 1000);
            pump();
            controller.update(true, 1010);
            auto press = [&](SDL_GameControllerButton button, bool down) {
                Require(SDL_JoystickSetVirtualButton(joystick, button, down) == 0, "Access mapped button failed");
                SDL_JoystickUpdate();
                pump();
            };
            W::HandheldRideAccessOpen(target);
            W::HandheldMenuMove(1);
            press(SDL_CONTROLLER_BUTTON_A, true);
            auto selecting = controller.update(true, 1020);
            Require(selecting.actions.size() == 1, "Access mapped A missing");
            H::HandleExplorerInput(controller, selecting);
            Require(P::IsActive() && state.park.cash == initialCash, "Access mapped selecting A placed a path");
            Require(controller.update(true, 1030).actions.empty(), "Access held selecting A leaked");
            press(SDL_CONTROLLER_BUTTON_A, false);
            Require(controller.update(true, 1040).actions.empty(), "Access A release placed path");
            press(SDL_CONTROLLER_BUTTON_A, true);
            auto fresh = controller.update(true, 1050);
            Require(
                fresh.actions.size() == 1 && fresh.actions[0] == H::Action::confirm && !H::HandleMenuInput(controller, fresh),
                "Access fresh A not handed to Paths");
            press(SDL_CONTROLLER_BUTTON_A, false);
            controller.update(true, 1060);
            press(SDL_CONTROLLER_BUTTON_B, true);
            H::HandleExplorerInput(controller, controller.update(true, 1070));
            Require(!P::IsActive() && W::HandheldDashboardIsOpen(), "Mapped B did not resume access");
            Require(controller.update(true, 1080).actions.empty(), "Held B closed recovery page");
            press(SDL_CONTROLLER_BUTTON_B, false);
            controller.update(true, 1090);
            SDL_JoystickClose(joystick);
            SDL_JoystickDetachVirtual(device);
        }
        W::HandheldMenuConfirm();
        Require(!W::HandheldDashboardIsOpen() && state.park.cash == initialCash, "Mapped access cancellation changed park");
    }
    auto setTarget = [&](CoordsXY xy) {
        InteractionInfo info;
        info.Loc = xy;
        info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(xy));
        info.interactionType = ViewportInteractionItem::terrain;
        P::UpdateTargetFromInfo(info);
    };
    auto place = [&](CoordsXY xy, bool queue) {
        P::Begin(queue);
        setTarget(xy);
        const auto query = P::Action(false);
        const auto cash = state.park.cash;
        Require(query.error == GameActions::Status::ok, "Access path native query failed");
        P::UpdatePreview();
        Require(state.park.cash == cash, "Access path ghost spent money");
        P::ClearPreview();
        if (realUi)
        {
            H::ControllerInput input;
            H::Frame frame;
            frame.actions = { H::Action::confirm, H::Action::confirm };
            H::HandleExplorerInput(input, frame);
        }
        else
            Require(P::Action(true).error == GameActions::Status::ok, "Access path placement failed");
        GameActions::ProcessQueue(state);
        const auto expectedCharge = state.park.flags.has(ParkFlag::noMoney) ? 0 : query.cost;
        if (state.park.cash != cash - expectedCharge)
            std::cerr << "Accounting tile " << xy.x << "," << xy.y << " queried " << query.cost << " charged "
                      << cash - state.park.cash << " input " << int(InputGetState()) << "\n";
        Require(state.park.cash == cash - expectedCharge, "Access path did not charge exact native cost once");
        int realPaths = 0;
        for (auto* path : TileElementsView<PathElement>(xy))
            realPaths += !path->isGhost();
        Require(realPaths == 1, "Access A did not place exactly one path");
        P::End();
    };
    const auto q = access->entrance.outside;
    P::Begin(true);
    setTarget(q);
    P::UpdatePreview();
    Require(!A::Inspect(target)->entrance.attached, "Ghost reported as entrance connection");
    auto* surface = MapGetSurfaceElementAt(q);
    surface->setOwnership({});
    P::ClearPreview();
    Require(
        P::Action(true).error != GameActions::Status::ok && state.park.cash == initialCash, "Invalid access changed park cash");
    surface->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    Require(P::Action(true).error == GameActions::Status::insufficientFunds, "Access path funds rule bypassed");
    P::UpdatePreview();
    Require(!MapGetFootpathElement(q), "Unaffordable connection retained ghost");
    state.park.cash = initialCash;
    gGamePaused = 1;
    Require(P::Action(true).error != GameActions::Status::ok, "Paused access path placed");
    P::UpdatePreview();
    Require(!MapGetFootpathElement(q), "Paused access retained misleading ghost");
    gGamePaused = 0;
    P::End();
    place(q, true);
    auto info = A::Inspect(target);
    Require(info->entrance.attached && !info->entrance.reachesNormalPath, "Dangling queue reported joined to path");
    const auto* queue = MapGetFootpathElement(q);
    Require(
        queue && queue->getRideIndex() == target.id && queue->getStationIndex() == StationIndex::FromUnderlying(0),
        "Native queue association missing");
    const CoordsXY beyond = CoordsXY(q) - CoordsDirectionDelta[q.direction];
    place(beyond, false);
    Require(A::Inspect(target)->entrance.reachesNormalPath, "Native queue to normal connection not recognized");
    const auto exit = access->exit.outside;
    place(exit, true);
    Require(!A::Inspect(target)->exit.attached, "Exit queue accepted as normal access");
    place(exit, false);
    Require(A::Inspect(target)->exit.attached, "Exit normal access not recognized");
    if (realUi)
    {
        W::HandheldRideAccessOpen(target);
        for (int n = 0; n < 3; ++n)
            W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        Require(
            W::HandheldRidesPresentation().confirmation && W::HandheldRidesPresentation().selected == 1,
            "Access opening lost Cancel-first review");
        W::HandheldMenuConfirm();
        Require(GetRide(target.id)->status == RideStatus::closed, "Access Cancel opened ride");
        for (int n = 0; n < 5 && W::HandheldDashboardIsOpen(); ++n)
            W::HandheldMenuBack();
        const auto openCash = state.park.cash;
        W::HandheldRideAccessOpen(target);
        for (int n = 0; n < 3; ++n)
            W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(-1);
        H::ControllerInput confirming;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleExplorerInput(confirming, confirm);
        GameActions::ProcessQueue(state);
        Require(
            GetRide(target.id)->status == RideStatus::open && state.park.cash == openCash,
            "Access explicit opening failed / charged");
        for (int n = 0; n < 5 && W::HandheldDashboardIsOpen(); ++n)
            W::HandheldMenuBack();
        auto close = H::RideManagement::Begin(target, RideStatus::closed);
        Require(
            close && H::RideManagement::Action(*close, true).error == GameActions::Status::ok, "Access close fixture failed");
        GameActions::ProcessQueue(state);
        Require(A::BeginPaths(target, true) && !P::IsQueue(), "Exit handoff chose queue");
        P::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
        Require(P::TargetTile() == CoordsXY(exit), "Exit did not focus native target");
        H::EndTerrain();
    }
    const auto noMoneyCash = state.park.cash;
    state.park.flags.set(ParkFlag::noMoney);
    place({ 64, 64 }, false);
    Require(state.park.cash == noMoneyCash, "No-money connection deducted cash");
    state.park.flags.unset(ParkFlag::noMoney);
    P::Begin(false);
    const auto previewTile = CoordsXY(exit) - CoordsDirectionDelta[exit.direction];
    setTarget(previewTile);
    P::UpdatePreview();
    Require(
        MapGetFootpathElement(CoordsXYZ{ previewTile, MapGetSurfaceElementAt(previewTile)->getBaseZ() })
            && MapGetFootpathElement(CoordsXYZ{ previewTile, MapGetSurfaceElementAt(previewTile)->getBaseZ() })->isGhost(),
        "Access save test has no native ghost");
    const auto cash = state.park.cash;
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/ride-access.park";
    Require(ScenarioSave(state, save, flags) != 0, "Access preview save failed");
    // DummyUiContext has no playing-scene WindowManager teardown. Exercise the same
    // explicit tool exit used by the handheld load menu; real UI tests load directly.
    if (!realUi)
        H::EndTerrain();
    Require(context.LoadParkFromFile(save), "Access reload failed");
    for (auto* path : TileElementsView<PathElement>(previewTile))
        Require(!path->isGhost(), "Saved access preview became permanent");
    Require(!P::IsActive(), "Load retained guided path tool");
    const auto fresh = H::RideManagement::Identify(target.id);
    Require(
        fresh && A::Inspect(*fresh)->entrance.reachesNormalPath && A::Inspect(*fresh)->exit.attached && state.park.cash == cash,
        "Saved native access/accounting missing");
    if (realUi)
    {
        InputSetState(InputState::normal);
        Require(A::BeginPaths(*fresh, false), "Reload cannot recover access");
        P::UpdateTarget({ ContextGetWidth() / 2, ContextGetHeight() / 2 });
        P::UpdatePreview();
    }
    H::RideManagement::Invalidate(target.id);
    if (realUi)
    {
        H::ControllerInput input;
        H::UpdateExplorer(input);
        Require(!P::IsActive(), "Stale access retained tool/ghost");
    }
    Require(!A::Inspect(*fresh) && !A::BeginPaths(*fresh, false), "Stale/reused access identity accepted");
    if (realUi)
        context.GetUiContext().CloseWindow();
    std::cout << "Native ride access / optional-recovery-input-focus / queue association-connectivity / exact path cash / "
                 "opening-cancel / stale-save-reload passed\n";
}
