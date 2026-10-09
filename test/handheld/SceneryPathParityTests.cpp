// SPDX-License-Identifier: GPL-3.0-only
// Native initialized scenario fixture; construction and editing use real actions.
#include <SDL.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/RideAccess.h>
#include <openrct2-ui/handheld/SceneryTools.h>
#include <openrct2-ui/handheld/TrackEditing.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
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
using namespace OpenRCT2;
namespace H = Ui::Handheld;
namespace S = H::SceneryTools;
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
            const auto focus = CoordsXYZ{ 656, 656, 32 };
            const auto centre = Translate3DTo2DWithZ(0, focus);
            viewport.viewPos = centre - ScreenCoordsXY{ viewport.zoom.ApplyTo(310), viewport.zoom.ApplyTo(190) };
            ResetAllSpriteQuadrantPlacements();
            Drawing::UpdatePaletteEffects();
            ViewportRender(rt, &viewport);
            if (menu)
                Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu)->onDraw(rt);
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
        H::BeginScenery();
        W::HandheldSceneryOptionsOpen();
        W::HandheldConstructionMove(0, 12);
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
        SaveFlags flags{};
        auto save = std::string(argv[2]) + "/parity.park";
        Check(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Style save/reload failed");
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
