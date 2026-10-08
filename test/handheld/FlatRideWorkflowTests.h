// SPDX-License-Identifier: GPL-3.0-only
// Fixture objects and research setup are confined to integration tests.
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
static void TestFlatRides(IContext& context, const std::string& profile, bool realUi)
{
    namespace H = Ui::Handheld;
    namespace F = H::FlatRides;
    namespace W = Ui::Windows;
    H::EndTerrain();
    if (realUi)
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
    auto& state = getGameState();
    gameStateInitAll(state, { 32, 32 });
    gLegacyScene = LegacyScene::playing;
    gGamePaused = 0;
    state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
    state.park.cash = 1000000;
    auto& objects = context.GetObjectManager();
    Require(objects.LoadObject("rct2.station.plain"), "Flat ride station fixture missing");
    for (const auto* name : { "mgr1", "twist1", "twist2", "swsh1", "swsh2", "dodg1", "fwh1", "simpod" })
    {
        auto* object = objects.LoadObject(std::string("rct2.ride.") + name);
        Require(object, "Flat ride object fixture missing");
        const auto i = objects.GetLoadedObjectEntryIndex(object);
        ResearchInsertRideEntry(i, true);
        RideEntrySetInvented(i);
        RideTypeSetInvented(FlatRidePlacement::Type(i));
    }
    const auto entries = F::Entries();
    Require(entries.size() > H::ObjectPicker::pageSize, "Flat ride paging fixture missing");
    for (const auto& e : entries)
        Require(
            F::Eligible(e.identity.index)
                && !GetRideTypeDescriptor(FlatRidePlacement::Type(e.identity.index)).flags.has(RtdFlag::hasTrack),
            "Unsupported flat ride picker entry");
    H::ObjectPicker picker;
    picker.setEntries(entries);
    picker.move(0, 1);
    picker.move(0, 1);
    Require(picker.page() == 1, "Flat ride picker paging failed");
    const auto index = objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.ride.mgr1"));
    Require(F::Eligible(index), "Merry-go-round unavailable");
    SetEveryRideEntryNotInvented();
    Require(F::Entries().empty() && !F::Begin(index), "Unresearched flat ride allowed");
    for (const auto& e : entries)
        RideEntrySetInvented(e.identity.index);
    SetEveryRideTypeNotInvented();
    Require(F::Entries().empty(), "Unresearched flat type allowed");
    for (const auto& e : entries)
        RideTypeSetInvented(FlatRidePlacement::Type(e.identity.index));
    for (int y = 32; y < 31 * 32; y += 32)
        for (int x = 32; x < 31 * 32; x += 32)
            MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
    const CoordsXY tile{ 320, 320 };
    auto tracks = [&](bool isGhost) {
        int count = 0;
        for (int y = 32; y < 31 * 32; y += 32)
            for (int x = 32; x < 31 * 32; x += 32)
                for (auto* e : TileElementsView<TrackElement>(CoordsXY{ x, y }))
                    count += e->isGhost() == isGhost;
        return count;
    };
    const auto cash = state.park.cash;
    // Exercise every eligible fixture footprint and all four orientations through the shared native ghost owner.
    for (const auto& entry : entries)
    {
        Require(H::BeginFlatRide(entry.identity.index), "Eligible flat fixture cannot begin");
        for (int rotation = 0; rotation < 4; ++rotation)
        {
            Require(F::Action(tile, false).error == GameActions::Status::ok, "Eligible flat footprint query failed");
            F::UpdatePreview(tile);
            Require(tracks(true) > 0 && tracks(false) == 0 && state.park.cash == cash, "Flat fixture ghost/cash failed");
            F::Rotate();
            Require(tracks(true) == 0, "Rotating flat fixture retained its old ghost");
        }
        H::EndTerrain();
        Require(tracks(true) == 0, "Flat fixture exit retained ghost");
    }
    Require(H::BeginFlatRide(index), "Flat ride begin failed");
    auto query = F::Action(tile, false);
    Require(query.error == GameActions::Status::ok && query.cost > 0, "Flat ride native query failed");
    F::UpdatePreview(tile);
    Require(tracks(true) == 9 && tracks(false) == 0 && state.park.cash == cash, "Flat footprint preview/cash failed");
    auto* first = *TileElementsView<TrackElement>(tile).begin();
    first->setColourScheme(static_cast<RideColourScheme>(1));
    F::UpdatePreview(tile);
    Require((*TileElementsView<TrackElement>(tile).begin())->getColourScheme() == 1, "Unchanged flat ghost rebuilt");
    F::Rotate();
    F::UpdatePreview(tile);
    Require(
        tracks(true) == 9 && (*TileElementsView<TrackElement>(tile).begin())->getDirection() == 1, "Flat ride rotation failed");
    F::UpdatePreview(CoordsXY{ 384, 320 });
    Require(tracks(true) == 9, "Flat ghost target replacement duplicated");
    Ui::ProvisionalElementsRemove();
    Ui::ProvisionalElementsRemove();
    Require(tracks(true) == 0, "Flat suspension retained ghost");
    Ui::ProvisionalElementsRestore();
    Require(tracks(true) == 0, "Flat nested suspension restored early");
    Ui::ProvisionalElementsRestore();
    Require(tracks(true) == 9, "Flat preview restoration failed");
    F::UpdatePreview({});
    Require(tracks(true) == 0, "Invalid flat target retained ghost");
    F::UpdatePreview(tile);
    MapGetSurfaceElementAt(tile)->setOwnership({});
    F::UpdatePreview(tile);
    Require(tracks(true) == 0 && F::Action(tile, true).error != GameActions::Status::ok, "Flat ownership bypass");
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    F::UpdatePreview(tile);
    Require(tracks(true) == 0 && F::Action(tile, true).error == GameActions::Status::insufficientFunds, "Flat funds bypass");
    state.park.cash = cash;
    gGamePaused = 1;
    F::UpdatePreview(tile);
    Require(tracks(true) == 0 && F::Action(tile, true).error != GameActions::Status::ok, "Paused flat placement bypass");
    gGamePaused = 0;
    if (realUi)
    {
        F::UpdatePreview(tile);
        W::HandheldBuildOpen();
        Require(tracks(true) == 0, "Build retained flat ghost");
        for (int n = 0; n < 8; ++n)
            W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        Require(W::HandheldObjectPickerIsOpen(), "Build Rides picker missing");
        const auto selection = F::Selection();
        W::HandheldObjectPickerMove(1, 0);
        W::HandheldMenuBack();
        Require(
            F::Selection() == selection && Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu) != nullptr,
            "Flat picker cancellation changed selection");
        W::HandheldMenuBack();
        W::HandheldRidePickerOpen();
        H::ControllerInput controller;
        H::Frame frame;
        frame.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(controller, frame);
        Require(
            !W::HandheldObjectPickerIsOpen() && tracks(false) == 0 && state.park.cash == cash,
            "Flat picker selecting A placed ride");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Require(device >= 0, "Flat virtual controller failed");
        auto* joystick = SDL_JoystickOpen(device);
        auto pump = [&] {
            SDL_Event event;
            while (SDL_PollEvent(&event))
                controller.handleEvent(event);
        };
        pump();
        controller.update(true, 1000);
        pump();
        controller.update(true, 1010);
        auto press = [&](bool down) {
            SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, down);
            SDL_JoystickUpdate();
            pump();
        };
        W::HandheldRidePickerOpen();
        press(true);
        auto selecting = controller.update(true, 1020);
        Require(selecting.actions.size() == 1 && H::HandleMenuInput(controller, selecting), "Flat picker mapped A lost");
        Require(controller.update(true, 1030).actions.empty(), "Flat held A leaked into placement");
        press(false);
        Require(controller.update(true, 1040).actions.empty(), "Flat release A placed");
        press(true);
        Require(controller.update(true, 1050).actions.size() == 1, "Flat fresh A lost");
        press(false);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);
        Require(H::BeginFlatRide(index), "Restore test flat selection failed");
        W::HandheldRidePickerOpen();
        W::HandheldObjectPickerPause();
        W::HandheldMenuBack();
        Require(F::CurrentStage() == F::Stage::ride && state.park.cash == cash, "Flat pause/cancel lost selection or charged");
    }
    F::UpdatePreview(tile);
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/flat-rides.park";
    // Exercise nested native save suspension, including the engine-only context.
    Ui::ProvisionalElementsRemove();
    Require(ScenarioSave(state, save, flags) != 0, "Flat preview save failed");
    Ui::ProvisionalElementsRestore();
    F::ClearPreview();
    Require(context.LoadParkFromFile(save), "Flat preview reload failed");
    Require(tracks(true) == 0 && tracks(false) == 0 && state.park.cash == cash, "Flat preview leaked into save");
    gGamePaused = 0;
    Require(H::BeginFlatRide(index), "Flat tool after reload failed");
    F::Rotate();
    query = F::Action(tile, false);
    Require(F::Action(tile, true).error == GameActions::Status::ok, "Flat placement rejected");
    Require(F::Action(tile, true).error != GameActions::Status::ok, "Duplicate/in-flight flat placement accepted");
    GameActions::ProcessQueue(state);
    Require(
        F::CurrentStage() == F::Stage::entrance && tracks(false) == 9 && tracks(true) == 0
            && state.park.cash == cash - query.cost,
        "Flat one-time placement/cash/entrance handoff failed");
    const auto identity = F::Target();
    Require(identity.has_value(), "Flat ride identity missing");
    auto* ride = GetRide(identity->id);
    Require(ride && ride->status == RideStatus::closed, "New flat ride not safely closed");
    std::vector<CoordsXY> locations;
    for (int y = tile.y - 160; y <= tile.y + 160; y += 32)
        for (int x = tile.x - 160; x <= tile.x + 160; x += 32)
            if (F::EntranceLocation(CoordsXY{ x, y }) && F::Action(CoordsXY{ x, y }, false).error == GameActions::Status::ok)
                locations.push_back({ x, y });
    Require(locations.size() >= 2, "Native flat station sides unavailable");
    Require(F::Action(CoordsXY{ 32, 32 }, true).error != GameActions::Status::ok, "Disconnected entrance allowed");
    auto nativeLocation = F::EntranceLocation(locations[0]);
    GameActions::FlatRideEntrancePlaceAction stale(identity->id, *nativeLocation, false);
    const auto start = ride->getStation(StationIndex::FromUnderlying(0)).start;
    ride->getStation(StationIndex::FromUnderlying(0)).start.x += 32;
    Require(GameActions::Query(&stale, state).error != GameActions::Status::ok, "Stale station accepted");
    ride->getStation(StationIndex::FromUnderlying(0)).start = start;
    gGamePaused = 1;
    Require(F::Action(locations[0], true).error != GameActions::Status::ok, "Paused entrance accepted");
    gGamePaused = 0;
    const auto entranceCash = state.park.cash;
    const auto entranceCost = F::Action(locations[0], false).cost;
    Require(F::Action(locations[0], true).error == GameActions::Status::ok, "Entrance placement failed");
    GameActions::ProcessQueue(state);
    Require(
        F::CurrentStage() == F::Stage::exit && state.park.cash == entranceCash - entranceCost, "Entrance stage/cash failed");
    Require(F::Action(locations[0], true).error != GameActions::Status::ok, "Exit allowed over entrance");
    H::EndTerrain();
    Require(F::CanResume(*identity) && H::ResumeFlatRide(identity->id), "Incomplete ride cannot resume");
    Require(F::CurrentStage() == F::Stage::exit, "Resume repeated entrance stage");
    const auto exitCash = state.park.cash;
    const auto exitCost = F::Action(locations[1], false).cost;
    Require(F::Action(locations[1], true).error == GameActions::Status::ok, "Exit placement failed");
    GameActions::ProcessQueue(state);
    Require(F::CurrentStage() == F::Stage::inactive && state.park.cash == exitCash - exitCost, "Exit stage/cash failed");
    Require(
        !ride->getStation(StationIndex::FromUnderlying(0)).entrance.isNull()
            && !ride->getStation(StationIndex::FromUnderlying(0)).exit.isNull(),
        "Flat entrance/exit missing");
    Require(F::Action(locations[1], true).error != GameActions::Status::ok, "Completed exit duplicated");
    const auto roster = H::RideManagement::Roster();
    Require(std::find(roster.begin(), roster.end(), *identity) != roster.end(), "New flat ride absent from native roster");
    Require(H::RideManagement::Location(*identity).has_value(), "New flat ride locate unavailable");
    auto status = H::RideManagement::Begin(*identity, RideStatus::open);
    Require(
        status && H::RideManagement::Action(*status, false).error == GameActions::Status::ok,
        "Complete flat ride cannot natively open");
    Require(H::RideManagement::Action(*status, true).error == GameActions::Status::ok, "Flat opening failed");
    GameActions::ProcessQueue(state);
    const auto finalCash = state.park.cash;
    H::EndTerrain();
    Require(ScenarioSave(state, save, flags) != 0 && context.LoadParkFromFile(save), "Complete flat save/reload failed");
    Require(
        state.park.cash == finalCash && GetRide(identity->id) && GetRide(identity->id)->status == RideStatus::open
            && tracks(false) == 9 && tracks(true) == 0,
        "Complete flat ride/accounting not persistent");
    const auto fresh = H::RideManagement::Identify(identity->id);
    Require(fresh.has_value(), "Reloaded flat identity missing");
    H::RideManagement::Invalidate(identity->id);
    Require(!F::Resume(*fresh), "Stale/reused flat identity accepted");
    if (realUi)
    {
        W::HandheldDashboardOpen();
        W::HandheldDashboardMove(0, 1);
        W::HandheldMenuConfirm();
        Require(
            W::HandheldRidesIsOpen() && W::HandheldRidesPresentation().rows.size() == 2,
            "Reloaded flat Dashboard roster failed");
        W::HandheldMenuBack();
        W::HandheldMenuBack();
    }
    std::cout << "Native flat rides / eligibility-picker / footprint-rotation-ghost / exact cash / entrance-exit-resume / "
                 "native-open-roster / input-save-reload passed\n";
}
