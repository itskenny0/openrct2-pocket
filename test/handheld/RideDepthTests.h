// SPDX-License-Identifier: GPL-3.0-only
// Operates on the native flat ride constructed by FlatRideWorkflowTests; no direct gameplay mutation.
#include "ModeOperationsTests.h"
#include "VehicleDispatchTests.h"

#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/TextEntry.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideSetSettingAction.h>
static void TestRideDepth(
    IContext& context, const std::string& profile, Ui::Handheld::RideManagement::Target target, bool realUi)
{
    namespace R = Ui::Handheld::RideManagement;
    auto& state = getGameState();
    TestModeOperations(context, profile, target, realUi);
    target = *R::Identify(target.id);
    TestVehicleDispatch(context, profile, target.id, realUi);
    target = *R::Identify(target.id);
    // Music uses a genuinely placed flat ride and loaded public music objects.
    Require(context.GetObjectManager().LoadObject("rct2.music.organ"), "Public music metadata missing");
    auto music = R::MusicSettings(*GetRide(target.id));
    Require(!music.empty() && music.front().key == GameActions::RideSetSetting::music, "Supported flat music absent");
    auto musicDraft = *R::BeginSetting(target, music.front());
    const auto musicCash = state.park.cash;
    R::AdjustPrice(musicDraft, musicDraft.value ? -1 : 1);
    Require(
        R::Action(musicDraft, false).error == GameActions::Status::ok
            && GetRide(target.id)->flags.has(RideFlag::music) != bool(musicDraft.value) && state.park.cash == musicCash,
        "Music query mutated state");
    Require(R::Action(musicDraft, true).error == GameActions::Status::ok, "Music native toggle failed");
    GameActions::ProcessQueue(state);
    Require(
        GetRide(target.id)->flags.has(RideFlag::music) == bool(musicDraft.value) && state.park.cash == musicCash,
        "Music toggle/accounting differs");
    Require(R::Action(musicDraft, true).error != GameActions::Status::ok, "Repeated music accepted");
    auto invalidMusic = *R::BeginSetting(target, R::MusicSettings(*GetRide(target.id)).front());
    invalidMusic.value = 2;
    Require(R::Action(invalidMusic, true).error != GameActions::Status::ok, "Invalid toggle accepted");
    // The carousel requires explicit music listing; organ must be filtered until
    // the already-existing native operating-limit cheat policy permits it.
    const bool oldUnlock = state.cheats.unlockOperatingLimits;
    state.cheats.unlockOperatingLimits = true;
    music = R::MusicSettings(*GetRide(target.id));
    Require(music.size() > 1, "Loaded supported music style absent");
    if (music.size() > 1)
    {
        auto style = *R::BeginSetting(target, music[1]);
        auto alternative = std::find_if(
            style.setting->values.begin(), style.setting->values.end(), [&](auto value) { return value != music[1].current; });
        if (alternative != style.setting->values.end())
        {
            style.value = *alternative;
            Require(R::Action(style, false).error == GameActions::Status::ok, "Music style query failed");
            Require(R::Action(style, true).error == GameActions::Status::ok, "Native music style failed");
            GameActions::ProcessQueue(state);
            Require(GetRide(target.id)->music == style.value, "Style not applied");
        }
    }
    state.cheats.unlockOperatingLimits = oldUnlock;
    std::cout << "Native Music / toggle-style-query-cancel-repeat-bounds / zero cash / persistence fixture passed\n";
    Ui::Handheld::TextEntry keyboard{ "Café" };
    Require(keyboard.Length() == 4, "Keyboard UTF-8 length differs");
    keyboard.Press(43);
    Require(keyboard.text == "Caf", "Keyboard split UTF-8 deletion");
    keyboard.Press(44);
    keyboard.Press(4);
    Require(keyboard.text == "Cafe", "Keyboard lowercase failed");
    for (int n = 0; n < 40; ++n)
        keyboard.Press(0);
    Require(keyboard.Length() == 32, "Keyboard length unbounded");
    keyboard.Press(45);
    Require(
        keyboard.text.empty() && keyboard.Move(0, -1, -1) == 0 && keyboard.Move(47, 1, 1) == 47,
        "Keyboard clear/grid bounds differ");
    auto nameDraft = *R::BeginName(target);
    nameDraft.name = "Pocket Café";
    Require(
        R::Action(nameDraft, false).error == GameActions::Status::ok && GetRide(target.id)->getName() == nameDraft.baselineName,
        "Name query changed ride");
    Require(R::Action(nameDraft, true).error == GameActions::Status::ok, "Native name failed");
    GameActions::ProcessQueue(state);
    Require(
        GetRide(target.id)->getName() == nameDraft.name && R::Action(nameDraft, true).error != GameActions::Status::ok,
        "Name not applied/repeated");
    const auto savedName = GetRide(target.id)->getName();
    const auto savedMusic = GetRide(target.id)->music;
    const bool savedPlaying = GetRide(target.id)->flags.has(RideFlag::music);
    auto cash = state.park.cash;
    auto* ride = GetRide(target.id);
    Require(ride, "Ride depth native placement missing");
    auto closing = R::Begin(target, RideStatus::closed);
    Require(closing && R::Action(*closing, true).error == GameActions::Status::ok, "Depth close failed");
    GameActions::ProcessQueue(state);
    auto settings = R::Settings(*ride, true);
    Require(settings.size() == 1, "Flat maintenance availability wrong");
    auto draft = *R::BeginSetting(target, settings.front());
    R::AdjustPrice(draft, draft.value == 0 ? 1 : -1);
    GameActions::RideSetSettingAction native(target.id, draft.setting->key, draft.value);
    const auto nativeQuery = GameActions::Query(&native, state);
    const auto query = R::Action(draft, false);
    Require(
        query.error == nativeQuery.error && query.cost == nativeQuery.cost && state.park.cash == cash,
        "Inspection query differs or spends money");
    Require(R::Action(draft, true).error == GameActions::Status::ok, "Native inspection setting failed");
    GameActions::ProcessQueue(state);
    Require(
        static_cast<uint8_t>(ride->inspectionInterval) == draft.value && state.park.cash == cash,
        "Inspection setting/accounting wrong");
    Require(R::Action(draft, true).error != GameActions::Status::ok, "Repeated inspection draft accepted");
    draft = *R::BeginSetting(target, R::Settings(*ride, true).front());
    auto bad = draft;
    bad.value = 255;
    Require(R::Action(bad, true).error != GameActions::Status::ok, "Invalid setting accepted");
    for (int i = 0; i < 300; ++i)
        R::AdjustPrice(draft, 1);
    Require(draft.value == static_cast<uint8_t>(RideInspection::never), "Inspection upper bound wrong");
    for (int i = 0; i < 300; ++i)
        R::AdjustPrice(draft, -1);
    Require(draft.value == 0, "Inspection lower bound wrong");
    const auto operations = R::Settings(*ride);
    for (const auto& setting : operations)
    {
        Require(setting.key != GameActions::RideSetSetting::liftHillSpeed, "Flat ride has irrelevant lift setting");
        auto edit = *R::BeginSetting(target, setting);
        R::AdjustPrice(edit, edit.value == setting.values.back() ? -1 : 1);
        const auto q = R::Action(edit, false);
        if (q.error == GameActions::Status::ok)
        {
            Require(R::Action(edit, true).error == GameActions::Status::ok, "Native flat operation failed");
            GameActions::ProcessQueue(state);
            Require(state.park.cash == cash, "Operations charged money");
        }
    }
    const auto interval = GetRide(target.id)->inspectionInterval;
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/ride-depth.park";
    Require(ScenarioSave(state, save, flags) && context.LoadParkFromFile(save), "Ride depth save/reload failed");
    target = *R::Identify(target.id);
    Require(
        GetRide(target.id)->music == savedMusic && GetRide(target.id)->flags.has(RideFlag::music) == savedPlaying,
        "Music serialization differs");
    Require(GetRide(target.id)->inspectionInterval == interval && state.park.cash == cash, "Depth setting not persisted");
    auto renew = *R::BeginRefurbishment(target);
    const auto renewQuery = R::Action(renew, false);
    GameActions::RideDemolishAction nativeRenew(target.id, GameActions::RideModifyType::renew);
    const auto nativeRenewQuery = GameActions::Query(&nativeRenew, state);
    Require(
        renewQuery.error == nativeRenewQuery.error && renewQuery.cost == nativeRenewQuery.cost && state.park.cash == cash,
        "Refurbish query differs/spends");
    if (renewQuery.error == GameActions::Status::ok)
    {
        const auto beforeName = GetRide(target.id)->getName();
        const auto beforeMusic = GetRide(target.id)->music;
        Require(R::Action(renew, true).error == GameActions::Status::ok, "Native refurbishment failed");
        GameActions::ProcessQueue(state);
        Require(
            GetRide(target.id) && !GetRide(target.id)->flags.has(RideFlag::everBeenOpened) && GetRide(target.id)->getAge() == 0
                && GetRide(target.id)->getName() == beforeName && GetRide(target.id)->music == beforeMusic
                && state.park.cash == cash - renewQuery.cost,
            "Refurbishment renewal/accounting differs");
        Require(R::Action(renew, true).error != GameActions::Status::ok, "Repeated refurbishment accepted");
        target = *R::Identify(target.id);
        cash = state.park.cash;
    }
    std::cout << "Native Refurbishment / native-query-cancel-repeat / age-reliability-retained-ride / exact cost passed\n";
    draft = *R::BeginDemolition(target);
    auto pathCount = [&] {
        size_t count = 0;
        for (int y = 32; y < 31 * 32; y += 32)
            for (int x = 32; x < 31 * 32; x += 32)
                for (const auto* path : TileElementsView<PathElement>(CoordsXY{ x, y }))
                    count += !path->isGhost();
        return count;
    };
    const auto neighbouringPaths = pathCount();
    // Reversible restriction fixture; native Query remains authoritative.
    GetRide(target.id)->flags.set(RideFlag::indestructible);
    Require(
        R::Action(draft, true).error != GameActions::Status::ok && state.park.cash == cash,
        "Indestructible ride demolition accepted");
    GetRide(target.id)->flags.unset(RideFlag::indestructible);
    GameActions::RideDemolishAction demolish(target.id, GameActions::RideModifyType::demolish);
    Require(
        R::Action(draft, false).error == GameActions::Query(&demolish, state).error
            && draft.refund == RideGetRefundPrice(*GetRide(target.id)),
        "Demolition query/refund differs");
    const auto model = R::Presentation(R::View::confirmation, 1, {}, target, draft);
    Require(
        model.confirmation && model.selected == 1 && model.rows[1] == "Cancel" && model.title == GetRide(target.id)->getName(),
        "Destructive review does not identify ride/default Cancel");
    const auto paused = gGamePaused;
    gGamePaused = 1; // Reversible restriction fixture only; all ride changes use actions.
    Require(R::Action(draft, true).error != GameActions::Status::ok && R::Resolve(target), "Paused demolition accepted");
    gGamePaused = paused;
    const auto result = R::Action(draft, true);
    Require(result.error == GameActions::Status::ok, "Native demolition failed");
    GameActions::ProcessQueue(state);
    Require(!R::Resolve(target) && state.park.cash == cash - draft.refund, "Demolition refund/removal wrong");
    Require(R::Action(draft, true).error != GameActions::Status::ok, "Repeated demolition accepted");
    Require(pathCount() == neighbouringPaths, "Demolition removed neighbouring real paths");
    Require(ScenarioSave(state, save, flags) && context.LoadParkFromFile(save), "Demolished save/reload failed");
    Require(!GetRide(target.id) && state.park.cash == cash - draft.refund, "Demolition not persisted");
    // Second genuine native placement verifies no-money demolition does not invent a refund.
    state.park.flags.set(ParkFlag::noMoney);
    Require(Ui::Handheld::BeginFlatRide(target.object), "No-money flat entry failed");
    const CoordsXY noMoneyTile{ 640, 640 };
    Require(
        Ui::Handheld::FlatRides::Action(noMoneyTile, true).error == GameActions::Status::ok,
        "No-money native flat placement failed");
    GameActions::ProcessQueue(state);
    const auto freeTarget = Ui::Handheld::FlatRides::Target();
    Require(freeTarget.has_value(), "No-money placed ride identity missing");
    Ui::Handheld::EndTerrain();
    const auto freeCash = state.park.cash;
    const auto freeDraft = *R::BeginDemolition(*freeTarget);
    Require(
        R::Action(freeDraft, false).cost == 0 && R::Action(freeDraft, true).error == GameActions::Status::ok,
        "No-money native demolition failed");
    GameActions::ProcessQueue(state);
    Require(!R::Resolve(*freeTarget) && state.park.cash == freeCash, "No-money demolition charged/refunded money");
    state.park.flags.unset(ParkFlag::noMoney);
    std::cout << "Native ride depth / settings-bounds-stale / queried demolition-exact-cash / save-reload passed\n";
}
