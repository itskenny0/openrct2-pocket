// SPDX-License-Identifier: GPL-3.0-only
// Public animation metadata / native scenario fixture, no original sprites required.
#include <iostream>
#include <openrct2-ui/handheld/StaffManagement.h>
#include <openrct2-ui/handheld/StaffPatrol.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/peep/StaffHireNewAction.h>
#include <openrct2/actions/peep/StaffSetCostumeAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/management/Finance.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/scenario/Scenario.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace S = Ui::Handheld::StaffManagement;
static void Require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = gOpenRCT2NoGraphics = true;
        auto context = CreateContext();
        Config::SetDefaults();
        Require(context->Initialise(), "Staff fixture init failed");
        auto& state = getGameState();
        gameStateInitAll(state, { 32, 32 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 1000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        state.park.entrances.push_back({ 64, 64, 16, 0 }); // Initial scenario fixture placement source.
        auto& objects = context->GetObjectManager();
        for (const char* id :
             { "rct2.peep_animations.handyman", "rct2.peep_animations.mechanic", "rct2.peep_animations.security",
               "rct2.peep_animations.entertainer_elephant", "rct2.peep_animations.entertainer_tiger" })
            Require(objects.LoadObject(id), "Staff animation metadata missing");
        Require(S::Hire(StaffType::count, false).error != GameActions::Status::ok, "Invalid role accepted");
        const auto cash = state.park.cash;
        std::vector<S::Target> hired;
        for (auto role : { StaffType::handyman, StaffType::mechanic, StaffType::security, StaffType::entertainer })
        {
            const auto count = S::Roster().size();
            Require(
                S::Hire(role, false).error == GameActions::Status::ok && S::Roster().size() == count && state.park.cash == cash,
                "Role query changed entities/cash");
            std::optional<S::Target> target;
            Require(
                S::Hire(
                    role, true,
                    [&](const auto*, const auto* result) {
                        if (result->error == GameActions::Status::ok)
                            target = S::Identify(
                                result->template getData<GameActions::StaffHireNewActionResult>().StaffEntityId);
                    }).error
                    == GameActions::Status::ok,
                "Native role hire failed");
            GameActions::ProcessQueue(state);
            Require(target && S::Resolve(*target), "Native hire callback missing");
            hired.push_back(*target);
            Require(target->type == role && state.park.cash == cash, "Role identity or upfront charge differs");
            const auto inspect = S::Presentation(S::View::inspection, 0, {}, target);
            Require(inspect.stats[0].second == S::RoleName(role), "Native role label differs");
        }
        const auto entertainer = hired.back();
        const auto baseline = S::Resolve(entertainer)->animationObjectIndex;
        const auto costumes = S::Costumes();
        Require(costumes.size() >= 2, "Loaded costume filtering failed");
        auto next = costumes.front().first == baseline ? costumes.back().first : costumes.front().first;
        Require(
            S::SetCostume(entertainer, baseline, kObjectEntryIndexNull, true).error != GameActions::Status::ok,
            "Missing costume accepted");
        GameActions::StaffSetCostumeAction invalid(entertainer.id, kObjectEntryIndexNull);
        Require(GameActions::Query(&invalid, state).error != GameActions::Status::ok, "Native missing costume guard failed");
        Require(
            S::SetCostume(entertainer, baseline, next, false).error == GameActions::Status::ok
                && S::Resolve(entertainer)->animationObjectIndex == baseline,
            "Costume query mutated staff");
        Require(S::SetCostume(entertainer, baseline, next, true).error == GameActions::Status::ok, "Costume execute failed");
        GameActions::ProcessQueue(state);
        Require(
            S::Resolve(entertainer)->animationObjectIndex == next
                && S::SetCostume(entertainer, baseline, next, true).error != GameActions::Status::ok,
            "Native costume persistence/stale repeat failed");
        Require(
            S::SetCostume(hired.front(), S::Resolve(hired.front())->animationObjectIndex, next, false).error
                != GameActions::Status::ok,
            "Non-entertainer costume accepted");
        money64 wages = 0;
        for (const auto& t : hired)
            wages += GetStaffWage(t.type) / 4;
        FinancePayWages();
        Require(state.park.cash == cash - wages, "Native weekly wages differ");
        auto before = state.park.cash;
        state.park.flags.set(ParkFlag::noMoney);
        FinancePayWages();
        Require(state.park.cash == before, "No-money wages charged");
        state.park.flags.unset(ParkFlag::noMoney);
        namespace P = Ui::Handheld::StaffPatrol;
        Require(P::Begin(hired.front()), "Patrol employee selection failed");
        const CoordsXY tile{ 320, 320 };
        P::settings.width = 3;
        P::settings.height = 2;
        Require(P::Action(CoordsXY{ 32, 32 }, false).error != GameActions::Status::ok, "Patrol bounds clamped");
        Require(P::Action(CoordsXY{ 321, 320 }, false).error != GameActions::Status::ok, "Unaligned patrol accepted");
        Require(
            P::Action(tile, false).error == GameActions::Status::ok && P::Snapshot(hired.front()).empty(),
            "Patrol query mutated coverage");
        Require(
            P::Action(tile, true).error == GameActions::Status::ok && P::Action(tile, true).error != GameActions::Status::ok,
            "Patrol pending guard failed");
        GameActions::ProcessQueue(state);
        Require(
            P::Snapshot(hired.front()).size() == 6 && P::Snapshot(hired[1]).empty(),
            "Native patrol footprint/employee differs");
        auto draft = P::ReviewClear();
        Require(draft && P::Clear(*draft, false).error == GameActions::Status::ok, "Patrol clear review failed");
        P::settings = { 1, 1, true };
        Require(P::Action(tile, true).error == GameActions::Status::ok, "Native patrol remove failed");
        GameActions::ProcessQueue(state);
        Require(
            P::Snapshot(hired.front()).size() == 5 && P::Clear(*draft, true).error != GameActions::Status::ok,
            "Patrol removal/stale clear failed");
        draft = P::ReviewClear();
        Require(draft.has_value(), "Patrol clear draft missing");
        Require(P::Clear(*draft, true).error == GameActions::Status::ok, "Native patrol clear failed");
        GameActions::ProcessQueue(state);
        Require(
            P::Snapshot(hired.front()).empty() && P::Clear(*draft, true).error != GameActions::Status::ok,
            "Repeated patrol clear accepted");
        P::settings = { 2, 2, false };
        Require(P::Action(tile, true).error == GameActions::Status::ok, "Patrol save seed failed");
        GameActions::ProcessQueue(state);
        P::End();
        Require(!P::Active(), "Patrol cleanup retained target");
        const auto originalName = S::Resolve(hired.front())->getName();
        Require(
            S::SetName(hired.front(), originalName, "Mónica", false).error == GameActions::Status::ok
                && S::Resolve(hired.front())->getName() == originalName,
            "Name query mutated employee");
        Require(
            S::SetName(hired.front(), originalName, "Mónica", true).error == GameActions::Status::ok, "Name execute failed");
        GameActions::ProcessQueue(state);
        Require(
            S::Resolve(hired.front())->getName() == "Mónica"
                && S::SetName(hired.front(), originalName, "Other", true).error != GameActions::Status::ok,
            "Stale name accepted");
        Require(
            S::SetName(hired.front(), "Mónica", std::string(33, 'x'), false).error != GameActions::Status::ok,
            "Oversized name accepted");
        for (const auto& t : hired)
            Require(S::Roster(t.type).size() == 1, "Role filter differs");
        const auto colour = StaffGetColour(hired.front().type);
        const auto newColour = static_cast<Drawing::Colour>((EnumValue(colour) + 1) % 72);
        Require(
            S::SetUniform(hired.front(), colour, newColour, false).error == GameActions::Status::ok
                && StaffGetColour(hired.front().type) == colour,
            "Uniform query mutated state");
        Require(
            S::SetUniform(entertainer, colour, newColour, false).error != GameActions::Status::ok,
            "Entertainer uniform accepted");
        Require(
            S::SetUniform(hired.front(), colour, static_cast<Drawing::Colour>(255), false).error != GameActions::Status::ok,
            "Invalid uniform accepted");
        Require(
            S::SetUniform(hired.front(), colour, newColour, true).error == GameActions::Status::ok, "Uniform execute failed");
        GameActions::ProcessQueue(state);
        Require(
            S::Resolve(hired.front())->tShirtColour == newColour && S::Resolve(hired.front())->trousersColour == newColour
                && S::SetUniform(hired.front(), colour, newColour, true).error != GameActions::Status::ok
                && state.park.cash == before,
            "Uniform native application/stale/accounting failed");
        for (const auto& t : { hired[1], hired[2] })
        {
            const auto base = StaffGetColour(t.type);
            const auto value = static_cast<Drawing::Colour>((EnumValue(base) + 1) % 72);
            Require(S::SetUniform(t, base, value, true).error == GameActions::Status::ok, "Native role uniform failed");
            GameActions::ProcessQueue(state);
            Require(
                S::Resolve(t)->tShirtColour == value && S::Resolve(t)->trousersColour == value, "Native role clothing differs");
        }
        Require(S::SetName(hired.front(), "Mónica", "", true).error == GameActions::Status::ok, "Default name reset failed");
        GameActions::ProcessQueue(state);
        const auto defaultName = S::Resolve(hired.front())->getName();
        Require(
            !S::Resolve(hired.front())->name
                && S::SetName(hired.front(), defaultName, "", false).error != GameActions::Status::ok,
            "Repeated default name accepted");
        Require(S::SetName(hired.front(), defaultName, "Mónica", true).error == GameActions::Status::ok, "Name restore failed");
        GameActions::ProcessQueue(state);
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/staff-completion.park";
        Require(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Staff save/reload failed");
        Require(
            S::Roster().size() == 4 && S::Resolve(entertainer)->animationObjectIndex == next && state.park.cash == before,
            "Staff role/costume save differs");
        Require(
            S::Resolve(hired.front())->getName() == "Mónica" && StaffGetColour(hired.front().type) == newColour
                && P::Snapshot(hired.front()).size() == 4,
            "Name/uniform/patrol save differs");
        for (const auto& t : S::Roster())
        {
            Require(S::Dismiss(t, true).error == GameActions::Status::ok, "Native role dismissal failed");
            GameActions::ProcessQueue(state);
            Require(!S::Resolve(t), "Dismissed identity survived");
        }
        Require(S::Roster().empty() && state.park.cash == before, "Dismissal invented refund");
        std::cout << "Native all staff roles / loaded costumes / invalid-role-object / query-stale-repeat / wages-no-money / "
                     "patrol-name-uniform-filter-dismissal-save passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
