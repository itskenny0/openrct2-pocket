// SPDX-License-Identifier: GPL-3.0-only
// Native scenario fixture, all operations use GameActions.
#include <iostream>
#include <openrct2-ui/handheld/GuestManagement.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/config/Config.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/scenario/Scenario.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace G = Ui::Handheld::GuestManagement;
static void Require(bool value, const char* text)
{
    if (!value)
        throw std::runtime_error(text);
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
        Require(context->Initialise(), "Guest fixture init failed");
        auto& state = getGameState();
        gameStateInitAll(state, { 32, 32 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 1000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        Require(context->GetObjectManager().LoadObject("rct2.peep_animations.guest"), "Guest metadata unavailable");
        // Initial scenario fixture: two guests inside the park, one initial native thought.
        for (int i = 0; i < 2; ++i)
        {
            auto* guest = Guest::generate({ 320 + i * 32, 320, 16 });
            Require(guest, "Fixture guest unavailable");
            guest->outsideOfPark = false;
            guest->setParkEntryTime(state.currentTicks);
            guest->peepFlags.set(PeepFlag::hasPaidForParkEntry);
            if (i == 0)
                guest->thoughts[0] = { PeepThoughtType::hungry, { .item = kPeepThoughtItemNone }, 1, 0 };
        }
        const auto target = G::Roster().front();
        const auto original = G::Resolve(target)->getName();
        const auto cash = state.park.cash;
        const auto ledger = state.park.expenditureTable;
        Require(
            G::SetName(target, original, "Mónica", false).error == GameActions::Status::ok
                && G::Resolve(target)->getName() == original,
            "Guest name query mutated state");
        Require(
            G::SetName(target, original, std::string(33, 'x'), false).error != GameActions::Status::ok,
            "Long guest name accepted");
        Require(G::SetName(target, original, "Mónica", true).error == GameActions::Status::ok, "Native guest name failed");
        GameActions::ProcessQueue(state);
        Require(
            G::Resolve(target)->getName() == "Mónica"
                && G::SetName(target, original, "Stale", true).error != GameActions::Status::ok,
            "Stale guest name accepted");
        const auto flags = G::Resolve(target)->peepFlags;
        Require(
            G::SetTracking(target, flags, true, false).error == GameActions::Status::ok && G::Roster({ {}, true }).empty(),
            "Tracking query mutated state");
        Require(G::SetTracking(target, flags, true, true).error == GameActions::Status::ok, "Native tracking failed");
        GameActions::ProcessQueue(state);
        Require(
            G::Roster({ {}, true }).size() == 1 && G::Resolve(target)->peepFlags.has(PeepFlag::hasPaidForParkEntry)
                && G::SetTracking(target, flags, true, true).error != GameActions::Status::ok,
            "Tracking flags/stale/filter differ");
        const auto groups = G::Groups(G::GroupMode::thoughts);
        Require(
            !groups.empty() && G::Roster({ groups.front(), false }).size() == groups.front().count,
            "Native thought membership differs");
        Require(!G::Presentation(G::View::preferences, 0, {}, target).bodyLines.empty(), "Guest preferences absent");
        SaveFlags saveFlags{};
        const auto save = std::string(argv[2]) + "/guest-completion.park";
        Require(ScenarioSave(state, save, saveFlags) && context->LoadParkFromFile(save), "Guest save/reload failed");
        Require(
            G::Resolve(target)->getName() == "Mónica" && G::Roster({ {}, true }).size() == 1 && state.park.cash == cash
                && state.park.expenditureTable == ledger,
            "Guest persistence/accounting differs");
        Require(G::SetName(target, "Mónica", "", true).error == GameActions::Status::ok, "Guest default reset failed");
        GameActions::ProcessQueue(state);
        Require(
            !G::Resolve(target)->name
                && G::SetName(target, G::Resolve(target)->getName(), "", false).error != GameActions::Status::ok,
            "Guest repeated default reset accepted");
        std::cout << "Native guest groups / names-Unicode-default-stale / tracking-flags-filter / preferences / "
                     "accounting-save passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
