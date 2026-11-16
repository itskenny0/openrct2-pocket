// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <openrct2-ui/handheld/ChoiceCards.h>
#include <thread>
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

        // These are loaded native sprite families, not generated artwork.
        // Check every animation frame, plus real text rendering when an icon
        // would otherwise consume the space needed for the full action label.
        {
            for(const auto art:{H::TabArt("Thoughts"),H::TabArt("Patrol"),H::TabArt("Details","Park / Guests"),
                H::TabArt("Overview","Park / Finances"),H::TabArt("Objectives"),H::TabArt("Project"),
                H::ActionArt("Pick up..."),H::ActionArt("Tracking: On..."),H::ActionArt("Maintenance..."),
                H::ActionArt("Sweep paths  On"),H::ActionArt("Park news..."),H::ActionArt("Read objectives")})
            {
                Check(art.sprite!=0,"Native menu sprite mapping missing");
                for(uint8_t frame=0;frame<art.frames;++frame) {
                    const auto* sprite=GfxGetG1Element(art.sprite+frame);
                    Check(sprite && sprite->width>0 && sprite->height>0,"Native menu animation frame missing");
                }
            }
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(280,80);engine.BeginDraw();
            auto& rt=*engine.getRT();H::Theme::Refresh();
            H::Fill(rt,{0,0},{280,80},H::Theme::raised);
            H::DrawActionRow(rt,{20,20},{190,28},"Place entrance / exit...","",false);
            std::vector<Drawing::PaletteIndex> label;
            const int textWidth=H::TextWidth("Place entrance / exit...",true);
            for(int y=28;y<44;++y)for(int x=30;x<30+textWidth;++x)label.push_back(rt.bits[y*rt.LineStride()+x]);
            H::Fill(rt,{0,0},{280,80},H::Theme::raised);
            H::DrawStrongText(rt,{30,31},"{WHITE}Place entrance / exit...");
            size_t pixel=0;
            for(int y=28;y<44;++y)for(int x=30;x<30+textWidth;++x)
                Check(label[pixel++]==rt.bits[y*rt.LineStride()+x],"Native artwork shortened or moved a full action label");
            engine.EndDraw();
        }
        // Render compact two-line cards against a sentinel to detect drawing
        // outside the button, including selected and pressed states.
        {
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(180,100);engine.BeginDraw();
            auto& rt=*engine.getRT();
            for(bool focused:{false,true}) for(bool pressed:{false,true})
            for(const char* label:{"Remove scenery","Shops & facilities","Land & rights"}) {
                std::fill_n(rt.bits,rt.LineStride()*rt.height,static_cast<Drawing::PaletteIndex>(255));
                H::MenuFeel feel;feel.Observe("card",0);if(pressed)feel.Press();H::MenuFeelScope motion(feel);
                H::DrawChoiceCard(rt,{20,20},{112,57},{0,label,"",H::PanelIcon::remove},focused);
                int secondLineInk=0;
                for(int y=0;y<rt.height;++y)for(int x=0;x<rt.width;++x) {
                    const auto pixel=static_cast<uint8_t>(rt.bits[y*rt.LineStride()+x]);
                    if(x<20||x>=132||y<20||y>=77)Check(pixel==255,"Choice card drew outside its bounds");
                    if(x>=28&&x<124&&y>=60&&y<74 && pixel==(focused?H::PocketPalette::dark:H::PocketPalette::primary))++secondLineInk;
                }
                if(std::string(label)!="Land & rights") Check(secondLineInk>=8,"Compact card lost its second label line");
            }
            engine.EndDraw();
        }
        // Visit every Build card through controller navigation, without executing it.
        W::HandheldBuildOpen();
        int buildIndex=0;
        for(int row=0;row<4;++row){
            for(int col=0;col<(row==3?2:4);++col){
                const auto filename="00-build-"+std::to_string(buildIndex++)+".png";capture(filename.c_str());
                H::Frame frame;frame.navigateX=1;H::HandleExplorerInput(input,frame);
            }
            H::Frame frame;frame.navigateY=1;H::HandleExplorerInput(input,frame);
            for(int i=0;i<3;++i){H::Frame left;left.navigateX=-1;H::HandleExplorerInput(input,left);}
        }
        closeMenu();
        auto move=[&](int x,int y){H::Frame frame;frame.navigateX=x;frame.navigateY=y;H::HandleExplorerInput(input,frame);};
        W::HandheldBuildOpen();move(1,0);move(1,0);press(H::Action::confirm);
        capture("00-path-categories.png");closeMenu();
        W::HandheldBuildOpen();move(0,1);move(0,1);move(1,0);move(1,0);press(H::Action::confirm);
        capture("00-scenery-categories.png");closeMenu();

        auto load=[&](const char* name,ride_type_t type){auto* o=objects.LoadObject(name);Check(o,"Ride object missing");const auto i=objects.GetLoadedObjectEntryIndex(o);
            ResearchInsertRideEntry(i,true);RideEntrySetInvented(i);RideTypeSetInvented(type);return i;};
        const auto flat=load("rct2.ride.mgr1",RIDE_TYPE_MERRY_GO_ROUND);
        Check(H::BeginFlatRide(flat),"Flat start failed");
        Check(F::Action(CoordsXY{640,640},true).error==GameActions::Status::ok,"Native flat placement failed");GameActions::ProcessQueue(state);
        const auto target=*F::Target();H::EndTerrain();state.park.flags.set(ParkFlag::unlockAllPrices);
        W::HandheldRideOpen(target.id);FocusRideTabAction("Admission price...");press(H::Action::confirm);
        const auto oldPrice=GetRide(target.id)->price[0];const auto oldCash=state.park.cash;
        H::Frame right;right.navigateX=1;H::HandleExplorerInput(input,right);
        Check(GetRide(target.id)->price[0]==oldPrice,"Price draft changed live ride");capture("01-price-editor.png");
        press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(GetRide(target.id)->price[0]==oldPrice+1 && state.park.cash==oldCash,"Direct Apply changed wrong price or cash");
        FocusRideTabAction("Admission price...");press(H::Action::confirm);H::HandleExplorerInput(input,right);press(H::Action::back);
        Check(GetRide(target.id)->price[0]==oldPrice+1 && state.park.cash==oldCash,"Back committed price draft");
        FocusRideTabAction("Admission price...");press(H::Action::confirm);H::HandleExplorerInput(input,right);
        GameActions::RideSetPriceAction external(target.id,oldPrice+3,true);Check(GameActions::Execute(&external,state).error==GameActions::Status::ok,"External price update failed");GameActions::ProcessQueue(state);
        press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(GetRide(target.id)->price[0]==oldPrice+3 && state.park.cash==oldCash && W::HandheldRidesPresentation().status!="Left/Right: Adjust","Stale editor overwrote state or hid feedback");closeMenu();
        Check(H::ResumeFlatRide(target.id),"Resume access failed");
        for(int part=0;part<2;++part){bool done=false;for(int y=480;y<=800&&!done;y+=32)for(int x=480;x<=800&&!done;x+=32) {
            if(F::Action(CoordsXY{x,y},false).error==GameActions::Status::ok){Check(F::Action(CoordsXY{x,y},true).error==GameActions::Status::ok,"Access failed");GameActions::ProcessQueue(state);done=true;} }
            Check(done,"No valid native access");}
        H::UpdateExplorer(input);press(H::Action::contextPrimary);
        const auto flags=gMapSelectFlags;const auto type=gMapSelectType;const auto a=gMapSelectPositionA,b=gMapSelectPositionB;const auto accessCash=state.park.cash;
        capture("02-spatial-access.png");
        Check(gMapSelectFlags==flags && gMapSelectType==type && gMapSelectPositionA==a && gMapSelectPositionB==b && state.park.cash==accessCash,"Access preview leaked selection or cash");closeMenu();
        for(const char* id:{"rct2.peep_names.original","rct2.peep_animations.guest"})Check(objects.LoadObject(id),"Peep animation missing");
        state.park.entrances.push_back({320,320,16,0});
        for(int i=0;i<40;++i){auto* g=Guest::generate({320+i*2,320,16});Check(g,"Guest fixture failed");g->outsideOfPark=false;g->state=PeepState::walking;g->setParkEntryTime(state.currentTicks);g->peepFlags.set(PeepFlag::hasPaidForParkEntry);}
        state.park.numGuestsInPark=H::GuestManagement::Roster().size();focus({336,336,16});
        W::HandheldDashboardOpen();while(W::HandheldDashboardSectionIndex()!=2)W::HandheldDashboardMove(0,1);W::HandheldDashboardMove(1,0);press(H::Action::confirm);capture("03-roster-start.png");
        const auto startRow=W::HandheldGuestsPresentation().selected;
        H::Frame down;down.navigateY=1;
        for(int i=0;i<12;++i){H::HandleExplorerInput(input,down);std::this_thread::sleep_for(std::chrono::milliseconds(55));capture(("scroll-"+std::to_string(i)+".png").c_str());}
        const auto selected=W::HandheldGuestsPresentation().selected;
        Check(selected==startRow+12,"Scrolling lost input or selected wrong guest");
        const auto movingRoster=W::HandheldGuestsPresentation();
        for(size_t i=selected-5;i<=selected+2;++i)Check(!movingRoster.rows[i].empty(),"Moving viewport contains unprepared rows");
        for(int i=0;i<10;++i){std::this_thread::sleep_for(std::chrono::milliseconds(16));capture(("motion-"+std::to_string(i)+".png").c_str());}
        capture("04-roster-scrolled.png");
        for(int i=0;i<100;++i)H::HandleExplorerInput(input,down);
        Check(W::HandheldGuestsPresentation().selected==W::HandheldGuestsPresentation().rows.size()-1,"Held scrolling wrapped at bottom");capture("05-roster-bottom.png");
        closeMenu();namespace S=H::StaffManagement;
        Check(objects.LoadObject("rct2.peep_animations.handyman"),"Staff animation missing");
        Check(S::Hire(StaffType::handyman,true).error==GameActions::Status::ok,"Native hire failed");GameActions::ProcessQueue(state);
        const auto employee=S::Roster(StaffType::handyman).front();W::HandheldStaffOpen(employee);
        auto selectStaff=[&](const char* label){for(int tab=0;tab<4;++tab){const auto m=W::HandheldStaffPresentation();const auto row=std::find(m.rows.begin(),m.rows.end(),label);
            if(row!=m.rows.end()){W::HandheldMenuMove(static_cast<int>(row-m.rows.begin())-static_cast<int>(m.selected));return;}press(H::Action::nextOption);}throw std::runtime_error(label);};
        selectStaff("Uniform colour...");press(H::Action::confirm);const auto uniform=StaffGetColour(StaffType::handyman);const auto uniformCash=state.park.cash;
        H::HandleExplorerInput(input,right);capture("06-uniform-grid.png");Check(StaffGetColour(StaffType::handyman)==uniform,"Palette draft applied early");
        press(H::Action::back);Check(StaffGetColour(StaffType::handyman)==uniform,"Palette Back applied draft");
        selectStaff("Uniform colour...");press(H::Action::confirm);H::Frame colourDown;colourDown.navigateY=1;H::HandleExplorerInput(input,colourDown);press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(StaffGetColour(StaffType::handyman)!=uniform && state.park.cash==uniformCash,"Uniform Apply failed or charged cash");
        closeMenu();
        auto rideModel=[](){return W::HandheldRidesPresentation();};
        auto select=[&](auto model,const char* label) { const auto m=model();const auto it=std::find(m.rows.begin(),m.rows.end(),label);
            Check(it!=m.rows.end(),label);W::HandheldMenuMove(static_cast<int>(it-m.rows.begin())-static_cast<int>(m.selected)); };
        W::HandheldRideOpen(target.id);FocusRideTabAction("Rename...");press(H::Action::confirm);
        select(rideModel,"Clear");press(H::Action::confirm);select(rideModel,"B");press(H::Action::confirm);
        capture("08-name-keyboard.png");select(rideModel,"Done");press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(GetRide(target.id)->getName()=="B" && !rideModel().confirmation,"Done did not apply ride name directly");
        FocusRideTabAction("Rename...");press(H::Action::confirm);select(rideModel,"C");press(H::Action::confirm);press(H::Action::back);
        Check(GetRide(target.id)->getName()=="B","Keyboard B changed native name");closeMenu();
        W::HandheldStaffOpen({});W::HandheldMenuMove(-100);press(H::Action::confirm);capture("09-hire-roles.png");closeMenu();
        for(const char* id:{"rct2.peep_animations.entertainer_elephant","rct2.peep_animations.entertainer_tiger"})Check(objects.LoadObject(id),"Entertainer animation absent");
        Check(S::Hire(StaffType::entertainer,true).error==GameActions::Status::ok,"Entertainer hire failed");GameActions::ProcessQueue(state);
        const auto entertainer=S::Roster(StaffType::entertainer).front();W::HandheldStaffOpen(entertainer);selectStaff("Costume...");press(H::Action::confirm);capture("11-costumes.png");press(H::Action::back);closeMenu();
        Check(H::BeginStaffPatrol(employee),"Patrol start failed");H::Frame increase;increase.actions={H::Action::slopeUp};H::HandleExplorerInput(input,increase);
        Check(H::StaffPatrol::settings.width==2 && H::StaffPatrol::settings.height==2,"Patrol brush shortcuts failed");capture("12-patrol-brush.png");
        H::EndTerrain();W::HandheldDashboardOpen();W::HandheldDashboardMove(0,5-static_cast<int>(W::HandheldDashboardSectionIndex()));press(H::Action::confirm);
        select([](){return W::HandheldManagementPresentation();},"Invention catalog...");press(H::Action::confirm);capture("13-research-catalog.png");
        const auto cashBeforeCatalog=state.park.cash;press(H::Action::contextSecondary);capture("14-research-filter.png");
        Check(state.park.cash==cashBeforeCatalog,"Catalog browsing spent money");closeMenu();
        W::HandheldDashboardOpen();W::HandheldDashboardMove(0,-static_cast<int>(W::HandheldDashboardSectionIndex()));press(H::Action::confirm);
        select([](){return W::HandheldOverviewPresentation();},"Park map...");press(H::Action::confirm);Check(W::HandheldParkMapIsOpen(),"Map handoff failed");capture("16-park-map.png");
        press(H::Action::slopeUp);capture("17-map-details.png");press(H::Action::back);Check(!W::HandheldParkMapIsOpen(),"Map cancel failed");closeMenu();
        News::InitQueue(state.park);News::AddItemToQueue(state.park.newsItems,News::ItemType::ride,"Ride requires your attention",target.id.ToUnderlying());
        W::HandheldNewsOpen();capture("19-ride-news.png");press(H::Action::back);closeMenu();
        for(int section=0;section<3;++section) {
            W::HandheldMenuOpen();W::HandheldMenuMove(7);press(H::Action::confirm);
            W::HandheldConstructionMove(section,0);capture(("21-guide-"+std::to_string(section)+".png").c_str());
            press(H::Action::back);closeMenu();
        }
        Check(H::ButtonAction(SDL_CONTROLLER_BUTTON_A)==H::Action::confirm && H::ActionKey(H::Action::confirm)=="A","Guide mapping differs from production");
        SDL_setenv("OPENRCT2_HANDHELD_SWAP_AB","1",1);
        Check(H::ButtonAction(SDL_CONTROLLER_BUTTON_A)==H::Action::back && H::ActionKey(H::Action::confirm)=="B","Swapped guide mapping incorrect");SDL_setenv("OPENRCT2_HANDHELD_SWAP_AB","0",1);
        std::cout<<"PASS direct price Apply / B discard / native stale guard / cash invariants / spatial access restoration / continuous 40-guest roster / held-scroll bounds / uniform grid Apply and Cancel / single-row footer\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
