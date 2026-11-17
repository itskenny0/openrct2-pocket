// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include "RideTabNavigation.h"

#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ChoiceCards.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/DesignAvailability.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/GuestManagement.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/ResearchCatalog.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/StaffManagement.h>
#include <openrct2-ui/handheld/StaffPatrol.h>
#include <openrct2-ui/handheld/ToolSettingsPanel.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/ride/RideSetPriceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/management/NewsItem.h>
#include <openrct2/management/Research.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <thread>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace C = H::CustomTracks;
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
        if (argc != 4)
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
        Check(
            context->LoadParkFromFile(std::string(argv[3]) + "/Scenarios/Six Flags Holland.SC6", false, true),
            "Real scenario load failed");
        gLegacyScene = LegacyScene::playing;
        Check(WindowGetMain(), "Park viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        auto* wm = Ui::GetWindowManager();
        wm->CloseAllExceptFlags({ WindowFlag::stickToBack, WindowFlag::stickToFront });
        WindowCullDead();
        H::ControllerInput input;
        auto press = [&](H::Action action) {
            H::Frame f;
            f.actions = { action };
            H::HandleExplorerInput(input, f);
        };
        auto close = [&]() {
            H::EndTerrain();
            wm->CloseByClass(WindowClass::handheldMenu);
            WindowCullDead();
        };
        auto focus = [&](CoordsXYZ point, int zoom = 0) {
            auto* v = WindowGetMain()->viewport;
            v->zoom = ZoomLevel{ static_cast<int8_t>(zoom) };
            v->viewPos = Translate3DTo2DWithZ(v->rotation, point)
                - ScreenCoordsXY{ v->zoom.ApplyTo(256), v->zoom.ApplyTo(192) };
            WindowGetMain()->savedViewPos = v->viewPos;
        };
        auto capture = [&](const std::string& name) {
            const auto* folder = std::getenv("POCKET_BRICK_AUDIT_CAPTURES");
            Check(folder, "Capture folder missing");
            std::filesystem::create_directories(folder);
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, WindowGetMain()->viewport);
            if (auto* w = wm->FindByClass(WindowClass::handheldMenu))
                w->onDraw(rt);
            else
                H::DrawExplorer(rt);
            Image image;
            image.Width = 512;
            image.Height = 384;
            image.Depth = 8;
            image.Stride = rt.LineStride();
            image.Palette = Drawing::gPalette;
            auto* bytes = reinterpret_cast<uint8_t*>(rt.bits);
            image.Pixels.assign(bytes, bytes + rt.LineStride() * rt.height);
            Imaging::WriteToFile((std::filesystem::path(folder) / name).string(), image, ImageFormat::png);
            engine.EndDraw();
        };
        const auto roster = H::RideManagement::Roster();
        Check(roster.size() > 5, "Scenario is not populated");
        std::cout << "Loaded Six Flags Holland: " << roster.size() << " rides\n";
        for (size_t i = 0; i < roster.size(); ++i)
        {
            const auto* r = H::RideManagement::Resolve(roster[i]);
            auto p = H::RideManagement::Location(roster[i]);
            std::cout << i << " " << r->getName() << " " << (p ? p->x : 0) << "," << (p ? p->y : 0) << "\n";
            if (p)
            {
                focus(*p, 1);
                capture("park-" + std::to_string(i) + ".png");
            }
        }
        auto target = roster.front();
        auto point = H::RideManagement::Location(target);
        if (point)
            focus(*point, 1);
        press(H::Action::openBuild);
        Check(wm->FindByClass(WindowClass::handheldMenu), "Select did not open Build");
        capture("build.png");
        close();
        W::HandheldRideOpen(target.id);
        capture("ride.png");
        close();
        namespace S = H::SceneryTools;
        S::SelectCategory(S::Category::small);
        S::SetGroup(kObjectEntryIndexNull);
        S::SetSearch("");
        const auto entries = S::Entries(S::Category::small);
        Check(!entries.empty(), "Scenario scenery unavailable");
        Check(S::Select(entries.front().identity.index), "Scenario scenery selection failed");
        W::HandheldSceneryCatalogOpen();
        capture("scenery-catalog.png");
        close();
        H::BeginScenery();
        W::HandheldSceneryOptionsOpen();
        W::HandheldMenuMove(18);
        capture("scenery-settings.png");
        close();
        W::HandheldDashboardOpen();
        capture("dashboard.png");
        close();
        std::cout << "RealParkCaptures passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
