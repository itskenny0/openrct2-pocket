// SPDX-License-Identifier: GPL-3.0-only
// Native station/actions with public object metadata; no operating coaster is fabricated.
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/FlatRides.h>
static void TestVehicleDispatch(IContext& context, const std::string& profile, RideId flat, bool realUi)
{
    namespace R = Ui::Handheld::RideManagement;
    namespace C = Ui::Handheld::CustomTracks;
    auto& state = getGameState();
    Require(R::VehicleSettings(*GetRide(flat)).empty(), "Fixed flat vehicles exposed editable counts");
    namespace F = Ui::Handheld::FlatRides;
    const auto dodgIndex = context.GetObjectManager().GetLoadedObjectEntryIndex(
        context.GetObjectManager().LoadObject("rct2.ride.dodg1"));
    Require(F::Begin(dodgIndex), "Native dodgems creation failed");
    GameActions::ProcessQueue(state);
    Require(F::Action(CoordsXY{ 128, 640 }, true).error == GameActions::Status::ok, "Native dodgems placement failed");
    GameActions::ProcessQueue(state);
    const auto dodgTarget = *F::Target();
    F::End();
    const auto dodgSettings = R::VehicleSettings(*GetRide(dodgTarget.id));
    Require(
        dodgSettings.size() == 1 && dodgSettings[0].vehicle == GameActions::RideSetVehicleType::numTrains
            && dodgSettings[0].values.back() == 12,
        "Native flat vehicle count bounds wrong");
    const bool noMoney = state.park.flags.has(ParkFlag::noMoney);
    state.park.flags.set(ParkFlag::noMoney); // Restriction fixture; native count action owns behavior.
    auto countDraft = *R::BeginSetting(dodgTarget, dodgSettings[0]);
    const auto flatCash = state.park.cash;
    R::AdjustPrice(countDraft, countDraft.value == 12 ? -1 : 1);
    Require(R::Action(countDraft, true).error == GameActions::Status::ok, "Native flat count action failed");
    GameActions::ProcessQueue(state);
    Require(
        GetRide(dodgTarget.id)->numTrains == countDraft.value && state.park.cash == flatCash,
        "Native flat vehicle count/accounting differs");
    state.park.flags.set(ParkFlag::noMoney, noMoney);
    auto* object = context.GetObjectManager().LoadObject("rct2.ride.ptct1");
    Require(object, "Public wooden vehicle metadata missing");
    const auto index = context.GetObjectManager().GetLoadedObjectEntryIndex(object);
    ResearchInsertRideEntry(index, true);
    RideEntrySetInvented(index);
    RideTypeSetInvented(RIDE_TYPE_WOODEN_ROLLER_COASTER);
    Require(C::Begin(index), "Native station fixture creation failed");
    GameActions::ProcessQueue(state);
    C::Update(CoordsXY{ 704, 704 });
    Require(C::Action(false).error == GameActions::Status::ok, "Native station fixture query failed");
    Require(C::Action(true).error == GameActions::Status::ok, "Native station fixture placement failed");
    GameActions::ProcessQueue(state);
    const auto target = *C::Target();
    C::End();
    auto* ride = GetRide(target.id);
    Require(ride && ride->numStations == 1, "Native station absent");
    GameActions::RideSetVehicleAction minimum(
        target.id, GameActions::RideSetVehicleType::numCarsPerTrain, ride->getRideEntry()->min_cars_in_train);
    Require(GameActions::Execute(&minimum, state).error == GameActions::Status::ok, "Native initial cars failed");
    GameActions::ProcessQueue(state);
    const auto cash = state.park.cash;
    const auto settings = R::VehicleSettings(*ride);
    const auto cars = std::find_if(settings.begin(), settings.end(), [](const auto& s) {
        return s.vehicle == GameActions::RideSetVehicleType::numCarsPerTrain;
    });
    Require(cars != settings.end(), "Coaster cars setting missing");
    auto draft = *R::BeginSetting(target, *cars);
    for (int n = 0; n < 300; ++n)
        R::AdjustPrice(draft, 1);
    Require(draft.value == cars->values.back(), "Native object upper bound wrong");
    Require(draft.effectiveVehicles[1] <= draft.value, "Station clamping not shown honestly");
    Require(ride->proposedNumCarsPerTrain != draft.value && state.park.cash == cash, "Draft mutated ride/cash");
    GameActions::RideSetVehicleAction native(target.id, *cars->vehicle, draft.value);
    const auto q = GameActions::Query(&native, state);
    const auto pocket = R::Action(draft, false);
    Require(q.error == pocket.error && q.cost == pocket.cost, "Vehicle query differs from native");
    auto invalid = draft;
    invalid.value = 0;
    Require(R::Action(invalid, true).error != GameActions::Status::ok, "Invalid vehicle draft accepted");
    const auto paused = gGamePaused;
    gGamePaused = 1; // Restriction fixture: native vehicle action explicitly allows pause.
    Require(R::Action(draft, true).error == GameActions::Status::ok, "Paused native vehicle action rejected");
    GameActions::ProcessQueue(state);
    gGamePaused = paused;
    Require(
        ride->proposedNumCarsPerTrain == draft.value && ride->numCarsPerTrain == draft.effectiveVehicles[1]
            && ride->numTrains == draft.effectiveVehicles[0] && state.park.cash == cash,
        "Native clamping/accounting differs");
    Require(R::Action(draft, true).error != GameActions::Status::ok, "Repeated vehicle draft accepted");
    draft = *R::BeginSetting(target, R::VehicleSettings(*ride).back());
    for (int n = 0; n < 300; ++n)
        R::AdjustPrice(draft, -1);
    Require(draft.value == draft.setting->values.front(), "Vehicle lower bound wrong");
    // Dispatch updates preserve every unrelated flag, and cancellation never executes an action.
    const auto beforeFlags = ride->departFlags;
    const auto dispatch = R::DispatchSettings(*ride);
    Require(dispatch.size() >= 3, "Native coaster dispatch options missing");
    for (const auto& setting : dispatch)
    {
        auto edit = *R::BeginSetting(target, setting);
        R::AdjustPrice(edit, edit.value == setting.values.back() ? -1 : 1);
        const auto review = R::Presentation(R::View::confirmation, 1, {}, target, edit);
        Require(review.selected == 1 && review.rows[1] == "Cancel", "Dispatch confirmation not Cancel-first");
        Require(ride->departFlags == beforeFlags && state.park.cash == cash, "Review/cancel mutated native state");
    }
    auto edit = *R::BeginSetting(target, dispatch[0]);
    R::AdjustPrice(edit, edit.value == edit.setting->values.back() ? -1 : 1);
    GameActions::RideSetSettingAction departure(target.id, GameActions::RideSetSetting::departure, edit.value);
    Require(R::Action(edit, false).error == GameActions::Query(&departure, state).error, "Departure query differs");
    Require(R::Action(edit, true).error == GameActions::Status::ok, "Native dispatch execution failed");
    GameActions::ProcessQueue(state);
    Require(
        ride->departFlags == edit.value
            && (ride->departFlags & ~RIDE_DEPART_WAIT_FOR_LOAD) == (beforeFlags & ~RIDE_DEPART_WAIT_FOR_LOAD)
            && state.park.cash == cash,
        "Dispatch damaged other flags/cash");
    Require(R::Action(edit, true).error != GameActions::Status::ok, "Stale dispatch draft accepted");
    for (auto status : { RideStatus::testing, RideStatus::open })
    {
        auto statusDraft = *R::Begin(target, status);
        GameActions::RideSetStatusAction action(target.id, status);
        Require(
            R::Action(statusDraft, false).error == GameActions::Query(&action, state).error
                && R::Action(statusDraft, false).error != GameActions::Status::ok,
            "Incomplete coaster bypassed native Test/Open");
    }
    const auto proposed = ride->proposedNumCarsPerTrain;
    const auto effective = ride->numCarsPerTrain;
    const auto flagsValue = ride->departFlags;
    auto stale = *R::BeginSetting(target, R::DispatchSettings(*ride).front());
    R::AdjustPrice(stale, stale.value == stale.setting->values.back() ? -1 : 1);
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/vehicles-dispatch.park";
    Require(ScenarioSave(state, save, flags) && context.LoadParkFromFile(save), "Vehicle/dispatch save/reload failed");
    ride = GetRide(target.id);
    Require(
        ride && ride->proposedNumCarsPerTrain == proposed && ride->numCarsPerTrain == effective
            && ride->departFlags == flagsValue && state.park.cash == cash,
        "Native configuration serialization differs");
    if (!realUi)
        R::InvalidateAll(); // DummyWindowManager deliberately omits WindowInitAll UI callbacks.
    Require(R::Action(stale, true).error != GameActions::Status::ok, "Pre-load vehicle identity accepted");
    const auto fresh = *R::Identify(target.id);
    const auto remove = *R::BeginDemolition(fresh);
    Require(R::Action(remove, true).error == GameActions::Status::ok, "Station fixture cleanup failed");
    GameActions::ProcessQueue(state);
    const auto dodgFresh = *R::Identify(dodgTarget.id);
    const auto dodgRemove = *R::BeginDemolition(dodgFresh);
    Require(R::Action(dodgRemove, true).error == GameActions::Status::ok, "Dodgems fixture cleanup failed");
    GameActions::ProcessQueue(state);
    std::cout << "Native Vehicles/Dispatch / station-object bounds-clamping / flags-cancel-stale / pause-zero-cash / Test-Open "
                 "rejection / save-reload passed\n";
}
