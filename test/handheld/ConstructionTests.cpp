// SPDX-License-Identifier: GPL-3.0-only
// Public object metadata/native park fixture. No original sprites or live coaster assets.
#include <iostream>
#include <openrct2-ui/handheld/SceneryTools.h>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace S = Ui::Handheld::SceneryTools;
static void Require(bool b, const char* m)
{
    if (!b)
        throw std::runtime_error(m);
}
static size_t Ghosts()
{
    size_t n = 0;
    for (int y = 32; y < 992; y += 32)
        for (int x = 32; x < 992; x += 32)
            for (const auto* e : TileElementsView<>(CoordsXY{ x, y }))
                n += e->isGhost();
    return n;
}
static S::Target TargetAt(CoordsXY tile, S::Category category)
{
    for (auto* e : TileElementsView<>(tile))
    {
        InteractionInfo hit{};
        hit.Loc = tile;
        hit.Element = e;
        const auto t = S::Capture(hit);
        if (t && t->category == category)
            return *t;
    }
    throw std::runtime_error("Placed scenery target not found");
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        auto context = CreateContext();
        Config::SetDefaults();
        Require(context->Initialise(), "Fixture init failed");
        auto& state = getGameState();
        auto& objects = context->GetObjectManager();
        gameStateInitAll(state, { 32, 32 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 1000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        // Scenario fixture ownership/research only. All tested construction uses actual native actions.
        for (int y = 32; y < 992; y += 32)
            for (int x = 32; x < 992; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        auto load = [&](const char* id, uint8_t type) {
            auto* o = objects.LoadObject(id);
            Require(o, "Object metadata missing");
            auto i = objects.GetLoadedObjectEntryIndex(o);
            ScenerySetInvented({ type, i });
            return i;
        };
        auto* surface = objects.LoadObject("rct2.footpath_surface.tarmac");
        auto* railing = objects.LoadObject("rct2.footpath_railings.wood");
        Require(surface && railing, "Path fixture objects missing");
        const CoordsXY bannerTile{ 608, 192 };
        for (auto t : { bannerTile, CoordsXY{ 576, 192 } })
        {
            GameActions::FootpathPlaceAction a(
                { t, MapGetSurfaceElementAt(t)->getBaseZ() }, FootpathSlope{}, objects.GetLoadedObjectEntryIndex(surface),
                objects.GetLoadedObjectEntryIndex(railing));
            Require(GameActions::Execute(&a, state).error == GameActions::Status::ok, "Banner path failed");
            GameActions::ProcessQueue(state);
        }
        struct Case
        {
            S::Category category;
            const char* id;
            uint8_t type;
            CoordsXY tile;
        };
        const Case cases[] = { { S::Category::small, "rct2.scenery_small.corroof", SCENERY_TYPE_SMALL, { 128, 192 } },
                               { S::Category::large, "rct2.scenery_large.ssig4", SCENERY_TYPE_LARGE, { 288, 192 } },
                               { S::Category::walls, "rct2.scenery_wall.wbrg", SCENERY_TYPE_WALL, { 448, 192 } },
                               { S::Category::banners, "rct2.footpath_banner.bn1", SCENERY_TYPE_BANNER, bannerTile } };
        for (const auto& c : cases)
        {
            const auto i = load(c.id, c.type);
            S::Settings settings;
            settings.category = c.category;
            settings.object = i;
            settings.direction = 0;
            settings.quadrant = 2;
            Require(
                S::Eligible(c.category, i) && !S::Eligible(c.category, kObjectEntryIndexNull), "Scenery loaded filter failed");
            ScenerySetNotInvented({ c.type, i });
            Require(S::Place(c.tile, settings, false).error != GameActions::Status::ok, "Unresearched object accepted");
            ScenerySetInvented({ c.type, i });
            Require(
                S::Place(CoordsXY{ 0, 0 }, settings, true).error != GameActions::Status::ok
                    && S::Place(CoordsXY{ 129, 192 }, settings, false).error != GameActions::Status::ok,
                "Scenery bounds redirected");
            const auto cash = state.park.cash;
            const auto q = S::Place(c.tile, settings, false);
            Require(q.error == GameActions::Status::ok && state.park.cash == cash, "Scenery query failed/charged");
            state.park.cash = 0;
            Require(S::Place(c.tile, settings, false).error != GameActions::Status::ok, "Scenery funds rule bypassed");
            state.park.cash = cash;
            S::SelectCategory(c.category);
            Require(S::Select(i), "Scenery selection failed");
            while (S::Current().direction != 0)
                S::Configure(2);
            S::UpdatePreview(c.tile);
            Require(Ghosts() > 0 && state.park.cash == cash, "Scenery ghost absent/charged");
            S::RemovePreviewTemporarily();
            S::RemovePreviewTemporarily();
            Require(Ghosts() == 0, "Nested scenery suspension leaked");
            S::RestorePreview();
            Require(Ghosts() == 0, "Nested scenery restored early");
            S::RestorePreview();
            Require(Ghosts() > 0, "Scenery restoration missing");
            MapGetSurfaceElementAt(c.tile)->setOwnership({});
            S::ClearPreview();
            Require(Ghosts() == 0, "Scenery ghost cleanup failed after ownership loss");
            Require(S::Place(c.tile, settings, false).error != GameActions::Status::ok, "Unowned scenery accepted");
            MapGetSurfaceElementAt(c.tile)->setOwnership({ OwnershipFlag::landOwned });
            Require(S::Place(c.tile, settings, true).error == GameActions::Status::ok, "Scenery execute failed");
            GameActions::ProcessQueue(state);
            Require(state.park.cash == cash - q.cost && Ghosts() == 0, "Scenery exact cost/ghost failed");
            auto target = TargetAt(c.tile, c.category);
            auto colours = target.colours;
            colours[0] = Drawing::Colour::brightRed;
            if (S::Channels(c.category, i)[0])
            {
                Require(
                    S::Modify(target, S::Mode::recolour, colours, false).error == GameActions::Status::ok,
                    "Recolour query failed");
                Require(
                    S::Modify(target, S::Mode::recolour, colours, true).error == GameActions::Status::ok,
                    "Recolour execute failed");
                GameActions::ProcessQueue(state);
                Require(
                    S::Modify(target, S::Mode::recolour, colours, true).error != GameActions::Status::ok,
                    "Repeated recolour accepted");
                target = TargetAt(c.tile, c.category);
                Require(target.colours[0] == colours[0], "Native recolour differs");
            }
            if (target.banner != BannerIndex::GetNull())
            {
                Require(S::SetText(target, "Pocket sign", false).error == GameActions::Status::ok, "Sign text query failed");
                Require(S::SetText(target, "Pocket sign", true).error == GameActions::Status::ok, "Sign text execute failed");
                GameActions::ProcessQueue(state);
                Require(
                    S::SetText(target, "Pocket sign", true).error != GameActions::Status::ok, "Repeated sign draft accepted");
                target = TargetAt(c.tile, c.category);
                Require(target.text == "Pocket sign", "Native sign text differs");
            }
            SaveFlags flags{};
            auto save = std::string(argv[2]) + "/construction.park";
            const auto savedCash = state.park.cash;
            Require(ScenarioSave(state, save, flags) && context->LoadParkFromFile(save), "Scenery save/reload failed");
            target = TargetAt(c.tile, c.category);
            Require(state.park.cash == savedCash && Ghosts() == 0, "Scenery persistence accounting failed");
            const auto remove = S::Modify(target, S::Mode::remove, {}, false);
            Require(remove.error == GameActions::Status::ok, "Scenery removal query failed");
            Require(
                S::Modify(target, S::Mode::remove, {}, true).error == GameActions::Status::ok,
                "Scenery removal execute failed");
            GameActions::ProcessQueue(state);
            Require(
                state.park.cash == savedCash - remove.cost
                    && S::Modify(target, S::Mode::remove, {}, true).error != GameActions::Status::ok,
                "Scenery refund/stale removal failed");
            if (c.category != S::Category::banners)
            {
                settings.direction = 2;
                settings.height = MapGetSurfaceElementAt(c.tile)->getBaseZ() + 16;
                const auto elevated = S::Place(c.tile, settings, false);
                Require(elevated.error == GameActions::Status::ok, "Elevated rotated scenery query failed");
                Require(
                    S::Place(c.tile, settings, true).error == GameActions::Status::ok,
                    "Elevated rotated scenery execute failed");
                GameActions::ProcessQueue(state);
                const auto raised = TargetAt(c.tile, c.category);
                Require(raised.location.z == settings.height, "Native scenery height differs");
                Require(
                    S::Modify(raised, S::Mode::remove, {}, true).error == GameActions::Status::ok,
                    "Elevated scenery cleanup failed");
                GameActions::ProcessQueue(state);
            }
            std::cout << "Native " << S::CategoryName(c.category)
                      << " / query-research-bounds-ghost-ownership-colour-removal-persistence passed\n";
        }
        std::cout << "Pocket park construction native checks passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
