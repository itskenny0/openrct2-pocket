// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/handheld/PeopleNavigation.h>
#include <openrct2/management/NewsItem.h>
#include <openrct2-ui/handheld/ParkInformation.h>
#include <openrct2-ui/handheld/ManagementNews.h>
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
        auto capture=[&](const char* name){const auto* folder=std::getenv("POCKET_RIDE_FLOW_CAPTURES");if(!folder||argc!=4)return;
            std::filesystem::create_directories(folder);Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();
            auto& rt=*engine.getRT();ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,WindowGetMain()->viewport);
            if(auto* window=wm->FindByClass(WindowClass::handheldMenu))window->onDraw(rt);else H::DrawExplorer(rt);
            Check(H::FooterHeight({ 512, 384 }, H::LastControlHints) == 28, "Runtime footer expanded beyond one row");
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;
            const auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(folder)/name).string(),image,ImageFormat::png);engine.EndDraw();};
        auto load=[&](const char* name,ride_type_t type){auto* o=objects.LoadObject(name);Check(o,"Ride object missing");const auto i=objects.GetLoadedObjectEntryIndex(o);
            ResearchInsertRideEntry(i,true);RideEntrySetInvented(i);RideTypeSetInvented(type);return i;};

        const auto wooden=load("rct2.ride.ptct1",RIDE_TYPE_WOODEN_ROLLER_COASTER);
        struct Track { int x,y,z,d,type,sequence; bool chain,inverted; bool operator==(const Track&)const=default; };
        auto tracks=[&](bool ghost){std::vector<Track> result;
            for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)
                for(auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))if(e->isGhost()==ghost)
                    result.push_back({x,y,e->getBaseZ(),e->getDirection(),static_cast<int>(e->getTrackType()),e->getSequenceIndex(),e->hasChain(),e->isInverted()});
            return result;};
        Check(H::BeginCustomTrack(wooden,RIDE_TYPE_WOODEN_ROLLER_COASTER),"Coaster begin failed");GameActions::ProcessQueue(state);
        focus({1424,1424,48});C::Update(CoordsXY{1408,1408});
        auto ghost=tracks(true);auto cash=state.park.cash;
        Check(C::HasPreview()&&!ghost.empty(),"Native station ghost absent");
        Ui::ProvisionalElementsRemove();Check(tracks(true).empty(),"Ghost not suspended");
        Ui::ProvisionalElementsRestore();Check(tracks(true)==ghost,"Provisional restore lost native ghost");
        gameStateUpdateLogic();Check(tracks(true)==ghost&&tracks(false).empty()&&cash==state.park.cash,"Native tick changed ghost/map/accounting");
        C::Slope(1);C::Update(CoordsXY{1408,1408});auto raised=tracks(true);
        Check(!raised.empty()&&raised.front().z==ghost.front().z+kLandHeightStep&&state.park.cash==cash,"Station height preview changed cash or wrong geometry");
        C::Slope(-1);C::Rotate();C::Update(CoordsXY{1408,1408});Check(!tracks(true).empty()&&tracks(true).front().d!=ghost.front().d,"Direction preview did not update");
        for(int i=0;i<3;++i){C::Rotate();}C::Update(CoordsXY{1408,1408});
        Check(tracks(true)==ghost,"Station rotation did not restore geometry");capture("01-station-ghost.png");
        auto build=[&](TrackElemType type){Check(C::Select(type),"Native piece unavailable");C::Update(CoordsXY{1408,1408});
            const auto quote=C::Action(false);auto preview=tracks(true);const auto before=tracks(false);auto balance=state.park.cash;
            Check(quote.error==GameActions::Status::ok&&C::HasPreview()&&!preview.empty(),"Valid piece has no native ghost");
            Check(C::Action(true).error==GameActions::Status::ok,"Native piece placement failed");GameActions::ProcessQueue(state);
            const auto after=tracks(false);Check(after.size()>before.size(),"No real track placed");
            for(const auto& t:preview)Check(std::any_of(after.begin(),after.end(),[&](const auto& placed){return t.x==placed.x&&t.y==placed.y&&t.z==placed.z&&t.d==placed.d&&t.sequence==placed.sequence;}),"Ghost differs from real geometry");
            Check(state.park.cash==balance-quote.cost,"Native coaster cost mismatch");C::Update(CoordsXY{1408,1408});};
        build(TrackElemType::endStation);build(TrackElemType::endStation);build(TrackElemType::flat);
        Check(C::Select(TrackElemType::flatToUp25),"Slope unavailable");C::Chain();C::Update(CoordsXY{1408,1408});
        auto liftGhost=tracks(true);Check(std::any_of(liftGhost.begin(),liftGhost.end(),[](const auto& t){return t.chain;}),"Native lift ghost lost chain");capture("02-lift-ghost.png");
        Check(!tracks(true).empty(),"Slope missing");
        build(TrackElemType::flatToUp25);build(TrackElemType::up25);build(TrackElemType::up25ToFlat);
        C::Chain();build(TrackElemType::flatToDown25);build(TrackElemType::down25ToFlat);
        Check(C::Select(TrackElemType::leftQuarterTurn3Tiles),"Turn unavailable");C::Update(CoordsXY{1408,1408});capture("03-turn-ghost.png");
        build(TrackElemType::leftQuarterTurn3Tiles);build(TrackElemType::rightQuarterTurn3Tiles);
        build(TrackElemType::flatToLeftBank);Check(C::Select(TrackElemType::leftBankToFlat),"Bank unavailable");C::Update(CoordsXY{1408,1408});capture("04-bank-ghost.png");
        build(TrackElemType::leftBankToFlat);
        const auto real=tracks(false);cash=state.park.cash;
        W::HandheldTrackPiecePickerOpen();capture("05-native-catalog.png");
        press(H::Action::contextSecondary);Check(C::CurrentGroup()!=C::PieceGroup::all,"Catalog group did not change");
        capture("06-filtered-catalog.png");press(H::Action::back);
        Check(tracks(false)==real&&state.park.cash==cash,"Catalog mutated live park");
        C::ClearPreview();Check(tracks(true).empty(),"Cancel left ghosts");C::Update(CoordsXY{1408,1408});Check(C::HasPreview(),"Preview did not return");
        const auto endpoint=C::Endpoint();Check(endpoint.has_value(),"Endpoint absent");
        auto* surface=MapGetSurfaceElementAt(CoordsXY{endpoint->x,endpoint->y});const auto ownership=surface->getOwnership();surface->setOwnership({});
        C::Update(CoordsXY{1408,1408});Check(C::Action(false).error!=GameActions::Status::ok&&!C::HasPreview()&&state.park.cash==cash,"Invalid preview changed cash or retained ghost");
        capture("07-invalid-placement.png");surface->setOwnership(ownership);C::Update(CoordsXY{1408,1408});
        Check(C::Undo().error==GameActions::Status::ok,"Undo failed");GameActions::ProcessQueue(state);
        auto ride=*C::Target();C::Update(CoordsXY{1408,1408});
        auto savedGeometry=tracks(false);const auto savedCash=state.park.cash;
        const auto save=(std::filesystem::path(argv[2])/"rc4-ghost-save.park").string();
        Check(C::HasPreview(),"Save fixture has no ghost");
        RideId provisionalRide=RideId::GetNull();
        for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)
            for(auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))if(e->isGhost())provisionalRide=e->getRideIndex();
        Check(!provisionalRide.IsNull()&&provisionalRide!=ride.id,"Native provisional ride record absent");
        Check(ScenarioSave(state,save,{}),"Ghost-active native save failed");
        Check(C::HasPreview()&&tracks(false)==savedGeometry&&state.park.cash==savedCash,"Save did not restore ghost or changed live state");
        Check(context->LoadParkFromFile(save),"Ghost-active save reload failed");
        Check(tracks(true).empty()&&tracks(false)==savedGeometry&&state.park.cash==savedCash,"Ghost leaked into save or real track/accounting changed");
        Check(GetRide(provisionalRide)==nullptr,"Provisional ride leaked into serialized park");
        InputSetState(InputState::normal);wm->CloseByClass(WindowClass::progressWindow);
        H::EndTerrain();ride=*H::RideManagement::Identify(ride.id);Check(tracks(true).empty(),"End left ghost");
        W::HandheldRideOpen(ride.id);
        auto model=W::HandheldRidesPresentation();
        while(model.selectedTab!=1){press(H::Action::nextOption);model=W::HandheldRidesPresentation();}
        auto row=std::find(model.rows.begin(),model.rows.end(),"Edit track...");Check(row!=model.rows.end(),"Track editor action missing");
        W::HandheldMenuMove(static_cast<int>(row-model.rows.begin())-static_cast<int>(model.selected));press(H::Action::confirm);
        auto before=H::TrackEditing::Blocks(ride);const auto oldCash=state.park.cash;
        capture("08-spatial-track-selection.png");
        for(int y=32;y<2016;y+=32)for(int x=32;x<2016;x+=32)
            for(auto* e:TileElementsView<TrackElement>(CoordsXY{x,y}))Check(!e->isHighlighted(),"Drawing left native track highlighted");
        Check(H::TrackEditing::Blocks(ride)==before&&state.park.cash==oldCash,"Highlight draw mutated track or cash");
        size_t start=0;while(start<before.size()&&!H::TrackEditing::Next(before[start]))++start;
        Check(start<before.size(),"All native connections missing");
        W::HandheldTrackSelectionMove(0,static_cast<int>(start));
        const auto connected=H::TrackEditing::Next(before[start]);Check(connected.has_value(),"Next native connection missing");
        Check(H::TrackEditing::Previous(*connected)==before[start],"Native predecessor mismatch");
        press(H::Action::nextOption);
        const auto selected=W::HandheldRidesPresentation().selected;Check(before[selected]==*connected,"Editor followed map order instead of track");
        press(H::Action::confirm);capture("09-spatial-track-actions.png");press(H::Action::back);
        Check(W::HandheldRidesPresentation().selected==selected,"Back lost selected piece");press(H::Action::back);closeMenu();
        auto event=[&](News::ItemType type,const char* text,uint32_t assoc){closeMenu();News::InitQueue(state.park);News::AddItemToQueue(state.park.newsItems,type,text,assoc);};
        event(News::ItemType::ride,"The ride has broken down",ride.id.ToUnderlying());capture("10-native-news-hud.png");
        press(H::Action::contextSecondary);auto detail=W::HandheldParkMapSubjectsPresentation();
        Check(detail.selected==2&&detail.rows[2]=="Open ride management","News does not focus native ride action");capture("11-ride-event.png");
        press(H::Action::confirm);Check(W::HandheldRidesPresentation().title==H::RideManagement::Resolve(ride)->getName(),"Wrong news ride");capture("12-news-ride-management.png");
        press(H::Action::back);press(H::Action::back);WindowCullDead();Check(!wm->FindByClass(WindowClass::handheldMenu),"News Back failed to return to park");
        Check(!state.park.newsItems.getRecent().empty(),"Back dismissed event");
        event(News::ItemType::money,"The park is running low on cash",0);press(H::Action::contextSecondary);press(H::Action::confirm);
        Check(W::HandheldFinancesIsOpen(),"Money event did not reach finance");press(H::Action::back);press(H::Action::back);
        event(News::ItemType::blank,"Information without a subject",0);press(H::Action::contextSecondary);detail=W::HandheldParkMapSubjectsPresentation();
        Check(detail.selected==0&&detail.rows[2]=="Subject unavailable","Informational event invented target");
        W::HandheldMenuMove(4);press(H::Action::confirm);Check(state.park.newsItems.getRecent().empty()&&state.park.newsItems.getArchived().size()==1,"Dismiss did not use native archive");
        capture("13-news-history.png");press(H::Action::back);
        W::HandheldDashboardOpen();const auto section=W::HandheldDashboardSectionIndex();press(H::Action::contextPrimary);
        Check(W::HandheldParkMapSubjectsPresentation().breadcrumb=="Overview / Native news","Dashboard X did not open news");
        press(H::Action::back);Check(W::HandheldDashboardIsOpen()&&W::HandheldDashboardSectionIndex()==section,"News Back lost dashboard context");closeMenu();
        for(auto type:{News::ItemType::peeps,News::ItemType::graph,News::ItemType::award,News::ItemType::campaign}) {
            event(type,"Native park event",0);const auto target=H::ParkInformation::NewsList().front();
            Check(H::ManagementNews::Available(target)!=H::ManagementNews::Link::none,"Native category handoff absent");
            press(H::Action::contextSecondary);press(H::Action::confirm);press(H::Action::back);
            Check(W::HandheldParkMapSubjectsPresentation().breadcrumb=="Overview / Park news","Category handoff Back lost news");press(H::Action::back);
        }
        const auto invented=std::find_if(state.researchItemsInvented.begin(),state.researchItemsInvented.end(),[&](const auto& item){return item.type==Research::EntryType::ride&&item.entryIndex==wooden;});
        Check(invented!=state.researchItemsInvented.end(),"Native invented ride reference missing");
        event(News::ItemType::research,"New ride researched",invented->rawValue);press(H::Action::contextSecondary);
        Check(W::HandheldParkMapSubjectsPresentation().rows[2]=="Inspect invention","Research news has no native invention target");
        press(H::Action::confirm);press(H::Action::back);
        Check(W::HandheldParkMapSubjectsPresentation().breadcrumb=="Overview / Park news","Research news return failed");press(H::Action::back);
        Check(H::ResumeCustomTrack(ride.id),"Construction resume failed");const auto draft=C::Endpoint();const auto choice=C::Selection();const auto balance=state.park.cash;
        W::HandheldDashboardOpen();press(H::Action::contextPrimary);press(H::Action::back);press(H::Action::back);
        Check(C::Active()&&C::Endpoint()==draft&&C::Selection()==choice&&state.park.cash==balance,"News navigation lost construction draft");H::EndTerrain();
        event(News::ItemType::ride,"Stale ride reference",65534);press(H::Action::contextSecondary);
        Check(W::HandheldParkMapSubjectsPresentation().rows[2]=="Subject unavailable","Invalid ride identifier accepted");closeMenu();
        std::cout<<"RC4 native ghost lifecycle/geometry/cost, catalog, spatial editor and news handoffs passed\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
