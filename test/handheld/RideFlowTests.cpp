// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include "RideTabNavigation.h"
#include <openrct2-ui/handheld/Mazes.h>
#include <openrct2/management/Research.h>
#include <openrct2/interface/Viewport.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/CustomTracks.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/FlatRides.h>
#include <openrct2-ui/handheld/RideFollow.h>
#include <openrct2-ui/handheld/Resume.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/handheld/Alerts.h>
#include <openrct2-ui/handheld/Landscape.h>
#include <openrct2-ui/handheld/ParkOverview.h>
#include <openrct2-ui/handheld/ShopManagement.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2-ui/handheld/Undo.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <openrct2/ride/Track.h>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
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
        auto pickTrackOption=[&](const char* key){
            const auto entries=C::Pieces();const auto found=std::find_if(entries.begin(),entries.end(),[&](const auto& e){return e.identity.key==key;});
            Check(found!=entries.end(),"Track option missing");const auto index=static_cast<size_t>(found-entries.begin());
            for(size_t i=0;i<entries.size();++i)W::HandheldObjectPickerMove(0,-1);
            W::HandheldObjectPickerMove(-1,0);W::HandheldObjectPickerMove(-1,0);
            for(size_t i=0;i<index/3;++i)W::HandheldObjectPickerMove(0,1);
            for(size_t i=0;i<index%3;++i)W::HandheldObjectPickerMove(1,0);
            press(H::Action::confirm);
        };
        auto load=[&](const char* name,ride_type_t type){auto* o=objects.LoadObject(name);Check(o,"Ride object missing");const auto i=objects.GetLoadedObjectEntryIndex(o);
            ResearchInsertRideEntry(i,true);RideEntrySetInvented(i);RideTypeSetInvented(type);return i;};
        auto flat=load("rct2.ride.mgr1",RIDE_TYPE_MERRY_GO_ROUND);
        Check(H::BeginFlatRide(flat),"Flat start failed");focus({656,656,48});F::UpdatePreview(CoordsXY{640,640});capture("01-flat-placement.png");
        const auto previewCash=state.park.cash;press(H::Action::back);Check(F::CurrentStage()==F::Stage::inactive&&state.park.cash==previewCash,"Unplaced flat cancellation spent cash");Check(H::BeginFlatRide(flat),"Flat restart failed");
        const auto quote=F::Action(CoordsXY{640,640},false);const auto flatCash=state.park.cash;
        Check(quote.error==GameActions::Status::ok&&F::Action(CoordsXY{640,640},true).error==GameActions::Status::ok,"Flat purchase failed");GameActions::ProcessQueue(state);
        const auto target=*F::Target();Check(state.park.cash==flatCash-quote.cost,"Flat native cost changed");
        press(H::Action::openMenu);Check(W::HandheldRidesPresentation().tabs.size()==4,"Construction Start lost modern workspace");
        closeMenu();H::EndTerrain();
        bool hit=false;const auto location=H::RideManagement::Location(target);Check(location.has_value(),"Ride has no native location");
        focus(*location);
        const auto centre=WindowGetMain()->viewport->viewPos;
        for(int y=-48;y<=48&&!hit;y+=4){WindowGetMain()->viewport->viewPos=centre+ScreenCoordsXY{0,y};
            const auto info=GetMapCoordinatesFromPos({256,192},{ViewportInteractionItem::ride});
            hit=info.Element&&info.Element->getRideIndex()==target.id;}
        Check(hit,"Native park-centre ride hit testing failed");const auto camera=WindowGetMain()->viewport->viewPos;
        press(H::Action::confirm);Check(W::HandheldRidesPresentation().tabs.size()==4,"Park A did not open Ride Overview");{const auto m=W::HandheldRidesPresentation();Check(H::MeasureManagement({496,H::Layout::ContentHeight({512,384})},m).capacity>=m.rows.size(),"Overview unnecessarily splits ordinary actions");}capture("02-park-ride-overview.png");
        press(H::Action::nextOption);capture("03-ride-operations.png");
        FocusRideTabAction("Operations...");press(H::Action::confirm);press(H::Action::back);
        Check(W::HandheldRidesPresentation().selectedTab==1,"Operations Back lost tab");
        press(H::Action::back);WindowCullDead();Check(!wm->FindByClass(WindowClass::handheldMenu)&&WindowGetMain()->viewport->viewPos==camera,"Park entry Back lost camera/context");
        W::HandheldRideOpen(target.id);FocusRideTabAction("Place entrance / exit...");const auto placementFocus=W::HandheldRidesPresentation().selected;
        press(H::Action::confirm);Check(F::CurrentStage()==F::Stage::entrance,"Placement handoff failed");capture("04-access-placement.png");
        press(H::Action::back);WindowCullDead();Check(W::HandheldRidesPresentation().tabs.size()==4&&W::HandheldRidesPresentation().selected==placementFocus,"Access cancellation lost focus");
        closeMenu();Check(H::ResumeFlatRide(target.id),"Resume access failed");
        for(int part=0;part<2;++part){bool done=false;for(int y=480;y<=800&&!done;y+=32)for(int x=480;x<=800&&!done;x+=32) {
            if(F::Action(CoordsXY{x,y},false).error==GameActions::Status::ok){Check(F::Action(CoordsXY{x,y},true).error==GameActions::Status::ok,"Access action failed");GameActions::ProcessQueue(state);done=true;} }
            Check(done,"No native access side");}
        H::UpdateExplorer(input);Check(W::HandheldRidesPresentation().tabs.size()==4,"Completion returned to legacy handoff");
        press(H::Action::contextPrimary);Check(W::HandheldRidesPresentation().title=="Entrances / exits","X Access did not open points");capture("05-existing-access.png");press(H::Action::back);
        // Ride state is a switch in the hero, changed in place with Left/Right (no review screen).
        FocusRideTabAction("Status");Check(W::HandheldRidesPresentation().hero&&W::HandheldRidesPresentation().hero->states.back()=="Open","Ride hero has no state switch");
        for(int i=0;i<3;++i){W::HandheldDashboardMove(1,0);GameActions::ProcessQueue(state);}
        Check(GetRide(target.id)->status==RideStatus::open&&!W::HandheldRidesPresentation().confirmation,"In-place Open failed");capture("05a-open-ride.png");
        for(int i=0;i<3;++i){W::HandheldDashboardMove(-1,0);GameActions::ProcessQueue(state);}
        Check(GetRide(target.id)->status==RideStatus::closed,"In-place Close failed");
        press(H::Action::confirm);GameActions::ProcessQueue(state);Check(GetRide(target.id)->status!=RideStatus::closed,"A did not step the state");
        for(int i=0;i<3;++i){W::HandheldDashboardMove(-1,0);GameActions::ProcessQueue(state);}
        FocusRideTabAction("Demolish...");press(H::Action::confirm);Check(W::HandheldRidesPresentation().selected==1,"Demolition review is not Cancel-first");capture("06-demolition-review.png");press(H::Action::back);Check(H::RideManagement::Resolve(target),"Cancel demolished ride");closeMenu();
        {
            // Shops: Open/Closed is the hero switch and each price steps in place with Left/Right.
            auto* object=objects.LoadObject("rct2.ride.burgb");Check(object,"Shop object missing");
            const auto shopObject=objects.GetLoadedObjectEntryIndex(object);
            ResearchInsertRideEntry(shopObject,true);RideEntrySetInvented(shopObject);RideTypeSetInvented(RIDE_TYPE_FOOD_STALL);
            const CoordsXY tile{800,640};
            Check(H::Shops::Action(tile,shopObject,0,true).error==GameActions::Status::ok,"Shop placement failed");GameActions::ProcessQueue(state);
            const auto shop=(*TileElementsView<TrackElement>(tile).begin())->getRideIndex();
            W::HandheldRideOpen(shop);Check(W::HandheldShopManagementIsOpen(),"Shop inspection did not open");
            W::HandheldShopManagementMove(-1,0);GameActions::ProcessQueue(state);
            Check(GetRide(shop)->status==RideStatus::closed,"Shop switch did not close in place");
            W::HandheldShopManagementMove(1,0);GameActions::ProcessQueue(state);
            Check(GetRide(shop)->status==RideStatus::open,"Shop switch did not open in place");
            const auto price=GetRide(shop)->price[0];
            W::HandheldShopManagementMove(0,1);
            W::HandheldShopManagementMove(1,0);GameActions::ProcessQueue(state);W::HandheldShopManagementMove(1,0);GameActions::ProcessQueue(state);
            Check(GetRide(shop)->price[0]==price+2,"Shop price did not step up in place");
            W::HandheldShopManagementMove(-1,0);GameActions::ProcessQueue(state);
            Check(GetRide(shop)->price[0]==price+1,"Shop price did not step down in place");
            focus({816,656,48});capture("06d-shop-prices.png");
            W::HandheldShopManagementMove(0,1);W::HandheldMenuConfirm();
            Check(W::HandheldShopManagementIsOpen()&&GetRide(shop),"Shop grid Demolish did not open its review");
            capture("06e-shop-demolish.png");W::HandheldMenuBack();closeMenu();
        }
        {
            // Context A: on a path end (one connected edge), A starts the connected path tool there.
            auto* surface=objects.LoadObject("rct2.footpath_surface.tarmac");auto* railing=objects.LoadObject("rct2.footpath_railings.wood");
            Check(surface&&railing,"Path objects missing");
            for(const CoordsXY t:{CoordsXY{1120,800},CoordsXY{1152,800}})
            {
                GameActions::FootpathPlaceAction a({t,MapGetSurfaceElementAt(t)->getBaseZ()},FootpathSlope{},
                    objects.GetLoadedObjectEntryIndex(surface),objects.GetLoadedObjectEntryIndex(railing));
                Check(GameActions::Execute(&a,state).error==GameActions::Status::ok,"Context path fixture failed");
                GameActions::ProcessQueue(state);
            }
            const CoordsXY end{1152,800};
            focus({end.x+16,end.y+16,MapGetSurfaceElementAt(end)->getBaseZ()});H::UpdateExplorer(input);
            capture("06f-path-end.png");
            press(H::Action::confirm);
            Check(H::Paths::IsActive()&&H::Paths::IsConnected()&&H::Paths::ConnectedReady(),"A on a path end did not start extending it");
            H::EndTerrain();
            Check(!H::Paths::IsActive(),"Extend path tool did not end");
        }
        {
            // Auto access: Y while placing the entrance puts the entrance and exit beside the ride
            // and lays a queue and an exit path to the nearest footpath.
            auto* surface=objects.LoadObject("rct2.footpath_surface.tarmac");auto* queue=objects.LoadObject("rct2.footpath_surface.queue_blue");
            auto* railing=objects.LoadObject("rct2.footpath_railings.wood");Check(surface&&queue&&railing,"Auto access path objects missing");
            for(int x=1376;x<=1856;x+=32)
            {
                const CoordsXY t{x,1408};
                GameActions::FootpathPlaceAction a({t,MapGetSurfaceElementAt(t)->getBaseZ()},FootpathSlope{},
                    objects.GetLoadedObjectEntryIndex(surface),objects.GetLoadedObjectEntryIndex(railing));
                Check(GameActions::Execute(&a,state).error==GameActions::Status::ok,"Auto access path fixture failed");
            }
            GameActions::ProcessQueue(state);
            Check(H::BeginFlatRide(flat),"Auto access ride start failed");
            Check(F::Action(CoordsXY{1600,1600},true).error==GameActions::Status::ok,"Auto access ride placement failed");GameActions::ProcessQueue(state);
            const auto autoRide=*F::Target();Check(F::CurrentStage()==F::Stage::entrance,"Auto access ride did not ask for an entrance");
            focus({1616,1616,MapGetSurfaceElementAt(CoordsXY{1600,1600})->getBaseZ()});H::UpdateExplorer(input);capture("06g-auto-access.png");
            const auto autoCash=state.park.cash;
            press(H::Action::contextSecondary);Check(F::Busy(),"Y did not start auto access");
            // Two passes: the exit and its path are queued once the entrance exists.
            GameActions::ProcessQueue(state);GameActions::ProcessQueue(state);
            const auto& autoStation=GetRide(autoRide.id)->getStation(StationIndex::FromUnderlying(0));
            Check(!autoStation.entrance.isNull()&&!autoStation.exit.isNull(),"Auto access did not place both entrance and exit");
            const auto access=H::RideAccess::Inspect(autoRide);
            Check(access&&access->entrance.reachesNormalPath&&access->exit.reachesNormalPath,"Auto access did not join both to the path");
            Check(state.park.cash<autoCash,"Auto access was free");
            H::UpdateExplorer(input);
            const auto overview=W::HandheldRidesPresentation();
            Check(overview.tabs.size()==4&&overview.status.find("joined to the path")!=std::string::npos,"Auto access did not report back on the ride");
            capture("06h-auto-access-done.png");closeMenu();focus({1616,1520,MapGetSurfaceElementAt(CoordsXY{1600,1600})->getBaseZ()});capture("06i-auto-access-world.png");
            // Ride along: once the ride has trains, the overview offers it; the camera follows the
            // train zoomed in, B returns to the ride, and panning ends it and gives the zoom back.
            Check(!H::RideFollow::Available(autoRide.id),"Ride along offered before the ride has trains");
            {GameActions::RideSetStatusAction open(autoRide.id,RideStatus::testing);GameActions::Execute(&open,state);GameActions::ProcessQueue(state);}
            Check(H::RideFollow::Available(autoRide.id),"Testing ride has no train to ride along");
            auto* main=WindowGetMain();main->viewport->zoom=ZoomLevel{2};const auto zoomBefore=main->viewport->zoom;
            W::HandheldRideOpen(autoRide.id);FocusRideTabAction("Ride along");press(H::Action::confirm);WindowCullDead();
            Check(!wm->FindByClass(WindowClass::handheldMenu)&&H::RideFollow::Current()&&main->viewportSmartFollowSprite==H::RideFollow::Current()->vehicle,
                "Ride along did not follow the train");
            Check(main->viewport->zoom==ZoomLevel{0},"Ride along did not zoom in");
            H::UpdateExplorer(input);
            press(H::Action::back);Check(W::HandheldRidesPresentation().tabs.size()==4,"B while riding did not return to the ride");closeMenu();
            H::Frame panAway;panAway.panX=8;H::HandleExplorerInput(input,panAway);H::UpdateExplorer(input);
            Check(!H::RideFollow::Current()&&main->viewport->zoom==zoomBefore,"Panning did not end ride along and restore the zoom");
            {GameActions::RideSetStatusAction close(autoRide.id,RideStatus::closed);GameActions::Execute(&close,state);GameActions::ProcessQueue(state);}
        }
        {
            // Attention: a broken ride becomes the first alert, Y glides the camera to it, and the
            // overview home leads with it. With no problems, Y is News as before.
            auto* ride=GetRide(target.id);ride->flags.set(RideFlag::brokenDown);H::Alerts::Invalidate();
            const auto& alerts=H::Alerts::Current();
            Check(!alerts.empty()&&alerts.front().kind==H::Alerts::Kind::breakdown&&alerts.front().ride==target.id.ToUnderlying(),"Broken ride is not the first alert");
            Check(H::Alerts::HintLabel().starts_with("Alerts ("),"Footer does not advertise alerts");
            auto* main=WindowGetMain();main->savedViewPos={};main->flags.unset(WindowFlag::scrollingToLocation);
            press(H::Action::contextSecondary);{H::Frame release;H::HandleExplorerInput(input,release);} // Tap: decided on release.
            Check(main->flags.has(WindowFlag::scrollingToLocation)&&main->savedViewPos!=ScreenCoordsXY{},"Y did not glide to the alert");
            main->viewport->viewPos=main->savedViewPos;main->flags.unset(WindowFlag::scrollingToLocation);
            capture("06a-alert-jump.png");
            const auto home=H::ParkOverview::Presentation(0);
            Check(home.bodyLines.size()>=2&&home.bodyLines.front().find("broke down")!=std::string::npos,"Overview does not lead with alerts");
            press(H::Action::openMenu);capture("06b-overview-alerts.png");closeMenu();
            {
                // Hold Y: the inbox, alerts above the news; A on an alert glides there.
                press(H::Action::contextSecondary);
                std::this_thread::sleep_for(std::chrono::milliseconds(450));
                H::Frame held;held.secondaryHeld=true;H::HandleExplorerInput(input,held);
                const auto inbox=W::HandheldManagementPresentation();
                Check(inbox.title=="Inbox"&&!inbox.rows.empty()&&inbox.rows.front().find("broke down")!=std::string::npos,"Hold Y did not open the inbox");
                capture("06b2-inbox.png");
                main->savedViewPos={};main->flags.unset(WindowFlag::scrollingToLocation);
                W::HandheldMenuConfirm();WindowCullDead();
                Check(!wm->FindByClass(WindowClass::handheldMenu)&&main->flags.has(WindowFlag::scrollingToLocation),"Inbox A did not glide to the alert");
                {H::Frame release;H::HandleExplorerInput(input,release);}
            }
            ride->flags.unset(RideFlag::brokenDown);H::Alerts::Invalidate();
        }
        {
            // A rendered look at quick landscaping beside the ride: a 3x3 pond and a hill.
            namespace L=H::Landscape;
            H::BeginWater();L::brush=3;
            Check(L::Action(CoordsXY{800,544},true).error==GameActions::Status::ok,"Pond failed beside ride");GameActions::ProcessQueue(state);GameActions::ProcessQueue(state);
            L::shape=L::Shape::hill;Check(L::Action(CoordsXY{544,800},true).error==GameActions::Status::ok,"Hill failed beside ride");GameActions::ProcessQueue(state);
            L::shape=L::Shape::pond;focus({720,600,48});H::UpdateExplorer(input);capture("06c-landscape.png");
            L::brush=1;H::EndTerrain();
        }
        {
            // Undo (hold B): land comes back to its exact heights, a placed shop is demolished.
            namespace L=H::Landscape;
            const CoordsXY hill{1248,1024};
            const auto before=MapGetSurfaceElementAt(hill)->baseHeight;
            H::BeginWater();L::shape=L::Shape::hill;L::brush=3;
            const auto cashBefore=state.park.cash;
            H::Undo::Prepare(H::Undo::Kind::land,hill,8,"land change");
            const auto raised=L::Action(hill,true);
            Check(raised.error==GameActions::Status::ok,"Undo fixture hill failed");GameActions::ProcessQueue(state);
            H::Undo::Commit(raised.cost);
            Check(MapGetSurfaceElementAt(hill)->baseHeight!=before,"Hill did not change the land");
            const auto undone=H::Undo::Perform();GameActions::ProcessQueue(state);
            Check(undone.error==GameActions::Status::ok&&MapGetSurfaceElementAt(hill)->baseHeight==before&&!H::Undo::Available(),
                "Undo did not restore the land");
            Check(state.park.cash==cashBefore,"Undo did not give the money back");
            L::brush=1;H::EndTerrain();
            auto* stall=objects.LoadObject("rct2.ride.drnks");Check(stall,"Stall object missing");
            const auto stallObject=objects.GetLoadedObjectEntryIndex(stall);
            ResearchInsertRideEntry(stallObject,true);RideEntrySetInvented(stallObject);RideTypeSetInvented(RIDE_TYPE_DRINK_STALL);
            const CoordsXY shopTile{1312,1120};
            const auto shopCash=state.park.cash;
            H::Undo::Prepare(H::Undo::Kind::shop,shopTile,1,"shop");
            const auto placed=H::Shops::Action(shopTile,stallObject,0,true);
            Check(placed.error==GameActions::Status::ok,"Undo fixture shop failed");GameActions::ProcessQueue(state);
            H::Undo::Commit(placed.cost);
            auto tracks=[&]{auto view=TileElementsView<TrackElement>(shopTile);return view.begin()!=view.end();};Check(tracks(),"Shop was not placed");
            Check(H::Undo::Perform().error==GameActions::Status::ok,"Undo did not demolish the shop");GameActions::ProcessQueue(state);
            Check(!tracks(),"Shop survived undo");
            Check(state.park.cash==shopCash,"Shop undo did not give the money back");
        }
        const auto wooden=load("rct2.ride.ptct1",RIDE_TYPE_WOODEN_ROLLER_COASTER);
        Check(H::BeginCustomTrack(wooden,RIDE_TYPE_WOODEN_ROLLER_COASTER),"Coaster start failed");GameActions::ProcessQueue(state);C::Update(CoordsXY{1408,640});focus({1424,656,64});
        for(int i=0;i<3;++i){Check(C::Select(TrackElemType::endStation)&&C::Action(true).error==GameActions::Status::ok,"Station action failed");GameActions::ProcessQueue(state);}
        Check(C::Select(TrackElemType::flat)&&C::Action(true).error==GameActions::Status::ok,"Track action failed");GameActions::ProcessQueue(state);
        const auto coaster=*C::Target();C::Update(CoordsXY{1408,640});capture("07-coaster-build.png");
        const auto beforeUndo=state.park.cash;press(H::Action::back);GameActions::ProcessQueue(state);Check(C::Active()&&state.park.cash>beforeUndo,"Native B Undo changed");
        press(H::Action::slopeUp);C::Update(CoordsXY{1408,640});capture("08-coaster-adjust.png");
        {
            using namespace OpenRCT2::TrackMetadata;
            // Steering: one D-pad press is one decision; held repeats never stack turns.
            auto dpad=[&](int x,int y,bool repeat=false){H::Frame f;f.navigateX=x;f.navigateY=y;f.navigateRepeat=repeat;H::HandleExplorerInput(input,f);};
            dpad(0,1);Check(GetTrackElementDescriptor(C::Selection()).definition.pitchEnd==TrackPitch::none,"D-pad down did not level the climb");
            const auto view=WindowGetMain()->savedViewPos;
            dpad(-1,0);Check(C::CurrentTurn()==-1,"D-pad left did not steer left");
            Check(WindowGetMain()->savedViewPos==view,"Steering panned the camera");
            dpad(-1,0,true);Check(C::CurrentTurn()==-1&&C::TurnSize()==1,"Held D-pad repeated a steer");
            const auto normal=C::Selection();
            press(H::Action::nextOption);Check(C::TurnSize()==2&&C::CurrentTurn()==-1,"R1 did not widen the turn");
            press(H::Action::previousOption);press(H::Action::previousOption);Check(C::TurnSize()==0&&C::CurrentTurn()==-1,"L1 did not tighten the turn");
            Check(GetTrackElementDescriptor(C::Selection()).sequenceData.numSequences<=GetTrackElementDescriptor(normal).sequenceData.numSequences,"Tight turn is larger than normal");
            press(H::Action::nextOption);
            C::Update(CoordsXY{1408,640});capture("08a-coaster-steer-left.png");
            dpad(1,0);Check(C::CurrentTurn()==0,"Opposite D-pad did not straighten");
            dpad(1,0);Check(C::CurrentTurn()==1,"D-pad right did not steer right");
            const auto end=C::Endpoint();
            Check(C::Action(true).error==GameActions::Status::ok,"Steered turn did not build");GameActions::ProcessQueue(state);
            Check(C::Endpoint()!=end&&C::CurrentTurn()==1,"Next piece lost the steering intent");
            Check(WindowGetMain()->savedViewPos!=view,"Camera did not follow the track end");
            {auto* m=WindowGetMain();m->viewport->viewPos=m->savedViewPos;m->flags.unset(WindowFlag::scrollingToLocation);} // the glide has finished
            C::Update(CoordsXY{1408,640});capture("08b-coaster-steer-built.png");
            press(H::Action::back);GameActions::ProcessQueue(state);Check(C::Endpoint()==end,"Undo after steering failed");
        }
        press(H::Action::openBuild);Check(W::HandheldObjectPickerIsOpen(),"Piece catalog missing");capture("09-track-pieces.png");
        press(H::Action::openMenu);Check(!C::Active()&&W::HandheldRidesPresentation().tabs.size()==4,"Start from piece catalog failed to return to ride");
        FocusRideTabAction("Continue track...");press(H::Action::confirm);press(H::Action::openBuild);press(H::Action::back);WindowCullDead();
        Check(C::Active(), "Piece Back ended builder");
        const auto cameraPiece = C::Selection();
        const auto cameraEndpoint = C::Endpoint();
        const auto cameraCash = state.park.cash;
        press(H::Action::openBuild);
        press(H::Action::contextPrimary);
        Check(C::CameraMode() && !W::HandheldObjectPickerIsOpen(), "Brick X camera shortcut unavailable");
        const auto beforePan=WindowGetMain()->savedViewPos;
        H::Frame pan;pan.panX=8;H::HandleExplorerInput(input,pan);
        Check(WindowGetMain()->savedViewPos!=beforePan,"Construction camera D-pad did not pan");
        press(H::Action::slopeUp);press(H::Action::contextSecondary);
        capture("09a-track-camera.png");
        press(H::Action::confirm);
        Check(
            !C::CameraMode() && C::Selection() == cameraPiece && C::Endpoint() == cameraEndpoint
                && state.park.cash == cameraCash,
            "Camera A placed track or changed draft");
        press(H::Action::openBuild);
        pickTrackOption("camera");
        Check(C::CameraMode(), "D-pad camera selection failed");
        press(H::Action::back);
        Check(C::Active() && !C::CameraMode(), "Camera Back failed to return to building");
        press(H::Action::openMenu);Check(!C::Active()&&W::HandheldRidesPresentation().tabs.size()==4,"Coaster Start did not return to ride");
        const auto resume=FocusRideTabAction("Continue track...");press(H::Action::confirm);Check(C::Active(),"Continue track failed");press(H::Action::openMenu);
        Check(W::HandheldRidesPresentation().selectedTab==1&&W::HandheldRidesPresentation().selected==resume,"Coaster return lost Operations focus");
        press(H::Action::confirm);press(H::Action::openBuild);pickTrackOption("leave");
        Check(!C::Active()&&W::HandheldRidesPresentation().selectedTab==1,"Picker leave failed to return to ride");closeMenu();
        const auto mazeIndex=load("rct2.ride.hmaze",RIDE_TYPE_MAZE);Check(H::BeginMaze(mazeIndex),"Maze begin failed");GameActions::ProcessQueue(state);M::Update(CoordsXY{960,1280});focus({976,1296,40});
        Check(M::Build(true).error==GameActions::Status::ok,"Maze native cell failed");GameActions::ProcessQueue(state);const auto maze=*M::Target();capture("10-maze-build.png");
        const auto mazeCash=state.park.cash;M::ToggleCamera();const auto mazeCursor=M::Cursor();
        press(H::Action::slopeUp);press(H::Action::contextSecondary);Check(M::Cursor()==mazeCursor&&!M::Reviewing(),"Maze camera edited geometry");
        press(H::Action::confirm);Check(!M::CameraMode()&&state.park.cash==mazeCash,"Maze camera A built a cell");
        M::ToggleCamera();press(H::Action::back);Check(M::Active()&&!M::CameraMode(),"Maze camera Back left builder");
        press(H::Action::back);Check(!M::Active()&&W::HandheldRidesPresentation().tabs.size()==4&&H::RideManagement::Resolve(maze)&&state.park.cash==mazeCash,"Maze leave changed native state");closeMenu();
        W::HandheldDashboardOpen();while(W::HandheldDashboardSectionIndex()!=1)W::HandheldDashboardMove(0,1);W::HandheldDashboardMove(1,0);
        const auto roster=W::HandheldRidesPresentation();const auto row=std::find(roster.rows.begin(),roster.rows.end(),GetRide(coaster.id)->getName());Check(row!=roster.rows.end(),"Coaster missing from roster");
        const auto rosterRow=static_cast<size_t>(row-roster.rows.begin());W::HandheldMenuMove(static_cast<int>(rosterRow)-static_cast<int>(roster.selected));press(H::Action::confirm);
        FocusRideTabAction("Continue track...");press(H::Action::confirm);press(H::Action::openMenu);press(H::Action::back);
        Check(W::HandheldRidesPresentation().selected==rosterRow&&W::HandheldRidesPresentation().tabs.empty(),"Roster return lost original row");capture("11-roster-return.png");
        {
            // Save when the device sleeps: the resume save is written atomically and dropped again
            // when the player deliberately quits without saving.
            Check(H::Resume::Wanted()&&H::Resume::Save()&&std::filesystem::exists(H::Resume::Path()),"Resume save was not written");
            SDL_Event power{};power.type=SDL_KEYDOWN;power.key.keysym.scancode=SDL_SCANCODE_POWER;
            const auto written=std::filesystem::last_write_time(H::Resume::Path());H::Resume::OnEvent(power);
            Check(std::filesystem::last_write_time(H::Resume::Path())==written,"Power key saved again inside the throttle window");
            H::Resume::Discard();Check(!std::filesystem::exists(H::Resume::Path()),"Discard kept the resume save");
        }
        std::cout<<"PASS native park selection / modern tabs / contextual access / camera preservation / safe status and demolition reviews\n";
        std::cout<<"PASS coaster Undo/piece/camera/return, flat access cancellation/completion, maze camera/leave, roster focus restoration\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

