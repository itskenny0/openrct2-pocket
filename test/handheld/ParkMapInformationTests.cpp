// SPDX-License-Identifier: GPL-3.0-only
// Native map fixture, real SDL controller routing and production map rendering.
#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/ParkInformation.h>
#include <openrct2-ui/handheld/ParkMap.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/general/MapChangeSizeAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Window.h>
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
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "Main viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        H::ParkMap::State map;
        map.Begin();
        for (int i = 0; i < 4; ++i)
            map.Update();
        Check(map.ready && map.extent == 132, "Rectangular map cache incomplete");
        for (int y = 32; y < 47 * 32; y += 32)
            for (int x = 32; x < 63 * 32; x += 32)
            {
                const auto p = W::NativeMiniMapCoords({ x, y });
                const auto c = W::NativeMiniMapTileColour({ x, y }, false);
                Check(p.x > 0 && p.y >= 0 && p.x < map.extent && p.y < map.extent, "Native coordinate outside raster");
                Check(map.pixels[p.y * map.extent + p.x] == c[1], "Cache colour differs from desktop map");
            }
        map.Move(-10000, 10000);
        Check(map.focus == CoordsXY{ 32, 46 * 32 }, "Focus escaped map bounds");
        map.Mode();
        Check(map.rotation == 255, "Mode retained stale raster");
        map.Update();
        H::ControllerInput input;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm };
        auto openMap = [&] {
            W::HandheldDashboardOpen();
            W::HandheldDashboardMove(1, 0);
            H::HandleMenuInput(input, confirm);
            Check(W::HandheldOverviewIsOpen(), "Overview entry failed");
            W::HandheldMenuMove(2);
            H::HandleMenuInput(input, confirm);
            Check(W::HandheldParkMapIsOpen(), "Pocket map entry failed");
        };
        openMap();
        for (int rotation = 0; rotation < 4; ++rotation)
        {
            H::Frame frame;
            frame.actions = { H::Action::contextPrimary };
            H::HandleMenuInput(input, frame);
            map.Update();
            Check(map.rotation == GetCurrentRotation(), "Rotation cache stale");
        }
        H::Frame move;
        move.navigateX = 1;
        move.navigateY = 1;
        H::HandleMenuInput(input, move);
        for (auto a : { H::Action::contextSecondary, H::Action::previousOption, H::Action::nextOption })
        {
            H::Frame f;
            f.actions = { a };
            H::HandleMenuInput(input, f);
        }
        W::HandheldMenuBack();
        Check(W::HandheldOverviewIsOpen(), "Map Back failed");
        W::HandheldMenuMove(2);
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        Check(!W::HandheldParkMapIsOpen(), "Locate failed to return to park");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        GameActions::MapChangeSizeAction resize({ 80, 56 });
        Check(GameActions::Query(&resize, state).error == GameActions::Status::ok, "Native map resize query failed");
        Check(GameActions::Execute(&resize, state).error == GameActions::Status::ok, "Native map resize failed");
        GameActions::ProcessQueue(state);
        map.Update();
        Check(map.size == state.mapSize && map.extent == 164, "Changed dimensions retained cache");
        namespace I = H::ParkInformation;
        Check(I::NewsList().empty(), "Native initial news should be empty");
        Check(I::NewsPresentation({}, 0).status == "No native park messages", "Empty news state missing");
        const uint32_t assoc = 640u | (640u << 16);
        News::AddItemToQueue(state.park.newsItems, News::ItemType::blank, "Native fixture location message", assoc);
        auto news = I::NewsList();
        Check(news.size() == 1 && !news[0].archived && I::Current(news[0]), "Native news ordering/membership failed");
        const auto location = I::Location(news[0]);
        Check(location && location->x == 640 && location->y == 640, "Native news location failed");
        auto changed = news[0];
        changed.item.text += " changed";
        Check(!I::Current(changed) && !I::Location(changed), "Stale news text accepted");
        News::CloseCurrentItem(state.park.newsItems);
        Check(!I::Current(news[0]), "Moved news retained recent baseline");
        news = I::NewsList();
        Check(news.size() == 1 && news[0].archived && I::Current(news[0]), "Archived news missing");
        News::RemoveItem(state.park.newsItems, News::ItemHistoryStart);
        Check(!I::Current(news[0]) && !I::Location(news[0]), "Expired archived message accepted");
        Check(I::Awards(0).status == "No active native awards", "Empty awards missing");
        AwardGrant(AwardType::bestStaff);
        Check(
            I::Awards(0).rows.size() == 2 && I::Awards(0).rows[0].starts_with("Positive:"), "Native award information missing");
        Check(state.park.ratingHistory[0] == kParkRatingHistoryUndefined, "Fixture history unexpectedly defined");
        Park::UpdateHistories(state.park);
        Check(
            state.park.ratingHistory[0] == state.park.rating && state.park.ratingHistory[1] == kParkRatingHistoryUndefined,
            "Native history order/undefined samples changed");
        const auto history = I::History(false, 0);
        Check(history.stats[0].second == std::to_string(state.park.rating), "History invented current rating");
        W::HandheldDashboardOpen();
        W::HandheldDashboardMove(1, 0);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuMove(3);
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        Check(W::HandheldOverviewIsOpen(), "News Back failed to recover Overview");
        W::HandheldMenuMove(4);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuBack();
        W::HandheldMenuMove(5);
        H::HandleMenuInput(input, confirm);
        H::HandleMenuInput(input, confirm);
        W::HandheldMenuBack();
        Check(W::HandheldOverviewIsOpen(), "History Back failed");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        SaveFlags flags{};
        const auto save = std::string(argv[2]) + "/map.park";
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Map save/reload failed");
        map.Update();
        Check(map.size == state.mapSize, "Map dimensions not recovered");
        if (const auto* scenario = std::getenv("POCKET_MAP_SCENARIO"))
            Check(context->LoadParkFromFile(scenario), "Capture scenario failed");
        if (const auto* output = std::getenv("POCKET_MAP_CAPTURES"); output && argc == 4)
        {
            std::filesystem::create_directories(output);
            auto capture = [&](const char* name) {
                Drawing::X8DrawingEngine engine(context->GetUiContext());
                engine.Resize(512, 384);
                engine.BeginDraw();
                Drawing::UpdatePaletteEffects();
                auto& rt = *engine.getRT();
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
                Image image;
                image.Width = 512;
                image.Height = 384;
                image.Depth = 8;
                image.Stride = rt.LineStride();
                image.Palette = Drawing::gPalette;
                const auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
                image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
                Imaging::WriteToFile((std::filesystem::path(output) / name).string(), image, ImageFormat::png);
                engine.EndDraw();
            };
            Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
            openMap();
            auto* menu = Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu);
            for (int i = 0; i < 1000; ++i)
                menu->onUpdate();
            capture("map-01-terrain.png");
            W::HandheldParkMapControl(0);
            for (int i = 0; i < 1000; ++i)
                menu->onUpdate();
            capture("map-02-rides.png");
            W::HandheldParkMapControl(2);
            W::HandheldConstructionMove(4, 2);
            capture("map-03-zoom.png");
            W::HandheldMenuBack();
            W::HandheldMenuMove(3);
            H::HandleMenuInput(input, confirm);
            capture("information-04-news.png");
            const auto nativeNews = I::NewsList();
            if (!nativeNews.empty())
            {
                H::HandleMenuInput(input, confirm);
                capture("information-05-news-details.png");
                W::HandheldMenuBack();
            }
            W::HandheldMenuBack();
            W::HandheldMenuMove(4);
            H::HandleMenuInput(input, confirm);
            capture("information-06-awards.png");
            W::HandheldMenuBack();
            W::HandheldMenuMove(5);
            H::HandleMenuInput(input, confirm);
            capture("information-07-rating-history.png");
            H::HandleMenuInput(input, confirm);
            capture("information-08-guest-history.png");
        }
        H::EndTerrain();
        std::cout << "Native map raster/bounds/rotation/resize/save and SDL navigation/locate passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
