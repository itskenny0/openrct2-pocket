// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <openrct2-ui/handheld/ChoiceCards.h>
#include <thread>
#include <openrct2-ui/handheld/ToolSettingsPanel.h>
#include <openrct2-ui/handheld/DesignAvailability.h>
#include <openrct2-ui/handheld/GuestManagement.h>
#include <openrct2-ui/handheld/StaffPatrol.h>
#include <openrct2-ui/handheld/ResearchCatalog.h>
#include <openrct2/management/NewsItem.h>
#include <openrct2-ui/handheld/StaffManagement.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/actions/ride/RideSetPriceAction.h>
#include "RideTabNavigation.h"
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/management/Research.h>
#include <openrct2/interface/Viewport.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/ride/Vehicle.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
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
        gameStateInitAll(state, { 64, 64 });
        gLegacyScene = LegacyScene::playing;
        Check(W::MainOpen(), "Fixture viewport missing");
        WindowGetMain()->onResize();
        InputSetState(InputState::normal);
        Ui::GetWindowManager()->CloseByClass(WindowClass::progressWindow);
        state.park.cash = 10000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        for (int y = 32; y < 2016; y += 32)
            for (int x = 32; x < 2016; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto& objects = context->GetObjectManager();
        Check(
            objects.LoadObject("rct2.terrain_surface.grass") && objects.LoadObject("rct2.terrain_edge.rock")
                && objects.LoadObject("rct2.station.plain"),
            "Fixture terrain/station missing");
        namespace F=H::FlatRides;namespace M=H::Mazes;
        auto* wm=Ui::GetWindowManager();H::ControllerInput input;
        auto press=[&](H::Action action){H::Frame frame;frame.actions={action,action};H::HandleExplorerInput(input,frame);};
        auto closeMenu=[&](){wm->CloseByClass(WindowClass::handheldMenu);WindowCullDead();};
        auto focus=[&](CoordsXYZ point){auto* v=WindowGetMain()->viewport;v->zoom=ZoomLevel{0};
            v->viewPos=Translate3DTo2DWithZ(v->rotation,point)-ScreenCoordsXY{256,192};WindowGetMain()->savedViewPos=v->viewPos;};
        auto capture=[&](const char* name){const auto* folder=std::getenv("POCKET_BRICK_AUDIT_CAPTURES");if(!folder||argc!=4)return;
            std::filesystem::create_directories(folder);Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,WindowGetMain()->viewport);
            if(auto* window=wm->FindByClass(WindowClass::handheldMenu))window->onDraw(rt);else H::DrawExplorer(rt);
            Check(H::FooterHeight({ 512, 384 }, H::LastControlHints) == 28, "Runtime footer expanded beyond one row");
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            const auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(folder)/name).string(),image,ImageFormat::png);engine.EndDraw();};

        auto move=[&](int x,int y){H::Frame f;f.navigateX=x;f.navigateY=y;H::HandleExplorerInput(input,f);};
        const auto cash=state.park.cash;
        H::AreaTools::settings={};H::BeginAreaTools();focus({640,640,16});
        press(H::Action::contextSecondary);move(0,1);move(1,0);move(0,1);move(1,0);
        Check(H::AreaTools::settings.width==2 && H::AreaTools::settings.height==2,"Brush editor changed elevation instead of footprint");
        capture("01-landscape-adjust.png");
        press(H::Action::contextPrimary);W::HandheldMenuMove(3);capture("02-landscape-settings.png");
        H::AreaTools::settings.operation=H::AreaTools::Operation::waterRaise;
        W::HandheldMenuMove(1);const auto selection=H::AreaTools::settings.selection;
        move(1,0);press(H::Action::confirm);
        Check(H::AreaTools::settings.selection==selection,"Disabled water shape responded to input");
        W::HandheldMenuMove(1);press(H::Action::confirm);Check(W::HandheldObjectPickerIsOpen(),"Landscape material did not open visual catalog");
        press(H::Action::back);move(-1,0);
        Check(H::AreaTools::settings.surface!=kObjectEntryIndexNull,"Material picker Back lost selected setting");
        press(H::Action::back);Check(!wm->FindByClass(WindowClass::handheldMenu),"Landscape settings lost caller");
        press(H::Action::back);H::EndTerrain();closeMenu();

        namespace S=H::SceneryTools;
        auto* roof=objects.LoadObject("rct2.scenery_small.corroof");Check(roof,"Scenery missing");
        const auto roofIndex=objects.GetLoadedObjectEntryIndex(roof);ScenerySetInvented({SCENERY_TYPE_SMALL,roofIndex});
        S::SelectCategory(S::Category::small);Check(S::Select(roofIndex),"Scenery select failed");H::BeginScenery();
        focus({640,640,16});S::UpdatePreview(CoordsXY{640,640});
        const auto initial=S::Place(CoordsXY{640,640},S::Current(),false);
        Check(initial.error==GameActions::Status::ok,"Ground scenery query failed");
        press(H::Action::contextSecondary);move(0,2);move(1,0);
        Check(S::Current().height==initial.position.z+kCoordsZStep,"Height field did not raise from real preview");
        capture("03-scenery-height.png");
        press(H::Action::confirm);Check(S::Current().height==0,"A did not restore ground-following");
        move(1,0);Check(S::Current().height==initial.position.z+kCoordsZStep,"Height reset exited adjustment or lost focus");
        press(H::Action::confirm);press(H::Action::nextOption);capture("04-scenery-colour.png");
        press(H::Action::nextOption);press(H::Action::nextOption);move(1,0);move(0,1);move(1,0);
        capture("05-scatter-adjust.png");press(H::Action::contextPrimary);W::HandheldMenuMove(18);capture("06-scenery-settings.png");
        press(H::Action::back);Check(H::IsSceneryToolActive(),"Scenery settings lost tool");
        Check(state.park.cash==cash,"Tool configuration spent money");H::EndTerrain();closeMenu();

        H::Paths::Begin(false,false);H::Paths::UpdateTarget({256,192});
        W::HandheldPathOptionsOpen();W::HandheldMenuMove(1);capture("07-ground-path-settings.png");
        press(H::Action::back);H::EndTerrain();closeMenu();
        H::Paths::BeginConnected(false);W::HandheldPathOptionsOpen();W::HandheldMenuMove(5);capture("08-connected-path-settings.png");
        press(H::Action::back);H::EndTerrain();closeMenu();

        auto* coaster=objects.LoadObject("rct2.ride.arrt1");Check(coaster,"Coaster object missing");
        const auto rideIndex=objects.GetLoadedObjectEntryIndex(coaster);
        ResearchInsertRideEntry(rideIndex,true);RideEntrySetInvented(rideIndex);RideTypeSetInvented(RIDE_TYPE_CORKSCREW_ROLLER_COASTER);
        Check(H::BeginCustomTrack(rideIndex,RIDE_TYPE_CORKSCREW_ROLLER_COASTER),"Coaster begin failed");
        GameActions::ProcessQueue(state);C::Update(CoordsXY{960,960});
        Check(C::Action(true).error==GameActions::Status::ok,"Station placement failed");GameActions::ProcessQueue(state);
        const auto target=*C::Target();const auto station=H::TrackEditing::Blocks(target).front();
        const auto before=H::TrackEditing::Context(station);const auto trackCash=state.park.cash;
        H::EndTerrain();W::HandheldRideOpen(target.id);FocusRideTabAction("Edit track...");press(H::Action::confirm);press(H::Action::confirm);
        auto select=[&](const std::string& label){const auto m=W::HandheldRidesPresentation();const auto it=std::find(m.rows.begin(),m.rows.end(),label);
            Check(it!=m.rows.end(),"Track action missing");W::HandheldMenuMove(static_cast<int>(it-m.rows.begin())-static_cast<int>(m.selected));};
        select("Inspect connections / station...");press(H::Action::confirm);capture("09-track-connections.png");
        auto m=W::HandheldRidesPresentation();
        Check(std::find(m.rows.begin(),m.rows.end(),"Entrance: Needs building")!=m.rows.end(),"Station access status missing");
        select("Technical details...");press(H::Action::confirm);capture("10-track-technical.png");
        m=W::HandheldRidesPresentation();Check(m.rows[m.selected]=="Hide technical details","Technical toggle lost focus");
        press(H::Action::confirm);m=W::HandheldRidesPresentation();Check(m.rows[m.selected]=="Technical details...","Hide toggle lost focus");
        press(H::Action::back);Check(W::HandheldRidesPresentation().rows[W::HandheldRidesPresentation().selected]=="Inspect connections / station...","B lost track caller focus");
        Check(H::TrackEditing::Context(station)==before && state.park.cash==trackCash,"Connection inspection changed track or cash");
        closeMenu();

        TrackDesign td{};td.trackAndVehicle.rtdIndex=RIDE_TYPE_CORKSCREW_ROLLER_COASTER;
        td.trackAndVehicle.vehicleObject=coaster->GetDescriptor();
        const auto ordinary=H::DesignAvailability::Lines(td), technical=H::DesignAvailability::Lines(td,true);
        Check(std::none_of(ordinary.begin(),ordinary.end(),[](const auto& row){return row.starts_with("Vehicle ID:");}),"Ordinary design details expose object IDs");
        Check(std::any_of(technical.begin(),technical.end(),[](const auto& row){return row.starts_with("Vehicle ID:");}),"Advanced design identity missing");
        std::cout<<"PASS friendly tool settings / direct height and ground reset / native previews / track details toggle and focus / no-spend / footer\n";
        context.reset();return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
