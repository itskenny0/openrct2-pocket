// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/ParkManagementTabs.h>
#include <openrct2-ui/handheld/ResearchCatalog.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/RideData.h>
#include <openrct2/management/Research.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace W = Ui::Windows;
static void Check(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=4)return 2;
        SDL_setenv("OPENRCT2_HANDHELD","1",1);SDL_setenv("SDL_VIDEODRIVER","offscreen",0);SDL_setenv("SDL_AUDIODRIVER","dummy",0);
        gCustomOpenRCT2DataPath=argv[1];gCustomUserDataPath=argv[2];gOpenRCT2Headless=true;gOpenRCT2NoGraphics=false;
        auto env=CreatePlatformEnvironment();env->SetBasePath(DirBase::rct2,argv[3]);auto ui=Ui::CreateUiContext(*env);
        auto context=CreateContext(std::move(env),Audio::CreateDummyAudioContext(),std::move(ui));Config::SetDefaults();
        Config::Get().general.rct2Path=argv[3];Config::Get().general.windowWidth=512;Config::Get().general.windowHeight=384;Config::Get().general.windowScale=1;
        Check(context->Initialise(),"Context failed");context->GetUiContext().CreateWindow();auto& state=getGameState();gameStateInitAll(state,{64,64});gLegacyScene=LegacyScene::playing;
        Check(W::MainOpen(),"Viewport missing");WindowGetMain()->onResize();InputSetState(InputState::normal);auto* wm=Ui::GetWindowManager();wm->CloseByClass(WindowClass::progressWindow);
        auto& park=state.park;park.name="Pocket Park";park.cash=12000.00_GBP;park.bankLoan=2000.00_GBP;park.maxBankLoan=10000.00_GBP;
        park.value=28000.00_GBP;park.currentProfit=-125.00_GBP;park.entranceFee=10.00_GBP;park.rating=720;
        park.flags.unset(ParkFlag::noMoney,ParkFlag::noMoneyScenario,ParkFlag::freeEntry,ParkFlag::forbidMarketingCampaigns);park.flags.set(ParkFlag::unlockAllPrices);
        state.date.monthsElapsed=11;
        for(auto& month:park.expenditureTable)for(auto& entry:month)entry=0;
        park.expenditureTable[0][EnumValue(ExpenditureType::parkEntranceTickets)]=450.00_GBP;
        park.expenditureTable[0][EnumValue(ExpenditureType::rideConstruction)]=-120.00_GBP;
        park.expenditureTable[0][EnumValue(ExpenditureType::wages)]=-60.00_GBP;
        park.expenditureTable[1][EnumValue(ExpenditureType::shopSales)]=280.00_GBP;
        for(size_t i=0;i<64;++i)
        {
            park.cashHistory[i]=i<18?park.cash-park.bankLoan-static_cast<int>(i)*130.00_GBP:kMoney64Undefined;
            park.weeklyProfitHistory[i]=i<18?(static_cast<int>(i)%7-3)*75.00_GBP:kMoney64Undefined;
            park.valueHistory[i]=i<18?park.value-static_cast<int>(i)*330.00_GBP:kMoney64Undefined;
            if(i<std::size(park.ratingHistory))park.ratingHistory[i]=i<18?720-i*4:kParkRatingHistoryUndefined;
        }
        auto& objects=context->GetObjectManager();for(const char* id:{"rct2.terrain_surface.grass","rct2.terrain_edge.rock","rct2.station.plain","rct2.water.wtrcyan"})Check(objects.LoadObject(id),"Terrain object missing");
        auto* carousel=objects.LoadObject("rct2.ride.mgr1");Check(carousel,"Carousel missing");const auto objectIndex=objects.GetLoadedObjectEntryIndex(carousel);
        // Initialized native records provide eligible target/catalog data; management changes below use real actions.
        for(uint16_t i=100;i<118;++i){auto* ride=RideAllocateAtIndex(RideId::FromUnderlying(i));Check(ride,"Ride record fixture failed");
            ride->type=RIDE_TYPE_MERRY_GO_ROUND;ride->subtype=objectIndex;ride->status=RideStatus::open;ride->customName="Carousel "+std::to_string(i);ride->overallView={320+(i-100)*32,320};ride->price[0]=2.00_GBP;}
        const ResearchItem item(Research::EntryType::ride,objectIndex,RIDE_TYPE_MERRY_GO_ROUND,ResearchCategory::gentle,RESEARCH_ENTRY_FLAG_FIRST_OF_TYPE);
        state.researchItemsUninvented={item};state.researchNextItem=item;state.researchProgressStage=RESEARCH_STAGE_COMPLETING_DESIGN;
        state.researchProgress=32768;state.researchFundingLevel=1;state.researchPriorities=0x80|(1u<<EnumValue(ResearchCategory::gentle));
        state.researchExpectedMonth=4;state.researchExpectedDay=12;
        H::ControllerInput input;
        auto press=[&](H::Action a){H::Frame f;f.actions={a,a};H::HandleExplorerInput(input,f);};
        auto nav=[&](int x,int y){H::Frame f;f.navigateX=x;f.navigateY=y;H::HandleExplorerInput(input,f);};
        auto model=[](){return W::HandheldManagementPresentation();};
        auto select=[&](const std::string& label){auto m=model();auto it=std::find(m.rows.begin(),m.rows.end(),label);Check(it!=m.rows.end(),("Missing row: "+label).c_str());W::HandheldMenuMove(static_cast<int>(it-m.rows.begin())-static_cast<int>(m.selected));};
        auto open=[&](size_t section){W::HandheldDashboardOpen();W::HandheldDashboardMove(0,static_cast<int>(section)-static_cast<int>(W::HandheldDashboardSectionIndex()));press(H::Action::confirm);};
        auto capture=[&](const char* name){const char* dir=std::getenv("POCKET_MANAGEMENT_UX_CAPTURES");if(!dir)return;std::filesystem::create_directories(dir);
            Drawing::X8DrawingEngine engine(context->GetUiContext());engine.Resize(512,384);engine.BeginDraw();auto& rt=*engine.getRT();ResetAllSpriteQuadrantPlacements();Drawing::UpdatePaletteEffects();ViewportRender(rt,WindowGetMain()->viewport);
            auto* window=wm->FindByClass(WindowClass::handheldMenu);Check(window,"Capture menu missing");window->onDraw(rt);
            Check(H::FooterHeight({ 512, 384 }, H::LastControlHints) == 28, "Runtime footer expanded beyond one row");
            Image image;image.Width=512;image.Height=384;image.Depth=8;image.Stride=rt.LineStride();image.Palette=Drawing::gPalette;const auto* bytes=reinterpret_cast<uint8_t*>(rt.bits);image.Pixels.assign(bytes,bytes+rt.LineStride()*rt.height);
            Imaging::WriteToFile((std::filesystem::path(dir)/name).string(),image,ImageFormat::png);engine.EndDraw();};
        W::HandheldDashboardOpen();
        capture("00-dashboard.png");
        press(H::Action::back);
        WindowCullDead();
        Check(!W::HandheldDashboardIsOpen(), "Dashboard rail Back did not return to park");
        open(4);Check(model().tabs.size()==4&&model().selectedTab==0,"Finance tabs missing");
        Check(model().chart&&model().chart->splitValues.size()==2&&std::get<1>(model().chart->split[0])-std::get<1>(model().chart->split[1])==270.00_GBP,"Month in/out not native signed ledger totals");Check(model().heroStat&&model().primaryRow==0,"Finance overview lost its headline or primary action");capture("01-finance-overview.png");
        press(H::Action::nextOption);Check(model().rowValues.size()==14,"Ledger omitted categories/values");
        Check(model().rowValues[0]==H::FormatMoney(-120.00_GBP),"Ledger sign lost");capture("02-finance-ledger.png");
        nav(-1,0);Check(model().rowValues[6]==H::FormatMoney(280.00_GBP),"Ledger month selection failed");nav(1,0);
        W::HandheldMenuMove(10);const auto ledgerFocus=model().selected;
        press(H::Action::nextOption);Check(model().selectedTab==2,"History did not remain in workspace");capture("03-finance-history.png");
        press(H::Action::confirm);Check(model().title=="Weekly profit history","Next native graph unavailable");nav(-1,0);capture("04-finance-profit-history.png");
        press(H::Action::previousOption);Check(model().selected==ledgerFocus,"Ledger focus lost across tabs");press(H::Action::nextOption);press(H::Action::nextOption);
        Check(model().selectedTab==3,"Financial controls unavailable");capture("05-finance-controls.png");
        select("Borrow / repay...");const auto cash=park.cash;const auto loan=park.bankLoan;press(H::Action::confirm);nav(1,0);capture("06-finance-loan-draft.png");
        Check(park.cash==cash&&park.bankLoan==loan,"Finance draft spent cash");press(H::Action::confirm);Check(model().confirmation&&model().selected==0,"Finance review does not start on Apply");
        W::HandheldMenuMove(1);press(H::Action::confirm);Check(park.cash==cash,"Cancel applied loan");Check(model().selectedTab==3,"Cancel lost Controls tab");
        press(H::Action::confirm);nav(1,0);press(H::Action::confirm);capture("07-finance-review.png");select("Borrow");press(H::Action::confirm);
        Check(park.bankLoan==loan,"Finance bypassed native queue");press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(park.bankLoan==loan+1000.00_GBP&&park.cash==cash+1000.00_GBP,"Finance native callback charged incorrectly/twice");
        Check(model().selectedTab==3,"Finance result lost Controls/focus");
        park.flags.set(ParkFlag::noMoney);Check(!model().rowEnabled[0]&&!model().rowEnabled[1],"No-money controls not disabled");capture("08-finance-unavailable.png");park.flags.unset(ParkFlag::noMoney);
        // Park admission is now a direct editor with a return bookmark, rather than a detour through Finance overview.
        open(0);Check(model().tabs.size()==3,"Park tabs absent");capture("09-park-administration.png");
        select("Admission...");const auto admission=park.entranceFee;press(H::Action::confirm);Check(model().editing,"Park admission still requires another menu");nav(1,0);press(H::Action::back);
        Check(model().tabs.size()==3&&model().selected==1&&park.entranceFee==admission,"Admission cancel lost park focus/applied draft");
        press(H::Action::confirm);nav(1,0);select("Apply price");press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(park.entranceFee==admission+1.00_GBP&&model().tabs.size()==3&&model().selected==1,"Native admission commit/park return failed");
        press(H::Action::nextOption);capture("10-park-objectives.png");press(H::Action::nextOption);capture("11-park-records.png");
        select("Park history...");press(H::Action::confirm);capture("12-park-native-history.png");press(H::Action::back);Check(model().selectedTab==2&&model().selected==2,"History Back lost Records focus");
        select("Park news...");press(H::Action::confirm);press(H::Action::back);Check(model().selectedTab==2&&model().selected==0,"News return lost Records");
        press(H::Action::nextOption);select("Park map...");press(H::Action::confirm);Check(W::HandheldParkMapIsOpen(),"Native park map unavailable");press(H::Action::back);Check(model().tabs.size()==3,"Map Back lost park workspace");
        // Current native project, stage normalization, all seven priority categories, draft-only funding shortcut.
        open(5);Check(model().tabs.size()==2&&model().meters[2]==50,"Research native stage progress incorrect");capture("13-research-project.png");
        const auto funding=state.researchFundingLevel,priorities=state.researchPriorities;press(H::Action::nextOption);
        Check(model().rows.size()==9&&model().rowEnabled.size()==9,"Research priority group missing");capture("14-research-settings.png");
        press(H::Action::contextPrimary);Check(state.researchFundingLevel==funding,"X funding applied without review");
        W::HandheldMenuMove(1);press(H::Action::confirm);Check(state.researchPriorities==priorities&&model().headerStatus=="Unsaved draft","Priority draft applied or hidden");capture("15-research-draft.png");
        const auto draftFocus = model().selected;
        press(H::Action::openBuild);
        Check(model().title == "Keep research changes?", "Select dropped research draft");
        press(H::Action::back);
        Check(
            model().selected == draftFocus && model().headerStatus == "Unsaved draft", "Select cancellation lost draft focus");
        press(H::Action::openMenu);
        Check(model().title == "Keep research changes?", "Start dropped research draft");
        press(H::Action::back);
        press(H::Action::previousOption);
        Check(model().title == "Keep research changes?", "Research tab change silently discarded draft");
        capture("15a-research-leave-review.png");
        press(H::Action::back);
        Check(model().headerStatus == "Unsaved draft" && model().selectedTab == 1, "B did not continue editing research");
        press(H::Action::back);
        select("Discard changes");
        press(H::Action::confirm);
        Check(
            W::HandheldDashboardRailFocused() && state.researchPriorities == priorities
                && state.researchFundingLevel == funding,
            "Discard changed native settings or failed return");
        press(H::Action::confirm);
        if (model().selectedTab != 1)
            press(H::Action::nextOption);
        Check(model().headerStatus == "Current settings", "Discard left stale research draft");
        press(H::Action::contextPrimary);select("Review settings...");press(H::Action::confirm);Check(model().rows.size()==2&&model().selected==0,"Research review not compact/action-first");capture("16-research-review.png");
        press(H::Action::back);Check(model().headerStatus=="Unsaved draft"&&model().selectedTab==1,"Research review Back lost draft");
        select("Review settings...");
        press(H::Action::confirm);
        W::HandheldMenuMove(1); // the review starts on Apply; move to Cancel
        press(H::Action::confirm);
        Check(model().headerStatus == "Unsaved draft", "Explicit review Cancel discarded research draft");
        select("Review settings...");press(H::Action::confirm);select("Apply settings");press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(state.researchFundingLevel==2&&state.researchPriorities==priorities,"Research plan applied wrong native values");
        Check(model().headerStatus=="Current settings","Applied research retained stale baseline");
        press(H::Action::contextPrimary);
        press(H::Action::previousOption);
        select("Apply changes");
        press(H::Action::confirm);
        Check(state.researchFundingLevel == 2, "Guarded Apply bypassed native queue");
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        Check(
            state.researchFundingLevel == 3 && model().selectedTab == 0,
            "Guarded research Apply failed requested tab transition");
        press(H::Action::nextOption);
        press(H::Action::contextPrimary);
        press(H::Action::previousOption);
        select("Apply changes");
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        press(H::Action::nextOption);
        press(H::Action::contextPrimary);
        press(H::Action::previousOption);
        select("Apply changes");
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        press(H::Action::nextOption);
        press(H::Action::contextPrimary);
        press(H::Action::previousOption);
        select("Apply changes");
        press(H::Action::confirm);
        GameActions::ProcessQueue(state);
        press(H::Action::nextOption);
        Check(state.researchFundingLevel == 2, "Research cycle fixture did not restore normal funding");
        const auto beforeResearch=park.cash;FinancePayResearch();Check(park.cash==beforeResearch-kResearchCosts[2]/4,"Native recurring research charge changed");
        press(H::Action::previousOption);select("Invention catalog...");press(H::Action::confirm);capture("17-research-catalog.png");press(H::Action::back);Check(model().selectedTab==0&&model().selected==1,"Catalog return lost focus");
        // Campaign selection is now at the workspace root; target and duration adjust inline, full target browsing remains.
        open(6);Check(model().tabs.size()==2,"Marketing tabs absent");capture("18-marketing-empty.png");press(H::Action::nextOption);
        // Start a different native campaign first, so the next queued success must restore a nonzero active row.
        select("Advertise park");press(H::Action::confirm);select("Review campaign...");press(H::Action::confirm);select("Start campaign");press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(MarketingGetCampaign(ADVERTISING_CAMPAIGN_PARK),"First native campaign did not start");press(H::Action::nextOption);
        Check(model().rows.size()==ADVERTISING_CAMPAIGN_COUNT,"Campaign types lost");capture("19-marketing-types.png");
        select("Advertise ride");press(H::Action::confirm);Check(model().editing,"Campaign type did not open editor");nav(1,0);
        Check(model().rows[0].find("Carousel")!=std::string::npos,"Inline target change failed");const auto selectedTarget=model().rows[0];capture("20-campaign-editor.png");
        press(H::Action::confirm);W::HandheldMenuMove(12);capture("21-campaign-targets.png");press(H::Action::back);Check(model().rows[0]==selectedTarget,"Target browsing Cancel changed draft");
        W::HandheldMenuMove(1);nav(1,0);Check(model().rows[1].find("3 weeks")!=std::string::npos,"Inline duration failed");
        select("Review campaign...");press(H::Action::confirm);Check(model().confirmation&&model().selected==0,"Campaign review does not start on Start campaign");capture("22-campaign-review.png");
        const auto marketingCash=park.cash;press(H::Action::back);Check(model().rows[1].find("3 weeks")!=std::string::npos,"Review Back lost duration");
        select("Review campaign...");press(H::Action::confirm);select("Start campaign");press(H::Action::confirm);GameActions::ProcessQueue(state);
        Check(MarketingGetCampaign(ADVERTISING_CAMPAIGN_RIDE)&&park.cash==marketingCash-AdvertisingCampaignPricePerWeek[ADVERTISING_CAMPAIGN_RIDE]*3,"Native campaign accounting/duration changed");
        Check(model().selectedTab==0&&model().selected==1&&model().rowDetails[model().selected].find("Carousel")!=std::string::npos,"Active campaign lacks target");capture("23-marketing-active.png");
        press(H::Action::nextOption);Check(!model().rowEnabled[ADVERTISING_CAMPAIGN_RIDE],"Active campaign can be duplicated");
        park.flags.set(ParkFlag::forbidMarketingCampaigns);const auto unavailable=model();Check(std::none_of(unavailable.rowEnabled.begin(),unavailable.rowEnabled.end(),[](bool v){return v;}),"Authority restriction not shown");park.flags.unset(ParkFlag::forbidMarketingCampaigns);
        // Complete section/park return, with accepted advanced path modes untouched.
        press(H::Action::back);Check(W::HandheldDashboardRailFocused(),"Root B did not restore section rail");press(H::Action::back);Check(!W::HandheldDashboardIsOpen(),"Dashboard Back did not return to park");
        H::Paths::Begin(false);H::Paths::CycleMode(1);Check(H::Paths::Mode()==H::Paths::BuildMode::line,"Phase 7 Line mode regressed");H::Paths::End();
        gGamePaused=0;SaveFlags flags{};flags.set(SaveFlag::exportObjects);const auto save=(std::filesystem::path(argv[2])/"management.park").string();
        const auto savedCash=park.cash;const auto savedLoan=park.bankLoan;Check(ScenarioSave(state,save,flags)!=0&&context->LoadParkFromFile(save),"Management save/reload failed");
        Check(getGameState().park.cash==savedCash&&getGameState().park.bankLoan==savedLoan&&MarketingGetCampaign(ADVERTISING_CAMPAIGN_RIDE),"Management native values lost on reload");
        std::cout<<"PASS ManagementUxTests: finance values/graphs/loan/cancel, native research drafts/cost/catalog, marketing targets/duration/cost, park/context return, disabled states, save/reload\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}

