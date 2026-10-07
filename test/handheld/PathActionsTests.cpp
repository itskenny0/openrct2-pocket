// SPDX-License-Identifier: GPL-3.0-only
#include <iostream>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/MapOwnership.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <stdexcept>
using namespace OpenRCT2;
namespace Paths = OpenRCT2::Ui::Handheld::Paths;
static void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
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
        Require(context->Initialise(), "Context initialisation failed");
        auto& objects = context->GetObjectManager();
        Require(objects.LoadObject("rct2.footpath_surface.tarmac") != nullptr, "Normal surface missing");
        Require(objects.LoadObject("rct2.footpath_surface.queue_blue") != nullptr, "Queue surface missing");
        Require(objects.LoadObject("rct2.footpath_railings.wood") != nullptr, "Railings missing");
        auto& state = getGameState();
        gameStateInitAll(state, { 16, 16 });
        state.park.cash = 1000000;
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        gLegacyScene = LegacyScene::playing;
        const CoordsXY tile{ 64, 64 };
        auto* surface = MapGetSurfaceElementAt(tile);
        Require(surface != nullptr, "Surface missing");
        surface->setOwnership({ OwnershipFlag::landOwned });
        InteractionInfo info;
        info.Loc = tile;
        info.Element = reinterpret_cast<TileElement*>(surface);
        info.interactionType = ViewportInteractionItem::terrain;
        Paths::Begin(false);
        Paths::UpdateTargetFromInfo(info);
        const auto originalCash = state.park.cash;
        const CoordsXYZ location{ tile, surface->getBaseZ() };
        auto result = Paths::Action(false);
        Require(result.error == GameActions::Status::ok, "Normal query failed");
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(result.error == GameActions::Status::ok, "Normal placement failed");
        auto* path = MapGetPathElementAt(TileCoordsXYZ{ location });
        Require(path && !path->isQueue(), "Normal path not inserted");
        Require(state.park.cash < originalCash, "Construction did not debit cash");
        info.Element = reinterpret_cast<TileElement*>(path);
        info.interactionType = ViewportInteractionItem::footpath;
        Paths::Begin(true);
        Paths::UpdateTargetFromInfo(info);
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(result.error == GameActions::Status::ok, "Queue conversion failed");
        path = MapGetPathElementAt(TileCoordsXYZ{ location });
        Require(path && path->isQueue(), "Queue flag missing");
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto save = std::string(argv[2]) + "/paths.park";
        Require(ScenarioSave(state, save, flags) != 0, "Native save failed");
        Require(context->LoadParkFromFile(save), "Native reload failed");
        path = MapGetPathElementAt(TileCoordsXYZ{ location });
        Require(path && path->isQueue(), "Queue not preserved in save");
        info.Element = reinterpret_cast<TileElement*>(path);
        Paths::Begin(false, true);
        Paths::UpdateTargetFromInfo(info);
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(result.error == GameActions::Status::ok, "Removal failed");
        Require(MapGetPathElementAt(TileCoordsXYZ{ location }) == nullptr, "Removed path remains");
        // Ownership checks and paused construction still go through native queries.
        const CoordsXY unowned{ 96, 64 };
        surface = MapGetSurfaceElementAt(unowned);
        surface->setOwnership({});
        info.Loc = unowned;
        info.Element = reinterpret_cast<TileElement*>(surface);
        info.interactionType = ViewportInteractionItem::terrain;
        Paths::Begin(false);
        Paths::UpdateTargetFromInfo(info);
        Require(Paths::Action(false).error == GameActions::Status::disallowed, "Unowned placement accepted");
        surface->setOwnership({ OwnershipFlag::landOwned });
        gGamePaused = 1;
        Require(Paths::Action(false).error == GameActions::Status::gamePaused, "Paused placement accepted");
        const auto priorities = state.researchPriorities;
        GameActions::ParkSetResearchFundingAction funding(priorities, 3);
        Require(GameActions::Execute(&funding, state).error == GameActions::Status::ok, "Funding action rejected while paused");
        GameActions::ProcessQueue(state);
        Require(state.researchFundingLevel == 3 && state.researchPriorities == priorities, "Funding action failed");
        gGamePaused = 0;
        Paths::Begin(false);
        Paths::UpdateTargetFromInfo({});
        Require(Paths::Action(true).error != GameActions::Status::ok, "Invalid target accepted");
        Paths::End();
        Require(!Paths::IsActive(), "Tool remained active");
        std::cout << "Native path place / queue / save / reload / remove / ownership / pause and research checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
