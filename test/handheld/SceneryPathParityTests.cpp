// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/TextEntry.h>
#include <openrct2-ui/handheld/ConstructionPanel.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/SceneryTools.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/general/PauseToggleAction.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/object/ObjectEntryManager.h>
#include <openrct2/object/SceneryGroupEntry.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/park/ParkFile.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Banner.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace S = H::SceneryTools;
namespace W = Ui::Windows;
static void Check(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
#include "SceneryClearWorkflowTests.h"
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
        bool clearPanel = false;
        bool terrainPanel=false;
        CoordsXYZ captureFocus{ 656, 656, 32 };
        auto capture = [&](const char* name, bool menu) {
            const auto* output = std::getenv("POCKET_PARITY_CAPTURES");
            if (!output || argc != 4)
                return;
            std::filesystem::create_directories(output);
            Drawing::X8DrawingEngine engine(context->GetUiContext());
            engine.Resize(512, 384);
            engine.BeginDraw();
            auto& rt = *engine.getRT();
            Viewport viewport{};
            viewport.width = 512;
            viewport.height = 384;
            viewport.zoom = ZoomLevel{ 1 };
            const auto focus = captureFocus;
            const auto centre = Translate3DTo2DWithZ(0, focus);
            viewport.viewPos = centre - ScreenCoordsXY{ viewport.zoom.ApplyTo(310), viewport.zoom.ApplyTo(190) };
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (menu)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
            else if (clearPanel)
                H::SceneryClear::DrawOverlay(rt, { 8, 8 });
            else if (std::string(name).starts_with("landscaping-"))
            {
                H::DrawHud(rt, {512,384}, H::NativeHud());
                if (terrainPanel) H::AreaTools::DrawOverlay(rt,H::Concept::kOrigin);
                else S::DrawOverlay(rt, H::Concept::kOrigin, H::SceneryEditTarget());
                H::DrawConstructionFooter(rt, {512,384}, terrainPanel ? "Review" : S::Current().mode == S::Mode::pick ? "Pick" : S::ScatterActive() && !S::ScatterReviewed() ? "Review" : "Apply", "Options");
            }
            else
                S::DrawOverlay(rt, { 8, 8 });
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
        auto load = [&](const char* id, uint8_t type) {
            auto* object = objects.LoadObject(id);
            Check(object, "Scenery object missing");
            const auto index = objects.GetLoadedObjectEntryIndex(object);
            ScenerySetInvented({ type, index });
            return index;
        };
        const auto largeObject = load("rct2.scenery_large.ssig4", SCENERY_TYPE_LARGE);
        const auto smallObject = load("rct2.scenery_small.corroof2", SCENERY_TYPE_SMALL);
        const auto bannerObject = load("rct2.footpath_banner.bn1", SCENERY_TYPE_BANNER);
        const auto wallObject = load("rct2.scenery_wall.wallsign", SCENERY_TYPE_WALL);
        load("rct2.scenery_wall.wbr1a", SCENERY_TYPE_WALL);
        auto* groupObject = objects.LoadObject("rct2.scenery_group.scgwalls");
        Check(groupObject, "Native scenery group missing");
        const auto group = objects.GetLoadedObjectEntryIndex(groupObject);
        SceneryGroupSetInvented(group);
        auto* surface = objects.LoadObject("rct2.footpath_surface.tarmac");
        auto* railings = objects.LoadObject("rct2.footpath_railings.wood");
        Check(surface && railings, "Fixture paths missing");
        S::SelectCategory(S::Category::walls);
        const auto groups = S::Groups(S::Category::walls);
        Check(groups.size() > 1 && S::SetGroup(group), "Native group with researched wall hidden");
        const auto* entry = ObjectEntryManager::GetObjectEntry<SceneryGroupEntry>(group);
        for (const auto& item : S::Entries(S::Category::walls))
            Check(
                std::ranges::find(entry->SceneryEntries, ScenerySelection{ SCENERY_TYPE_WALL, item.identity.index })
                    != entry->SceneryEntries.end(),
                "Group contains non-member");
        Check(!S::SetGroup(30000), "Missing group accepted");
        const auto groupBeforeSearch=S::GroupName();
        const auto groupMembers=S::Entries(S::Category::walls);Check(!groupMembers.empty(),"Group search fixture empty");
        const auto member=groupMembers.front().identity;
        Check(S::SetSearch(member.key) && S::GroupName()==groupBeforeSearch,"Search replaced native group context");
        Check(std::ranges::any_of(S::Entries(S::Category::walls),[&](const auto& e){return e.identity==member;}),
            "Search lost eligible native group member");
        ScenerySetNotInvented({SCENERY_TYPE_WALL,member.index});
        Check(S::Entries(S::Category::walls).empty(),"Search bypassed native research membership");
        ScenerySetInvented({SCENERY_TYPE_WALL,member.index});
        Check(S::SetSearch("") && S::GroupName()==groupBeforeSearch && S::Entries(S::Category::walls).size()==groupMembers.size(),
            "Clearing search lost native group membership");
        H::BeginScenery();
        W::HandheldSceneryOptionsOpen();
        W::HandheldMenuMove(-100); W::HandheldMenuMove(12);
        H::ControllerInput controller;
        H::Frame confirm;
        confirm.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(controller, confirm);
        capture("parity-01-scenery-groups.png", true);
        H::Frame back;
        back.actions = { H::Action::back };
        H::HandleMenuInput(controller, back);
        H::EndTerrain();
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        S::SetGroup(kObjectEntryIndexNull);
        auto targetAt = [&](CoordsXY tile, S::Category category) {
            for (auto* e : TileElementsView<>(tile))
            {
                InteractionInfo info{};
                info.Element = e;
                info.Loc = tile;
                auto t = S::Capture(info);
                if (t && t->category == category)
                    return *t;
            }
            throw std::runtime_error("Target missing");
        };
        const CoordsXY tile{ 640, 640 };
        const auto height = MapGetSurfaceElementAt(tile)->getBaseZ();
        GameActions::FootpathPlaceAction path(
            { tile, height }, FootpathSlope{}, objects.GetLoadedObjectEntryIndex(surface),
            objects.GetLoadedObjectEntryIndex(railings));
        Check(GameActions::Execute(&path, state).error == GameActions::Status::ok, "Banner path failed");
        GameActions::ProcessQueue(state);
        GameActions::FootpathPlaceAction neighbour(
            { tile + CoordsDirectionDelta[0], height }, FootpathSlope{}, objects.GetLoadedObjectEntryIndex(surface),
            objects.GetLoadedObjectEntryIndex(railings));
        Check(GameActions::Execute(&neighbour, state).error == GameActions::Status::ok, "Banner neighbour failed");
        GameActions::ProcessQueue(state);
        S::Settings settings;
        settings.category = S::Category::banners;
        settings.object = bannerObject;
        settings.direction = 0;
        auto bannerResult = S::Place(tile, settings, true);
        if (bannerResult.error != GameActions::Status::ok)
            std::cerr << bannerResult.getErrorMessage() << "\n";
        Check(bannerResult.error == GameActions::Status::ok, "Banner placement failed");
        GameActions::ProcessQueue(state);
        auto target = targetAt(tile, S::Category::banners);
        H::BeginScenery();
        W::HandheldSceneryOptionsOpen();
        W::HandheldMenuMove(-100); W::HandheldMenuMove(14);
        capture("landscaping-03-eyedropper-options.png", true);
        H::HandleMenuInput(controller, confirm);
        Check(S::Current().mode == S::Mode::pick && !Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),
            "Controller eyedropper entry skipped pick mode");
        capture("landscaping-01-native-eyedropper.png", false);
        Check(S::Pick(target, false).error == GameActions::Status::ok, "Native banner eyedropper query failed");
        if (argc == 4)
        {
            auto* drawing = dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine());
            Check(drawing, "Native eyedropper drawing context missing");
            drawing->Drawing::X8DrawingEngine::Resize(512,384);
            auto* main = WindowGetMain();
            auto* viewport = main->viewport;
            const auto centre = ScreenCoordsXY{256,192};
            const auto projected = Translate3DTo2DWithZ(GetCurrentRotation(), CoordsXYZ{tile.x+16,tile.y+16,target.location.z});
            bool found = false;
            constexpr ViewportInteractionItems pickTypes = {ViewportInteractionItem::scenery,ViewportInteractionItem::wall,
                ViewportInteractionItem::largeScenery,ViewportInteractionItem::banner,ViewportInteractionItem::pathAddition};
            for (int dy=-40;dy<=40 && !found;dy+=2)
                for (int dx=-40;dx<=40 && !found;dx+=2)
                {
                    viewport->viewPos = projected - ScreenCoordsXY{viewport->zoom.ApplyTo(centre.x),viewport->zoom.ApplyTo(centre.y)} + ScreenCoordsXY{dx,dy};
                    main->savedViewPos = viewport->viewPos;
                    drawing->Drawing::X8DrawingEngine::BeginDraw();ViewportRender(*drawing->getRT(),viewport);
                    drawing->Drawing::X8DrawingEngine::EndDraw();
                    const auto hit = S::Capture(GetMapCoordinatesFromPos(centre,pickTypes));
                    found = hit && *hit == target;
                }
            Check(found, "Native eyedropper reticle did not hit actual banner sprite");
            H::UpdateExplorer(controller);
            const auto beforeCash = state.park.cash;
            const auto beforeLedger = state.park.expenditureTable;
            H::HandleExplorerInput(controller,confirm); // actual reticle; duplicate A must not place
            Check(S::Current().mode == S::Mode::place && S::Current().object == bannerObject
                    && state.park.cash == beforeCash && state.park.expenditureTable == beforeLedger
                    && targetAt(tile,S::Category::banners) == target,
                "Controller eyedropper changed native element/accounting or spilled held A into placement");
        }
        else Check(S::Pick(target, true).error == GameActions::Status::ok && S::Current().object == bannerObject,
            "Native banner eyedropper handoff failed");
        capture("landscaping-02-native-eyedropper-placement.png", false);
        H::EndTerrain();
        const auto searchCash = state.park.cash;
        const auto searchLedger = state.park.expenditureTable;
        const auto selectedBeforeSearch = S::Current();
        Check(S::SetSearch("sIgN") && !S::Entries(S::Category::banners).empty(),"Native case-insensitive object name search failed");
        Check(S::SetSearch("rct2.footpath_banner.bn1"),"Native descriptor search rejected");
        const auto descriptorResults = S::Entries(S::Category::banners);
        Check(descriptorResults.size()==1 && descriptorResults.front().identity.index==bannerObject
                && descriptorResults.front().identity.key==target.descriptor,"Search lost stable native descriptor identity");
        Check(!S::Select(bannerObject,target.descriptor+"-removed") && S::Current()==selectedBeforeSearch,
            "Search accepted stale descriptor or changed selection");
        Check(!S::SetSearch(std::string(33,'X')) && S::Search()=="rct2.footpath_banner.bn1","Search accepted unbounded input");
        Check(S::SetSearch(""),"Search clear failed");
        H::BeginScenery();W::HandheldSceneryOptionsOpen();W::HandheldMenuMove(-100); W::HandheldMenuMove(15);H::HandleMenuInput(controller,confirm);
        capture("landscaping-04-native-search-keyboard.png",true);
        size_t keyFocus=0;
        const auto keys=H::TextEntry{}.Keys();
        auto pressKey=[&](const std::string& key) {
            const auto it=std::find(keys.begin(),keys.end(),key);Check(it!=keys.end(),"Native keyboard key missing");
            const auto index=static_cast<size_t>(it-keys.begin());W::HandheldMenuMove(static_cast<int>(index)-static_cast<int>(keyFocus));
            H::HandleMenuInput(controller,confirm);keyFocus=index;
        };
        pressKey("B");pressKey("Done");
        Check(W::HandheldSceneryPickerIsOpen() && S::Search()=="B" && S::Current()==selectedBeforeSearch,
            "Controller search apply changed placement selection or missed results");
        const auto stableFocus=W::HandheldObjectPickerFocus();
        capture("landscaping-05-native-search-results.png",true);
        H::Frame searchAction;searchAction.actions={H::Action::contextPrimary,H::Action::contextPrimary};
        H::HandleMenuInput(controller,searchAction);keyFocus=0;pressKey("Z");H::HandleMenuInput(controller,back);
        Check(W::HandheldSceneryPickerIsOpen() && S::Search()=="B" && W::HandheldObjectPickerFocus()==stableFocus,
            "Cancelled search changed query or native focus");
        H::HandleMenuInput(controller,searchAction);keyFocus=0;pressKey("Clear");
        for(char c:std::string("NO-MATCH-4871")) { pressKey(std::string(1,c)); }
        pressKey("Done");
        Check(W::HandheldSceneryPickerIsOpen() && S::Entries(S::Category::banners).empty(),"Native empty search fabricated results");
        capture("landscaping-06-native-search-empty.png",true);
        H::HandleMenuInput(controller,searchAction);keyFocus=0;pressKey("Clear");pressKey("Done");
        Check(S::Search().empty() && !S::Entries(S::Category::banners).empty(),"Controller clear did not restore eligible objects");
        capture("landscaping-07-native-search-clear.png",true);
        H::HandleMenuInput(controller,confirm);
        Check(!Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu) && S::Current().object==bannerObject
                && state.park.cash==searchCash && state.park.expenditureTable==searchLedger,
            "Search/preview/selection mutated native accounting or missed placement handoff");
        H::EndTerrain();
        S::SelectCategory(S::Category::small); Check(S::Select(smallObject),"Scatter object missing"); S::Configure(6);
        H::BeginScenery(); W::HandheldSceneryOptionsOpen(); W::HandheldMenuMove(-100); W::HandheldMenuMove(16);
        H::HandleMenuInput(controller,confirm);
        W::HandheldConstructionMove(0,2); H::HandleMenuInput(controller,confirm); // medium density, size 1: two native attempts
        capture("landscaping-08-scatter-options.png",true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        const CoordsXY scatterTile{1504,1504}; captureFocus={1520,1520,32};
        if (argc==4)
        {
            auto* drawing=dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine());
            auto* main=WindowGetMain(); auto* viewport=main->viewport;
            const auto projected=Translate3DTo2DWithZ(GetCurrentRotation(),captureFocus);
            viewport->viewPos=projected-ScreenCoordsXY{viewport->zoom.ApplyTo(256),viewport->zoom.ApplyTo(192)};
            main->savedViewPos=viewport->viewPos;
            drawing->Drawing::X8DrawingEngine::BeginDraw(); ViewportRender(*drawing->getRT(),viewport);
            drawing->Drawing::X8DrawingEngine::EndDraw();
            H::UpdateExplorer(controller);
            std::cout << "Scatter controller native target " << gMapSelectPositionA.x << "," << gMapSelectPositionA.y
                << " / context " << ContextGetWidth() << "x" << ContextGetHeight() << "\n";
            captureFocus={gMapSelectPositionA.x+16,gMapSelectPositionA.y+16,32};
        }
        const auto bulkCash=state.park.cash;
        capture("landscaping-09-scatter-preview.png",false);
        if (argc==4) H::HandleExplorerInput(controller,confirm);
        else S::Place(scatterTile,S::Current(),true);
        Check(S::ScatterReviewed() && state.park.cash==bulkCash,"Held A skipped scatter review or charged");
        capture("landscaping-10-scatter-review.png",false);
        if (argc==4) H::HandleExplorerInput(controller,confirm);
        else S::Place(scatterTile,S::Current(),true);
        Check(!S::ScatterReviewed(),"Controller scatter apply missed native submission");
        Check(S::Place(scatterTile,S::Current(),true).error!=GameActions::Status::ok,"Pending scatter repeated");
        GameActions::ProcessQueue(state);
        Check(S::ScatterStatus().find("1/2 placed")!=std::string::npos && state.park.cash<bulkCash,
            "Scatter partial clearance or native accounting absent");
        capture("landscaping-11-scatter-partial-result.png",false);
        const auto nativeFailure=S::Options().back();
        Check(nativeFailure!="Last error: none","Partial scatter lost native failure reason");
        W::HandheldSceneryOptionsOpen(); W::HandheldMenuMove(-100); W::HandheldMenuMove(19);
        Check(S::Options().back()==nativeFailure,"Menu ghost cleanup erased completed native batch failure");
        capture("landscaping-12-scatter-native-error.png",true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        S::Configure(17,3); S::UpdatePreview(scatterTile);
        const auto bounds=S::ScatterBounds(scatterTile);
        gMapSelectPositionA=bounds.first; gMapSelectPositionB=bounds.second;
        captureFocus={scatterTile.x+16,scatterTile.y+16,32};
        capture("landscaping-13-scatter-area-preview.png",false);
        H::EndTerrain(); S::Configure(16); captureFocus={656,656,32};
        namespace A=H::AreaTools;
        auto* grid=objects.LoadObject("rct2.terrain_surface.grid");
        auto* stucco=objects.LoadObject("official.terrain_edge.stucco");
        Check(grid && stucco,"Native terrain preview objects missing");
        A::settings={}; A::settings.operation=A::Operation::paint; A::settings.width=A::settings.height=2;
        A::settings.surface=objects.GetLoadedObjectEntryIndex(grid); A::settings.edge=objects.GetLoadedObjectEntryIndex(stucco);
        A::ResetColours(); H::BeginAreaTools(); terrainPanel=true;
        W::HandheldAreaOptionsOpen(); W::HandheldMenuMove(-100); W::HandheldMenuMove(7);
        const auto beforePalette=A::settings.surfaceColour;
        H::HandleMenuInput(controller,confirm);
        Check(A::settings.surfaceColour!=beforePalette && A::SurfaceColourSupported(A::settings.surface),"Controller native palette selection failed");
        W::HandheldConstructionMove(0,1); const auto beforeEdgePalette=A::settings.edgeColour;
        H::HandleMenuInput(controller,confirm);
        Check(A::settings.edgeColour!=beforeEdgePalette && A::EdgeColourSupported(A::settings.edge),"Controller native edge palette selection failed");
        capture("landscaping-14-terrain-colours-options.png",true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        CoordsXY paintTile{1280,1280}; captureFocus={1296,1296,32};
        if (argc==4)
        {
            auto* drawing=dynamic_cast<Drawing::X8DrawingEngine*>(context->GetDrawingEngine());
            auto* main=WindowGetMain(); auto* viewport=main->viewport;
            const auto projected=Translate3DTo2DWithZ(GetCurrentRotation(),captureFocus);
            viewport->viewPos=projected-ScreenCoordsXY{viewport->zoom.ApplyTo(256),viewport->zoom.ApplyTo(192)};
            main->savedViewPos=viewport->viewPos;
            drawing->Drawing::X8DrawingEngine::BeginDraw(); ViewportRender(*drawing->getRT(),viewport);
            drawing->Drawing::X8DrawingEngine::EndDraw(); H::UpdateExplorer(controller);
            paintTile=gMapSelectPositionA; captureFocus={paintTile.x+16,paintTile.y+16,32};
        }
        const auto paintRange=*A::Range(paintTile,2,2);
        gMapSelectPositionA={paintRange.getX1(),paintRange.getY1()}; gMapSelectPositionB={paintRange.getX2(),paintRange.getY2()};
        capture("landscaping-15-terrain-native-preview.png",false);
        const auto paintCash=state.park.cash; const auto paintBefore=A::CapturePaint(paintRange);
        if (argc==4) H::HandleExplorerInput(controller,confirm);
        else W::HandheldTerrainPaintReviewOpen(paintTile);
        Check(Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu) && state.park.cash==paintCash,
            "Held A skipped native paint review");
        capture("landscaping-16-terrain-cancel-review.png",true);
        H::HandleMenuInput(controller,confirm);
        Check(state.park.cash==paintCash && A::CapturePaint(paintRange)==paintBefore,"Cancel-first paint mutated state");
        W::HandheldTerrainPaintReviewOpen(paintTile);
        const auto paintQuote=A::Action(paintTile,A::settings,false);
        W::HandheldConstructionMove(0,-1); H::HandleMenuInput(controller,confirm);
        Check(!Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),"Paint confirmation failed to recover tool");
        Check(!A::BeginPaint(paintTile,A::settings),"Paint action repeated before native completion");
        GameActions::ProcessQueue(state);
        Check(state.park.cash==paintCash-paintQuote.cost && MapGetSurfaceElementAt(paintTile)->getPrimarySurfaceColour()==A::settings.surfaceColour
                && MapGetSurfaceElementAt(paintTile)->getPrimaryEdgeColour()==A::settings.edgeColour,
            "Controller native colour action/cost differs");
        capture("landscaping-17-terrain-native-result.png",false);
        W::HandheldTerrainPaintReviewOpen(paintTile);
        MapGetSurfaceElementAt(paintTile)->setOwnership({});
        W::HandheldConstructionMove(0,-1); H::HandleMenuInput(controller,confirm);
        Check(Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),"Stale paint review executed instead of recovery");
        capture("landscaping-18-terrain-stale-review.png",true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        MapGetSurfaceElementAt(paintTile)->setOwnership({OwnershipFlag::landOwned});
        A::settings.surface=objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.terrain_surface.grass"));
        A::settings.edge=objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.terrain_edge.rock"));
        W::HandheldAreaOptionsOpen(); W::HandheldMenuMove(-100); W::HandheldMenuMove(8);
        const auto fixedColours=A::settings; H::HandleMenuInput(controller,confirm);
        Check(A::settings==fixedColours,"Controller unsupported colour changed native selection");
        capture("landscaping-19-terrain-unsupported-colour.png",true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        H::EndTerrain();
        H::BeginSurface(); H::Frame quickColourOptions; quickColourOptions.actions={H::Action::contextSecondary,H::Action::contextSecondary};
        H::HandleExplorerInput(controller,quickColourOptions);
        Check(!Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu) && A::settings.operation==A::Operation::paint
            && A::settings.width==1 && A::settings.height==1 && A::settings.surface!=kObjectEntryIndexNull,
            "Quick surface did not hand off native selection into Paint adjustment");
        H::Frame moreOptions;moreOptions.actions={H::Action::contextPrimary,H::Action::contextPrimary};
        H::HandleExplorerInput(controller,moreOptions);
        Check(Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu),"Paint Adjust More did not retain expanded options");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        H::EndTerrain(); terrainPanel=false; captureFocus={656,656,32};
        const auto cash = state.park.cash;
        auto draft = S::BeginStyle(target, S::Style::noEntry);
        Check(draft.has_value(), "No-entry draft absent");
        S::AdjustStyle(*draft, 1);
        Check(S::ApplyStyle(*draft, false).error == GameActions::Status::ok, "Style query failed");
        W::HandheldSceneryStyleOpen(target);
        H::HandleMenuInput(controller, confirm); // choose no-entry only, duplicate A cannot also alter it
        W::HandheldConstructionMove(1, 0);
        W::HandheldConstructionMove(0, 1);
        H::HandleMenuInput(controller, confirm); // review defaults Cancel
        capture("parity-02-no-entry-review.png", true);
        H::HandleMenuInput(controller, confirm); // cancel unchanged
        Check(!GetBanner(target.banner)->flags.has(BannerFlag::noEntry), "Cancel applied no entry");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Check(S::ApplyStyle(*draft, true).error == GameActions::Status::ok, "Style execute failed");
        GameActions::ProcessQueue(state);
        Check(
            GetBanner(target.banner)->flags.has(BannerFlag::noEntry) && state.park.cash == cash
                && S::ApplyStyle(*draft, true).error != GameActions::Status::ok,
            "No entry stale/cash mismatch");
        target = targetAt(tile, S::Category::banners);
        auto colour = S::BeginStyle(target, S::Style::textColour);
        Check(colour.has_value(), "Text colour missing");
        S::AdjustStyle(*colour, 1);
        auto invalid = *colour;
        invalid.value = 14;
        Check(S::ApplyStyle(invalid, true).error != GameActions::Status::ok, "Invalid text colour accepted");
        Check(S::ApplyStyle(*colour, true).error == GameActions::Status::ok, "Text colour action failed");
        GameActions::ProcessQueue(state);
        target = targetAt(tile, S::Category::banners);
        W::HandheldSceneryStyleOpen(target);
        W::HandheldConstructionMove(0, 1);
        H::HandleMenuInput(controller, confirm);
        capture("parity-03-banner-text-colour.png", true);
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        const CoordsXY wallTile{ 736, 640 };
        settings.category = S::Category::walls;
        settings.object = wallObject;
        Check(S::Place(wallTile, settings, true).error == GameActions::Status::ok, "Scrolling wall placement failed");
        GameActions::ProcessQueue(state);
        auto sign = targetAt(wallTile, S::Category::walls);
        Check(!S::BeginStyle(sign, S::Style::noEntry), "Wall offered no-entry");
        auto signColour = S::BeginStyle(sign, S::Style::textColour);
        Check(signColour.has_value(), "Native sign text colour absent");
        S::AdjustStyle(*signColour, 1);
        Check(S::ApplyStyle(*signColour, true).error == GameActions::Status::ok, "Native sign colour failed");
        GameActions::ProcessQueue(state);
        Check(S::ApplyStyle(*signColour, true).error != GameActions::Status::ok, "Repeated sign colour accepted");
        sign = targetAt(wallTile, S::Category::walls);
        auto stale = S::BeginStyle(sign, S::Style::mainColour);
        Check(stale.has_value(), "Sign primary missing");
        S::AdjustStyle(*stale, 1);
        Check(S::SetText(sign, "Changed after review", true).error == GameActions::Status::ok, "Text action failed");
        GameActions::ProcessQueue(state);
        Check(S::ApplyStyle(*stale, true).error != GameActions::Status::ok, "Changed sign text baseline accepted");
        TestSceneryClear(
            state, largeObject, smallObject, objects.GetLoadedObjectEntryIndex(surface),
            objects.GetLoadedObjectEntryIndex(railings), controller, confirm, clearPanel, captureFocus, capture);
        if(gGamePaused & GAME_PAUSED_NORMAL){GameActions::PauseToggleAction unpause;Check(GameActions::Execute(&unpause,state).error==GameActions::Status::ok,"Native repeat fixture unpause failed");GameActions::ProcessQueue(state);}
        // Continuous placement is a retained native tool, with one purchase per press.
        S::SelectCategory(S::Category::small);Check(S::Select(smallObject),"Repeat scenery selection failed");S::Configure(6,1);if(S::Current().scatter)S::Configure(16,1);
        const auto repeatedObject=S::Current().object;const CoordsXY repeatA{1856,1856},repeatB{1888,1856};
        S::UpdatePreview(repeatA);const auto automatic=S::Place(repeatA,S::Current(),false);if(automatic.error!=GameActions::Status::ok)std::cerr<<automatic.getErrorMessage()<<"\n";Check(automatic.error==GameActions::Status::ok,"Native automatic height query failed");
        const auto heightCash=state.park.cash;S::Configure(4,1);Check(S::Current().height==automatic.position.z+8 && state.park.cash==heightCash,"Manual height did not start at native proposal or changed cash");
        S::Configure(6,1);Check(S::Current().height==0,"Automatic height reset failed");
        for(const auto destination:{repeatA,repeatB}){
            S::UpdatePreview(destination);const auto quote=S::Place(destination,S::Current(),false);const auto beforeRepeat=state.park.cash;
            Check(quote.error==GameActions::Status::ok && S::Place(destination,S::Current(),true).error==GameActions::Status::ok,"Repeated native scenery placement failed");GameActions::ProcessQueue(state);
            Check(S::Current().object==repeatedObject && S::Current().mode==S::Mode::place && state.park.cash==beforeRepeat-quote.cost,"Repeated placement lost selection or changed native accounting");
        }
        S::ClearPreview();
        SaveFlags flags{};
        auto save = std::string(argv[2]) + "/parity.park";
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Style save/reload failed");
        for(const auto destination:{repeatA,repeatB}){bool present=false;for(const auto* e:TileElementsView<SmallSceneryElement>(destination))present|=!e->isGhost() && e->getEntryIndex()==repeatedObject;Check(present,"Repeated scenery did not persist through native load");}
        auto saved = targetAt(tile, S::Category::banners);
        Check(
            saved.bannerFlags == target.bannerFlags && saved.textColour == target.textColour,
            "Banner style persistence failed");

        std::cout << "Scenery groups, native banner/sign styles, stale/bounds, SDL Cancel-first and persistence passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
