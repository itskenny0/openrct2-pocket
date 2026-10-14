#include <openrct2-ui/handheld/AccessPreview.h>
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/actions/ride/MazeSetTrackAction.h>
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

        namespace M=H::Mazes;
        auto* mazeObject=objects.LoadObject("rct2.ride.hmaze");Check(mazeObject,"Native maze object missing");auto mazeIndex=objects.GetLoadedObjectEntryIndex(mazeObject);
        ResearchInsertRideEntry(mazeIndex,true);RideEntrySetInvented(mazeIndex);RideTypeSetInvented(RIDE_TYPE_MAZE);
        Check(H::BeginMaze(mazeIndex),"Native maze begin failed");GA::ProcessQueue(state);auto mazeTarget=*M::Target();auto* mazeRide=GetRide(mazeTarget.id);
        auto mazeGhosts=[&](){int n=0;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32)for(const auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))n+=e->isGhost() && e->getRideIndex()==mazeTarget.id;return n;};
        auto cells=[&](bool ghost){std::vector<std::tuple<int,int,int,uint16_t>> result;for(int y=0;y<2048;y+=32)for(int x=0;x<2048;x+=32){
            for(const auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))if(e->isGhost()==ghost && e->getRideIndex()==mazeTarget.id && e->getTrackType()==TrackElemType::maze)result.emplace_back(x,y,e->getBaseZ(),e->getMazeEntry());}return result;};
        const auto emptyMap=map();const auto emptyCash=state.park.cash;const auto emptyStation=mazeRide->getStation();
        M::Update(CoordsXY{960,960});auto initialQuote=M::Build(false);const auto initialProposal=cells(true);
        Check(initialQuote.error==GA::Status::ok && mazeGhosts()==1 && mazeRide->mazeTiles==0 && map()==emptyMap && state.park.cash==emptyCash && std::memcmp(&emptyStation,&mazeRide->getStation(),sizeof(emptyStation))==0,"Initial native maze ghost changed real map/station/cash/count");
        GA::MazeSetTrackAction quoted(*M::Cursor(),true,mazeTarget.id,GA::MazeBuildMode::build);
        const auto initialContext=AccessPlacementContext::Capture(*mazeRide,*M::Cursor());
        quoted.GuardContext(initialContext,initialQuote.cost+1);Check(quoted.Query(state,state.park).error!=GA::Status::ok,"Changed maze quote accepted");
        quoted.GuardContext(initialContext,initialQuote.cost);DataSerialiser mazeEncoded(true);quoted.Serialise(mazeEncoded);mazeEncoded.getStream().SetPosition(0);
        GA::MazeSetTrackAction mazeDecoded;DataSerialiser mazeDecode(false,mazeEncoded.getStream());mazeDecoded.Serialise(mazeDecode);
        Check(mazeDecoded.Query(state,state.park).error==GA::Status::ok && map()==emptyMap,"Maze serialized guard/query purity failed");
        GA::CheatSetAction noMoney(CheatType::noMoney,1);Check(GA::Execute(&noMoney,state).error==GA::Status::ok,"Native money-mode fixture failed");GA::ProcessQueue(state);
        Check(mazeDecoded.Query(state,state.park).error!=GA::Status::ok,"Changed money mode accepted old maze quote");
        GA::CheatSetAction restoreMoneyMode(CheatType::noMoney,0);Check(GA::Execute(&restoreMoneyMode,state).error==GA::Status::ok,"Native money-mode restoration failed");GA::ProcessQueue(state);
        Ui::ProvisionalElementsRemove();Ui::ProvisionalElementsRemove();Check(mazeGhosts()==0 && map()==emptyMap,"Nested maze suspension changed real map");Ui::ProvisionalElementsRestore();Check(mazeGhosts()==0,"Maze restored early");Ui::ProvisionalElementsRestore();Check(cells(true)==initialProposal,"Native maze proposal restoration differs");
        M::Update({});Check(mazeGhosts()==0 && map()==emptyMap,"Invalid initial maze target retained ghost");M::Update(CoordsXY{960,960});
        GA::CheatSetAction zeroCash(CheatType::setMoney,0);Check(GA::Execute(&zeroCash,state).error==GA::Status::ok,"Native funds fixture failed");GA::ProcessQueue(state);M::Update(CoordsXY{960,960});
        Check(M::Build(true).error==GA::Status::insufficientFunds && mazeGhosts()==0 && map()==emptyMap && mazeRide->mazeTiles==0,"Failed maze purchase changed map/count or retained ghost");
        GA::CheatSetAction restoreCash(CheatType::setMoney,emptyCash);Check(GA::Execute(&restoreCash,state).error==GA::Status::ok,"Native funds restoration failed");GA::ProcessQueue(state);M::Update(CoordsXY{960,960});

        M::Height(1);Check(mazeGhosts()==1 && M::Cursor()->z==128 && map()==emptyMap,"Native maze height preview failed");M::Height(-1);
        Check(M::Build(true).error==GA::Status::ok && M::Build(true).error!=GA::Status::ok,"Native initial maze apply/repeat failed");GA::ProcessQueue(state);mazeTarget=*M::Target();
        Check(cells(false)==initialProposal && mazeRide->mazeTiles==1 && state.park.cash==emptyCash-initialQuote.cost,"Native initial result differs from ghost/accounting");
        M::Turn(2);auto firstMap=map();const auto firstCash=state.park.cash;const auto secondProposal=cells(true);
        Check(mazeGhosts()==1 && map()==firstMap && mazeRide->mazeTiles==1,"Native within-tile proposal changed real maze");
        auto secondQuote=M::Build(false);Check(M::Build(true).error==GA::Status::ok,"Second half-cell native apply failed");GA::ProcessQueue(state);
        Check(cells(false)==secondProposal && state.park.cash==firstCash-secondQuote.cost,"Native wall algorithm differs between ghost and real half-cell");
        M::Update(CoordsXY{960,960});const auto multiProposal=cells(true);const auto multiMap=map();auto thirdQuote=M::Build(false);
        Check(mazeGhosts()==2 && mazeRide->mazeTiles==1 && map()==multiMap,"New-tile preview changed native count/walls");
        Check(M::Build(true).error==GA::Status::ok,"Native multi-tile build failed");GA::ProcessQueue(state);
        Check(cells(false)==multiProposal && mazeRide->mazeTiles==2 && state.park.cash==firstCash-secondQuote.cost-thirdQuote.cost,"Native neighbor wall result differs from preview");
        M::Turn(2);const auto moveMap=map();const auto moveCash=state.park.cash;Check(M::Move().error==GA::Status::ok,"Native cursor move failed");GA::ProcessQueue(state);
        Check(map()==moveMap && state.park.cash==moveCash,"Native move changed real geometry/accounting");
        Check(M::ReviewRemoval().error==GA::Status::ok,"Native fill preview review missing");const auto fillProposal=cells(true);
        Check(map()==moveMap && M::Confirm().error==GA::Status::ok && !M::Reviewing() && map()==moveMap,"Cancel-first fill changed real maze");
        Check(M::ReviewRemoval().error==GA::Status::ok,"Second native fill review failed");M::ChooseRemoval();Check(M::Confirm().error==GA::Status::ok,"Native fill apply failed");GA::ProcessQueue(state);
        Check(cells(false)==fillProposal && state.park.cash==moveCash,"Native fill ghost/result/cash differs");
        M::Update(CoordsXY{960,960});const auto staleMap=map();const auto staleCash=state.park.cash;auto version=mazeObject->GetVersion();auto changedMaze=version;++std::get<2>(changedMaze);mazeObject->SetVersion(changedMaze);
        Check(M::Build(true).error!=GA::Status::ok && mazeGhosts()==0 && map()==staleMap && state.park.cash==staleCash,"Changed descriptor executed or retained invalid maze ghost");mazeObject->SetVersion(version);
        M::Update(CoordsXY{960,960});const auto beforeSave=map();const auto savedTiles=mazeRide->mazeTiles;const auto mazeSave=std::filesystem::path(argv[2])/"maze-preview-autosave.park";
        Check(ScenarioSave(state,mazeSave.string(),flags)!=0 && mazeGhosts()>0 && map()==beforeSave,"Native maze autosave failed preview restoration");
        M::End();Check(mazeGhosts()==0,"Maze end stranded ghosts");
        Check(context->LoadParkFromFile(mazeSave.string()) && mazeGhosts()==0 && GetRide(mazeTarget.id)->mazeTiles==savedTiles && !M::Active() && map()==beforeSave,"Native save included maze ghosts or changed real walls");
        std::cout<<"MazeAccessPreviewTests access/maze native/SDL PASS\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}
}
