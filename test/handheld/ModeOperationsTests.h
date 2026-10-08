// SPDX-License-Identifier: GPL-3.0-only
// Public native metadata/actions; only the already placed flat ride is saved as an operating ride.
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/localisation/Formatting.h>
static void TestModeOperations(
    IContext& context, const std::string& profile, Ui::Handheld::RideManagement::Target existing, bool realUi)
{
    namespace R = Ui::Handheld::RideManagement;
    auto& state = getGameState();
    auto caption = [](StringId id) {
        auto text = FmtString(FormatStringID(id)).WithoutFormatTokens();
        while (!text.empty() && (text.back() == ':' || text.back() == ' '))
            text.pop_back();
        return text;
    };
    auto parameter = [](const Ride& ride, GameActions::RideSetSetting key) -> std::optional<R::Setting> {
        const auto settings = R::Settings(ride);
        const auto found = std::find_if(settings.begin(), settings.end(), [key](const auto& s) { return s.key == key; });
        return found == settings.end() ? std::optional<R::Setting>{} : *found;
    };
    auto exercise = [&](R::Target target, GameActions::RideSetSetting key) {
        auto* ride = GetRide(target.id);
        const auto setting = parameter(*ride, key);
        Require(setting && setting->values.size() > 1, "Mode-specific parameter absent");
        const auto cash = state.park.cash;
        const auto ledger = state.park.expenditureTable;
        const auto before = key == GameActions::RideSetSetting::operation ? ride->operationOption : ride->numCircuits;
        auto draft = *R::BeginSetting(target, *setting);
        const auto review = R::Presentation(R::View::confirmation, 1, {}, target, draft);
        Require(review.rows[1] == "Cancel" && review.selected == 1, "Mode review not Cancel-first");
        Require(
            (key == GameActions::RideSetSetting::operation ? ride->operationOption : ride->numCircuits) == before,
            "Mode review/cancellation mutated state");
        for (int n = 0; n < 300; ++n)
            R::AdjustPrice(draft, 1);
        Require(draft.value == setting->values.back(), "Mode upper bound wrong");
        for (int n = 0; n < 300; ++n)
            R::AdjustPrice(draft, -1);
        Require(draft.value == setting->values.front(), "Mode lower bound wrong");
        draft = *R::BeginSetting(target, *setting);
        R::AdjustPrice(draft, before == setting->values.back() ? -1 : 1);
        Require(
            std::abs(int(draft.value) - before)
                == std::min<int>(
                    setting->step,
                    before == setting->values.back() ? before - setting->values.front() : setting->values.back() - before),
            "Mode native spinner increment wrong");
        GameActions::RideSetSettingAction native(target.id, key, draft.value);
        const auto q = GameActions::Query(&native, state);
        const auto pocket = R::Action(draft, false);
        Require(q.error == pocket.error && q.cost == pocket.cost && state.park.cash == cash, "Mode query differs/spends");
        auto bad = draft;
        bad.value = 255;
        Require(R::Action(bad, true).error != GameActions::Status::ok, "Out-of-bound mode value accepted");
        const auto paused = gGamePaused;
        gGamePaused = 1; // Restriction fixture: native setting actions explicitly allow paused changes.
        Require(R::Action(draft, true).error == GameActions::Status::ok, "Native paused mode parameter failed");
        GameActions::ProcessQueue(state);
        gGamePaused = paused;
        Require(
            (key == GameActions::RideSetSetting::operation ? ride->operationOption : ride->numCircuits) == draft.value
                && state.park.cash == cash && state.park.expenditureTable == ledger,
            "Mode setting/accounting wrong");
        Require(R::Action(draft, true).error != GameActions::Status::ok, "Repeated/stale mode draft accepted");
    };
    Require(
        parameter(*GetRide(existing.id), GameActions::RideSetSetting::operation)->name == caption(STR_NUMBER_OF_ROTATIONS),
        "Native rotating flat label wrong");
    const auto originalStatus = GetRide(existing.id)->status;
    exercise(existing, GameActions::RideSetSetting::operation);
    Require(GetRide(existing.id)->status == originalStatus, "Native operation change invented closure requirement");
    std::vector<R::Target> created;
    auto create = [&](const char* objectName, ride_type_t type) {
        auto* object = context.GetObjectManager().LoadObject(objectName);
        Require(object, "Public mode object unavailable");
        const auto index = context.GetObjectManager().GetLoadedObjectEntryIndex(object);
        ResearchInsertRideEntry(index, true);
        RideEntrySetInvented(index);
        RideTypeSetInvented(type);
        GameActions::RideCreateAction action(type, index, 0, 0, kObjectEntryIndexNull, RideInspection::every30Minutes);
        const auto r = GameActions::Execute(&action, state);
        Require(r.error == GameActions::Status::ok, "Native mode fixture creation failed");
        GameActions::ProcessQueue(state);
        const auto t = *R::Identify(r.getData<RideId>());
        created.push_back(t);
        return t;
    };
    const auto dodg = create("rct2.ride.dodg1", RIDE_TYPE_DODGEMS);
    auto dodgSetting = *parameter(*GetRide(dodg.id), GameActions::RideSetSetting::operation);
    Require(
        dodgSetting.step == 10
            && dodgSetting.labels.front()
                == FmtString(FormatStringID(STR_RIDE_TIME_ENTRY, uint16_t{ 20 })).WithoutFormatTokens(),
        "Native duration units/step wrong");
    const bool noMoney = state.park.flags.has(ParkFlag::noMoney);
    state.park.flags.set(ParkFlag::noMoney); // Native no-money restriction fixture, not a gameplay shortcut.
    exercise(dodg, GameActions::RideSetSetting::operation);
    state.park.flags.set(ParkFlag::noMoney, noMoney);
    const auto swing = create("rct2.ride.swsh1", RIDE_TYPE_SWINGING_SHIP);
    Require(
        parameter(*GetRide(swing.id), GameActions::RideSetSetting::operation)->name == caption(STR_NUMBER_OF_SWINGS),
        "Swing label wrong");
    exercise(swing, GameActions::RideSetSetting::operation);
    const auto ferris = create("rct2.ride.fwh1", RIDE_TYPE_FERRIS_WHEEL);
    exercise(ferris, GameActions::RideSetSetting::operation);
    GameActions::RideSetSettingAction backwards(
        ferris.id, GameActions::RideSetSetting::mode, static_cast<uint8_t>(RideMode::backwardRotation));
    Require(GameActions::Execute(&backwards, state).error == GameActions::Status::ok, "Native reverse rotation mode failed");
    GameActions::ProcessQueue(state);
    exercise(ferris, GameActions::RideSetSetting::operation);
    const auto kart = create("rct2.ride.kart1", RIDE_TYPE_GO_KARTS);
    Require(
        parameter(*GetRide(kart.id), GameActions::RideSetSetting::operation)->name == caption(STR_NUMBER_OF_LAPS),
        "Race lap label wrong");
    exercise(kart, GameActions::RideSetSetting::operation);
    auto stale = *R::BeginSetting(kart, *parameter(*GetRide(kart.id), GameActions::RideSetSetting::operation));
    R::AdjustPrice(stale, 1);
    GameActions::RideSetSettingAction continuous(
        kart.id, GameActions::RideSetSetting::mode, static_cast<uint8_t>(RideMode::continuousCircuit));
    Require(GameActions::Execute(&continuous, state).error == GameActions::Status::ok, "Native race mode transition failed");
    GameActions::ProcessQueue(state);
    Require(
        !parameter(*GetRide(kart.id), GameActions::RideSetSetting::operation)
            && R::Action(stale, true).error != GameActions::Status::ok,
        "Irrelevant/stale laps survive mode transition");
    const auto lift = create("rct2.ride.clift1", RIDE_TYPE_CHAIRLIFT);
    auto speed = *parameter(*GetRide(lift.id), GameActions::RideSetSetting::operation);
    Require(
        speed.labels.back() == FmtString(FormatStringID(STR_VELOCITY, uint16_t{ 9 })).WithoutFormatTokens(),
        "Native speed conversion wrong");
    exercise(lift, GameActions::RideSetSetting::operation);
    const auto coaster = create("rct2.ride.rckc", RIDE_TYPE_CORKSCREW_ROLLER_COASTER);
    Require(!parameter(*GetRide(coaster.id), GameActions::RideSetSetting::operation), "Continuous coaster has launch control");
    exercise(coaster, GameActions::RideSetSetting::numCircuits);
    GameActions::RideSetSettingAction launch(
        coaster.id, GameActions::RideSetSetting::mode, static_cast<uint8_t>(RideMode::poweredLaunch));
    Require(GameActions::Execute(&launch, state).error == GameActions::Status::ok, "Native launch mode failed");
    GameActions::ProcessQueue(state);
    const auto launchSetting = *parameter(*GetRide(coaster.id), GameActions::RideSetSetting::operation);
    Require(
        launchSetting.name == caption(STR_LAUNCH_SPEED)
            && launchSetting.labels.front() == FmtString(FormatStringID(STR_VELOCITY, uint16_t{ 22 })).WithoutFormatTokens()
            && !parameter(*GetRide(coaster.id), GameActions::RideSetSetting::numCircuits),
        "Launch units/circuit visibility wrong");
    exercise(coaster, GameActions::RideSetSetting::operation);
    // Native Test/Open remains authoritative for incomplete fixtures; it is never forced successful.
    for (auto status : { RideStatus::testing, RideStatus::open })
    {
        const auto draft = *R::Begin(coaster, status);
        GameActions::RideSetStatusAction native(coaster.id, status);
        Require(
            R::Action(draft, false).error == GameActions::Query(&native, state).error
                && R::Action(draft, false).error != GameActions::Status::ok,
            "Mode editor bypassed track prerequisites");
    }
    for (const auto& t : created)
    {
        auto remove = *R::BeginDemolition(t);
        Require(R::Action(remove, true).error == GameActions::Status::ok, "Native mode fixture cleanup failed");
        GameActions::ProcessQueue(state);
    }
    // The genuinely placed/connected flat ride can return through native Close/Open after its parameter change.
    auto close = *R::Begin(existing, RideStatus::closed);
    Require(R::Action(close, true).error == GameActions::Status::ok, "Parameter workflow native Close failed");
    GameActions::ProcessQueue(state);
    GameActions::RideSetSettingAction modeWhileOpen(
        existing.id, GameActions::RideSetSetting::mode, static_cast<uint8_t>(GetRide(existing.id)->mode));
    auto open = *R::Begin(existing, RideStatus::open);
    Require(
        R::Action(open, false).error == GameActions::Status::ok && R::Action(open, true).error == GameActions::Status::ok,
        "Connected native flat reopening failed");
    GameActions::ProcessQueue(state);
    Require(GameActions::Query(&modeWhileOpen, state).error != GameActions::Status::ok, "Native mode closure restriction lost");
    const auto value = GetRide(existing.id)->operationOption;
    const auto cash = state.park.cash;
    auto preLoad = *R::BeginSetting(existing, *parameter(*GetRide(existing.id), GameActions::RideSetSetting::operation));
    R::AdjustPrice(preLoad, 1);
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/mode-operations.park";
    Require(ScenarioSave(state, save, flags) && context.LoadParkFromFile(save), "Native operation save/reload failed");
    if (!realUi)
        R::InvalidateAll(); // Dummy manager deliberately omits native reset notifications.
    Require(
        GetRide(existing.id)->operationOption == value && state.park.cash == cash
            && R::Action(preLoad, true).error != GameActions::Status::ok,
        "Mode serialization/pre-load draft wrong");
    std::cout << "Native mode Operations / duration-rotations-swings-laps-speed-launch-circuits / bounds-pause-cancel-stale / "
                 "zero cash-ledger / Test-Open prerequisites / save-reload passed\n";
}
