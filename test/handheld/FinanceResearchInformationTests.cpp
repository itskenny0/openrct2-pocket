// SPDX-License-Identifier: GPL-3.0-only
// Native map fixture, real SDL controller routing and production map rendering.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FinanceInformation.h>
#include <openrct2-ui/handheld/MapSubjects.h>
#include <openrct2-ui/handheld/ParkInformation.h>
#include <openrct2-ui/handheld/ParkMap.h>
#include <openrct2-ui/handheld/ParkOverview.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/general/MapChangeSizeAction.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/actions/peep/StaffFireAction.h>
#include <openrct2/actions/ride/RideCreateAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Park.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
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
        gameStateInitAll(state, { 64, 48 });
        // Scenario metadata initialized before native actions/progression.
        state.date = Date::FromYMD(3, 4);
        state.park.cash = 10000.00_GBP;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario, ParkFlag::forbidMarketingCampaigns);
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "Main viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        namespace F = H::FinanceInformation;
        F::Ledger ledger;
        ledger.Begin();
        const auto month = ledger.month;
        Check(ledger.Age() == 0 && ledger.Net() == 0, "Initial ledger mismatch");
        const auto cash = state.park.cash;
        GameActions::ParkMarketingAction campaign(ADVERTISING_CAMPAIGN_PARK, 0, 2);
        const auto quote = GameActions::Query(&campaign, state);
        Check(quote.error == GameActions::Status::ok && state.park.cash == cash, "Campaign query changed accounting");
        Check(GameActions::Execute(&campaign, state).error == GameActions::Status::ok, "Campaign execute failed");
        GameActions::ProcessQueue(state);
        Check(
            ledger.Value(EnumValue(ExpenditureType::marketing)) == -quote.cost && state.park.cash == cash - quote.cost,
            "Native campaign ledger payment mismatch");
        GameActions::ParkSetResearchFundingAction funding(0x7f, 3);
        Check(GameActions::Execute(&funding, state).error == GameActions::Status::ok, "Funding action failed");
        GameActions::ProcessQueue(state);
        FinancePayResearch();
        Check(
            ledger.Value(EnumValue(ExpenditureType::research)) == -kResearchCosts[3] / 4,
            "Native weekly research ledger mismatch");
        FinanceUpdateDailyProfit();
        Park::UpdateHistories(state.park);
        Check(F::Series(0)[0] == state.park.cash - state.park.bankLoan, "Cash history is not native cash less loan");
        Check(
            F::Series(1)[0] != kMoney64Undefined && F::Series(2)[0] == state.park.value, "Native profit/value history missing");
        Check(F::History(0, 63, 0).stats[1].second == H::FormatMoney(kMoney64Undefined), "Missing finance sample fabricated");
        const auto net = ledger.Net();
        Check(net == -quote.cost - kResearchCosts[3] / 4, "Net categories invented gross values");
        const auto date = GetDate().GetMonthsElapsed();
        while (GetDate().GetMonthsElapsed() == date)
            DateUpdate(state);
        FinanceShiftExpenditureTable();
        ledger.Clamp();
        Check(ledger.month == month && ledger.Age() == 1 && ledger.Net() == net, "Month shift lost selected period");
        ledger.Move(1);
        Check(ledger.Age() == 0 && ledger.Net() == 0, "Newest period has stale ledger entries");
        for (int i = 0; i < 100; ++i)
            ledger.Move(-1);
        Check(ledger.Age() == 15, "Ledger escaped retained history");
        for (int i = 0; i < 100; ++i)
            ledger.Move(1);
        Check(ledger.Age() == 0, "Ledger escaped current month");
        Check(ledger.Value(99) == kMoney64Undefined, "Invalid expenditure category accepted");
        auto model = F::LedgerPresentation(ledger, EnumValue(ExpenditureType::wages));
        Check(model.rows.size() == 17 && model.stats[1].second == H::FormatMoney(0), "Zero/category presentation missing");
        H::ControllerInput input;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm };
        auto openFinance = [&] {
            W::HandheldDashboardOpen();
            W::HandheldDashboardMove(0, 4 - static_cast<int>(W::HandheldDashboardSectionIndex()));
            H::HandleMenuInput(input, confirm);
            Check(W::HandheldFinancesIsOpen(), "Finance dashboard entry failed");
        };
        openFinance();
        W::HandheldDashboardMove(0, 3);
        Check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Virtual controller init failed");
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(device >= 0, "Virtual device missing");
        auto* joystick = SDL_JoystickOpen(device);
        Check(joystick, "Virtual joystick missing");
        auto pump = [&] {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                input.handleEvent(e);
        };
        pump();
        input.update(true, 100);
        pump();
        input.update(true, 110);
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 1);
        SDL_JoystickUpdate();
        pump();
        const auto pressed = input.update(true, 120);
        Check(!pressed.actions.empty(), "Mapped A missing");
        H::HandleMenuInput(input, pressed);
        const auto held = input.update(true, 130);
        Check(held.actions.empty(), "Held A repeated into ledger");
        H::HandleMenuInput(input, held);
        SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, 0);
        SDL_JoystickUpdate();
        pump();
        input.update(true, 140);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);
        pump();
        input.suspend();
        Check(!W::HandheldFinancesIsOpen() && W::HandheldConstructionMenuIsOpen(), "Ledger entry failed");
        H::Frame older;
        older.navigateX = -1;
        H::HandleMenuInput(input, older);
        Check(
            W::HandheldParkMapSubjectsPresentation().title == F::Period(GetDate().GetMonthsElapsed() - 1),
            "Controller month navigation failed");
        H::Frame cancel;
        cancel.actions = { H::Action::back };
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldFinancesIsOpen(), "Cancellation lost finance context");
        Check(state.park.cash == cash + net, "Read-only ledger changed cash");
        W::HandheldDashboardMove(0, 4);
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldParkMapSubjectsPresentation().title == "Cash less loan history", "Financial graph entry failed");
        H::HandleMenuInput(input, confirm);
        Check(W::HandheldParkMapSubjectsPresentation().title == "Weekly profit history", "Financial graph cycling failed");
        for (int i = 0; i < 100; ++i)
            H::HandleMenuInput(input, older);
        Check(
            W::HandheldParkMapSubjectsPresentation().stats[1].first == "Sample 63 weeks back", "History sample escaped bounds");
        H::HandleMenuInput(input, cancel);
        Check(W::HandheldFinancesIsOpen(), "History return lost Finances");
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/finance-information.park";
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Ledger save/reload failed");
        Check(
            state.park.expenditureTable[1][EnumValue(ExpenditureType::marketing)] == -quote.cost, "Ledger persistence failed");
        Check(!Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu), "Load retained transient ledger");
        Check(F::Series(0)[0] == state.park.cash - state.park.bankLoan, "Finance history did not persist");
        if (argc == 4)
        {
            // Native graph raster regression: a missing record must not connect its neighbours.
            Drawing::X8DrawingEngine engine(context->GetUiContext()); engine.Resize(512,384); engine.BeginDraw();
            auto& rt=*engine.getRT();
            const money64 samples[]={1000,kMoney64Undefined,-1000,0};
            Graph::GraphProperties<money64> props{};
            props.series=samples; props.min=-2000; props.max=2000; props.hoverIdx=-1;
            props.darkBackground=true; props.lineCol=ColourWithFlags{Drawing::Colour::grey};
            props.RecalculateLayout({{100,100},{190,190}},4,4);
            auto crossesGap=[&] {
                const auto* pixels=reinterpret_cast<const uint8_t*>(rt.bits);
                for(int y=140;y<151;++y) if(pixels[y*rt.LineStride()+160]==21) return true;
                return false;
            };
            std::fill_n(reinterpret_cast<uint8_t*>(rt.bits),rt.LineStride()*rt.height,uint8_t{118});
            Graph::DrawFinanceGraph(rt,props);
            Check(crossesGap(),"Native graph fixture did not expose continuity baseline");
            std::fill_n(reinterpret_cast<uint8_t*>(rt.bits),rt.LineStride()*rt.height,uint8_t{118});
            props.breakOnMissing=true; Graph::DrawFinanceGraph(rt,props);
            Check(!crossesGap(),"Native finance graph bridged an undefined record");
            engine.EndDraw();
        }
        // Separate initialized no-money/undefined scenario fixture; no gameplay patches.
        gameStateInitAll(state, { 32, 32 });
        state.park.flags.set(ParkFlag::noMoney);
        state.park.expenditureTable[0][0] = kMoney64Undefined;
        ledger.Begin();
        Check(
            ledger.Net() == kMoney64Undefined && F::LedgerPresentation(ledger, 0).stats[0].second == "Not applicable",
            "Undefined/no-money ledger mishandled");
        Check(F::History(0, 0, 0).stats[0].second == "Not applicable", "No-money financial graph fabricated values");
        std::cout << "Native finance histories and expenditure actions/calendar/bounds/controller/cancellation/save passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
