// SPDX-License-Identifier: GPL-3.0-only
// Asset-backed scenario, native actions/progression, production Pocket window rendering.
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FinanceInformation.h>
#include <openrct2-ui/handheld/ManagementNews.h>
#include <openrct2-ui/handheld/ResearchCatalog.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 6)
            return 2;
        SDL_setenv("OPENRCT2_HANDHELD", "1", 1);
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[5];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = false;
        auto env = CreatePlatformEnvironment();
        env->SetBasePath(DirBase::rct2, argv[2]);
        auto ui = Ui::CreateUiContext(*env);
        auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        Config::SetDefaults();
        Config::Get().general.rct2Path = argv[2];
        Config::Get().general.windowWidth = 512;
        Config::Get().general.windowHeight = 384;
        Config::Get().general.windowScale = 1;
        Check(context->Initialise(), "Context failed");
        context->GetUiContext().CreateWindow();
        Check(context->LoadParkFromFile(argv[3], false, true), "Scenario failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::parkInformation);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        Check(WindowGetMain() || W::MainOpen(), "Main viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        auto& state = getGameState();
        gGamePaused = 0;
        auto execute = [&](auto& a) {
            Check(GameActions::Query(&a, state).error == GameActions::Status::ok, "Native action query failed");
            Check(GameActions::Execute(&a, state).error == GameActions::Status::ok, "Native action failed");
            GameActions::ProcessQueue(state);
        };
        GameActions::ParkSetParameterAction open(GameActions::ParkParameter::open);
        execute(open);
        for (const auto& ride : RideManager(state))
            if (ride.isRide() && ride.status == RideStatus::closed)
            {
                GameActions::RideSetStatusAction a(ride.id, RideStatus::open);
                if (GameActions::Query(&a, state).error == GameActions::Status::ok)
                    execute(a);
            }
        GameActions::ParkMarketingAction campaign(ADVERTISING_CAMPAIGN_PARK, 0, 2);
        execute(campaign);
        GameActions::ParkSetResearchFundingAction funding(0x7f, 3);
        execute(funding);
        std::filesystem::create_directories(argv[4]);
        std::ofstream provenance(std::filesystem::path(argv[4]) / "provenance.txt");
        provenance
            << "Production Pocket native window/X8 software renders; SDL dummy host, not hardware framebuffer.\nScenario: "
            << state.scenarioOptions.name
            << "\nInitial native actions: open eligible rides/park, two-week park advertising, maximum research funding.\nNo "
               "scenario statistics/history/news/research records patched.\n";
        H::ControllerInput input;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm };
        H::Frame cancel;
        cancel.actions = { H::Action::back };
        auto action = [&](H::Frame frame) { H::HandleMenuInput(input, frame); };
        auto section = [&](int s) {
            Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
            W::HandheldDashboardOpen();
            W::HandheldDashboardMove(0, s - static_cast<int>(W::HandheldDashboardSectionIndex()));
            action(confirm);
        };
        auto capture = [&](const char* name) {
            const auto cash = state.park.cash;
            const auto ledger = state.park.expenditureTable;
            if (auto* native = dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine()))
            {
                native->Drawing::X8DrawingEngine::Resize(512, 384);
                native->Drawing::X8DrawingEngine::BeginDraw();
                ViewportUpdatePosition(WindowGetMain());
                native->Drawing::X8DrawingEngine::EndDraw();
            }
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            Drawing::UpdatePaletteEffects();
            auto& rt = *engine.getRT();
            if (auto* main = WindowGetMain(); main && main->viewport)
                ViewportRender(rt, main->viewport);
            auto* menu = Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu);
            Check(menu, "Capture menu missing");
            menu->onDraw(rt);
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(argv[4]) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
            Check(cash == state.park.cash && ledger == state.park.expenditureTable, "Rendering changed accounting");
            auto m = W::HandheldDashboardSectionIndex() == 6 ? W::HandheldDashboardPresentation(0)
                                                             : W::HandheldParkMapSubjectsPresentation();
            provenance << name << " ticks=" << state.currentTicks << " month=" << state.date.GetMonthsElapsed()
                       << " cash=" << cash << " title=" << m.title << "\n";
            for (auto& s : m.stats)
                provenance << s.first << ": " << s.second << "\n";
        };
        gGamePaused = 1;
        section(4);
        W::HandheldDashboardMove(0, 4);
        action(confirm);
        capture("01-current-cash-less-loan.png");
        H::Frame older;
        older.navigateX = -1;
        for (int i = 0; i < 63; ++i)
            action(older);
        capture("02-native-undefined-history.png");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        gGamePaused = 0;
        for (int i = 0; i < 32768; ++i)
        {
            gameStateUpdateLogic();
        }
        gGamePaused = 1;
        section(4);
        W::HandheldDashboardMove(0, 3);
        action(confirm);
        capture("03-ledger-current.png");
        W::HandheldMenuMove(11);
        capture("04-ledger-category-navigation.png");
        H::Frame previous;
        previous.navigateX = -1;
        action(previous);
        capture("05-ledger-older-period.png");
        W::HandheldMenuMove(5);
        capture("06-ledger-dense-controls.png");
        section(4);
        W::HandheldDashboardMove(0, 4);
        action(confirm);
        capture("07-cash-history.png");
        action(confirm);
        capture("08-profit-history.png");
        for (size_t i = 0; i < 64; ++i)
            if (H::FinanceInformation::Series(1)[i] != kMoney64Undefined && H::FinanceInformation::Series(1)[i] < 0)
            {
                for (size_t j = 0; j < i; ++j)
                    action(older);
                capture("08b-native-negative-profit-sample.png");
                break;
            }
        action(confirm);
        capture("09-park-value-history.png");
        section(5);
        W::HandheldDashboardMove(0, 2);
        action(confirm);
        capture("10-invention-catalog.png");
        auto catalog = H::ResearchCatalog::Roster(0);
        Check(!catalog.empty(), "Native catalog empty");
        action(confirm);
        capture("11-native-invention-preview.png");
        action(cancel);
        H::Frame filter;
        filter.navigateX = 1;
        for (int i = 0; i < 7; ++i)
        {
            action(filter);
            auto name = "12-category-" + std::to_string(i + 1) + ".png";
            capture(name.c_str());
        }
        for (auto type : { News::ItemType::campaign, News::ItemType::research, News::ItemType::money })
        {
            auto news = H::ParkInformation::NewsList();
            auto found = std::find_if(news.begin(), news.end(), [&](const auto& n) {
                return n.item.type == type && H::ManagementNews::Available(n) != H::ManagementNews::Link::none;
            });
            if (found == news.end())
            {
                provenance << "No naturally available linked message type=" << static_cast<int>(EnumValue(type)) << "\n";
                continue;
            }
            section(0);
            W::HandheldDashboardMove(0, 3);
            action(confirm);
            W::HandheldMenuMove(static_cast<int>(found - news.begin()));
            action(confirm);
            auto prefix = "13-news-" + std::to_string(EnumValue(type));
            capture((prefix + "-detail.png").c_str());
            W::HandheldMenuMove(2);
            action(confirm);
            capture((prefix + "-linked.png").c_str());
            action(cancel);
            Check(W::HandheldParkMapSubjectsPresentation().breadcrumb == "Overview / Park news", "News recovery failed");
            capture((prefix + "-return.png").c_str());
        }
        std::cout << "Finance/research production scenario captures passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
