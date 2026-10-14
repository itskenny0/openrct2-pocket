#include <openrct2-ui/handheld/AccessPreview.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2/ride/AccessPlacementContext.h>
#include <openrct2/world/tile_element/EntranceElement.h>
#include <openrct2/actions/ride/FlatRideEntrancePlaceAction.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
// SPDX-License-Identifier: GPL-3.0-only
#include <openrct2-ui/handheld/ShopAppearance.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2/core/DataSerialiser.h>
#include <openrct2/object/RideObject.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2/interface/Window.h>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/actions/cheats/CheatSetAction.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
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

        gGamePaused=0;namespace F=H::FlatRides;namespace GA=GameActions;
        auto map=[&](){std::vector<uint8_t> result;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32){
            for(const auto* e:TileElementsView(CoordsXY{x,y}))if(!e->isGhost()){auto copy=*e;copy.setLastForTile(false);const auto* p=reinterpret_cast<const uint8_t*>(&copy);result.insert(result.end(),p,p+sizeof(copy));}}return result;};
        auto ghosts=[&](){int n=0;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32)for(const auto* e:TileElementsView<EntranceElement>(CoordsXY{x,y}))n+=e->isGhost();return n;};
        auto* object=objects.LoadObject("rct2.ride.mgr1");Check(object,"Carousel fixture missing");auto index=objects.GetLoadedObjectEntryIndex(object);
        ResearchInsertRideEntry(index,true);RideEntrySetInvented(index);RideTypeSetInvented(FlatRidePlacement::Type(index));
        Check(H::BeginFlatRide(index),"Native flat start failed");Check(F::Action(CoordsXY{640,640},true).error==GA::Status::ok,"Native footprint failed");GA::ProcessQueue(state);
        auto target=*F::Target();auto* ride=GetRide(target.id);Check(ride && F::CurrentStage()==F::Stage::entrance,"Native access stage missing");
        std::vector<CoordsXYZD> candidates;
        for(int y=544;y<800;y+=32)for(int x=544;x<800;x+=32){auto loc=FlatRidePlacement::EntranceLocation(*ride,{x,y});if(loc && F::Action(CoordsXY{x,y},false).error==GA::Status::ok)candidates.push_back(*loc);}
        Check(candidates.size()>2,"Native station-side fixture missing");const auto loc=candidates.front();
        auto* surface=MapGetSurfaceElementAt(loc);const auto pathLoc=CoordsXY(loc)-CoordsDirectionDelta[loc.direction];
        auto* pathObject=objects.LoadObject("rct2.footpath_surface.tarmac");auto* rail=objects.LoadObject("rct2.footpath_railings.wood");Check(pathObject && rail,"Path fixture missing");
        GA::FootpathPlaceAction path({pathLoc,surface->getBaseZ()},{},objects.GetLoadedObjectEntryIndex(pathObject),objects.GetLoadedObjectEntryIndex(rail));
        Check(GA::Execute(&path,state).error==GA::Status::ok,"Adjacent native path failed");GA::ProcessQueue(state);
        const auto before=map();const auto cash=state.park.cash;const auto stationBefore=ride->getStation();const auto contextBefore=AccessPlacementContext::Capture(*ride,loc);
        auto q=F::Action(loc,false);Check(q.error==GA::Status::ok && map()==before && state.park.cash==cash,"Native access query changed map/cash");
        F::UpdatePreview(loc);Check(ghosts()==1,"Native access ghost missing");
        Check(map()==before && state.park.cash==cash && std::memcmp(&stationBefore,&ride->getStation(),sizeof(stationBefore))==0 && contextBefore==AccessPlacementContext::Capture(*ride,loc),"Access ghost modified real station/path/map/cash");
        GA::FlatRideEntrancePlaceAction stale(target.id,loc,false);stale.GuardContext(contextBefore,q.cost);
        DataSerialiser serialised(true);stale.Serialise(serialised);serialised.getStream().SetPosition(0);GA::FlatRideEntrancePlaceAction decoded;DataSerialiser input(false,serialised.getStream());decoded.Serialise(input);Check(decoded.Query(state,state.park).error==GA::Status::ok,"Access serialized review differs");
        Ui::ProvisionalElementsRemove();Ui::ProvisionalElementsRemove();Check(ghosts()==0 && map()==before,"Nested access suspension mutated real map");Ui::ProvisionalElementsRestore();Check(ghosts()==0,"Access restored early");Ui::ProvisionalElementsRestore();Check(ghosts()==1,"Access restoration missing");
        GA::CheatSetAction pauseBuild(CheatType::buildInPauseMode,1);Check(GA::Execute(&pauseBuild,state).error==GA::Status::ok,"Native paused-build fixture failed");GA::ProcessQueue(state);
        gGamePaused=1;Ui::GetWindowManager()->CloseAll();WindowCullDead();InputSetState(InputState::normal);
        auto* main=WindowGetMain();main->viewport->zoom=ZoomLevel{0};main->savedViewPos=Translate3DTo2DWithZ(GetCurrentRotation(),CoordsXYZ{loc.x+16,loc.y+16,loc.z})-ScreenCoordsXY{256,192};main->viewport->viewPos=main->savedViewPos;
        F::UpdatePreview(loc);Check(ghosts()==1,"Valid paused access preview missing");gameStateTick();
        Check(ghosts()==(ContextHasFocus()?1:0) && map()==before && state.park.cash==cash,"Paused native access frame changed real map/cash or stranded ghost");
        gGamePaused=0;GA::CheatSetAction unpauseBuild(CheatType::buildInPauseMode,0);Check(GA::Execute(&unpauseBuild,state).error==GA::Status::ok,"Native pause setting restoration failed");GA::ProcessQueue(state);
        F::UpdatePreview({});Check(ghosts()==0 && map()==before,"Invalid target retained ghost/mutated map");F::UpdatePreview(loc);F::UpdatePreview(candidates.back());Check(ghosts()==1 && map()==before,"Changed native side duplicated/mutated ghost");F::ClearPreview();Check(ghosts()==0 && map()==before,"Clear retained native ghost");
        auto originalVersion=object->GetVersion();auto changed=originalVersion;++std::get<2>(changed);object->SetVersion(changed);Check(decoded.Query(state,state.park).error!=GA::Status::ok,"Changed object version accepted");object->SetVersion(originalVersion);
        F::UpdatePreview(loc);SaveFlags flags;flags.set(SaveFlag::automatic);const auto save=std::filesystem::path(argv[2])/"access-preview.park";
        Check(ScenarioSave(state,save.string(),flags)!=0 && ghosts()==1 && map()==before,"Native access autosave failed restoration");
        Check(F::Action(loc,true).error==GA::Status::ok,"Native access apply failed");Check(F::Action(loc,true).error!=GA::Status::ok,"Pending access repeated");GA::ProcessQueue(state);
        Check(ghosts()==0 && !ride->getStation().entrance.isNull() && state.park.cash==cash-q.cost && F::CurrentStage()==F::Stage::exit,"Native access accounting/stage failed");
        Check(decoded.Execute(state,state.park).error!=GA::Status::ok,"Changed station accepted stale action");
        Check(context->LoadParkFromFile(save.string()) && ghosts()==0 && GetRide(target.id)->getStation().entrance.isNull() && F::CurrentStage()==F::Stage::inactive,"Native autosave included access ghost/station or retained draft");
        gGamePaused=0;
        const auto nativeIndex=GetRide(target.id)->subtype;
        GA::FlatRideEntrancePlaceAction reused(target.id,loc,false);
        GA::RideDemolishAction demolish(target.id,GA::RideModifyType::demolish);
        Check(GA::Execute(&demolish,state).error==GA::Status::ok,"Native reuse demolition failed");GA::ProcessQueue(state);
        Check(F::Begin(nativeIndex),"Native slot reuse object selection failed");
        const auto replacementResult=F::Action(CoordsXY{640,640},true);
        if(replacementResult.error!=GA::Status::ok)std::cerr<<"Reuse result "<<replacementResult.getErrorMessage()<<" index "<<index<<"\n";
        Check(replacementResult.error==GA::Status::ok,"Native slot reuse recreation failed");GA::ProcessQueue(state);
        Check(F::Target()->id==target.id && reused.Execute(state,state.park).error!=GA::Status::ok,"Native same-slot replacement accepted old access action");F::End();
        std::cout<<"MazeAccessPreviewTests access native/SDL PASS\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
