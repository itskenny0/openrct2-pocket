// SPDX-License-Identifier: GPL-3.0-only
#include <iostream>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/MapOwnership.h>
#include <openrct2/world/TileElementsView.h>
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
static PathElement* FindPath(const TileCoordsXYZ& location)
{
    // The engine's normal path lookup deliberately excludes ghosts.
    for (auto* path : TileElementsView<PathElement>(location.toCoordsXY()))
        if (path->baseHeight == location.z)
            return path;
    return nullptr;
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3 && argc != 4)
            return 2;
        gCustomOpenRCT2DataPath = argv[1];
        gCustomUserDataPath = argv[2];
        gOpenRCT2Headless = true;
        gOpenRCT2NoGraphics = true;
        std::unique_ptr<IContext> context;
        if (argc == 4)
        {
            auto env = CreatePlatformEnvironment();
            env->SetBasePath(DirBase::rct2, argv[3]);
            auto ui = Ui::CreateUiContext(*env);
            context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), std::move(ui));
        }
        else
            context = CreateContext();
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
        const auto placementCost = result.cost;
        Paths::UpdatePreview();
        auto* preview = FindPath(TileCoordsXYZ{ location });
        Require(preview && preview->isGhost() && !preview->isQueue(), "Normal ghost missing");
        Require(state.park.cash == originalCash, "Preview spent cash");
        Paths::UpdatePreview();
        Require(FindPath(TileCoordsXYZ{ location }) == preview, "Unchanged preview rebuilt");
        Require(Paths::Action(false).cost == placementCost, "Ghost changed placement cost");
        int ghosts = 0;
        for (auto* e : TileElementsView<PathElement>(tile))
            ghosts += e->isGhost();
        Require(ghosts == 1, "Duplicate ghosts inserted");
        Ui::ProvisionalElementsRemove();
        Require(!FindPath(TileCoordsXYZ{ location }), "Simulation suspension left ghost");
        Ui::ProvisionalElementsRestore();
        Require(FindPath(TileCoordsXYZ{ location })->isGhost(), "Simulation did not restore ghost");
        Paths::ClearPreview(); // Modal/options/focus loss use this same lifecycle entry point.
        Ui::ProvisionalElementsRestore();
        Require(!FindPath(TileCoordsXYZ{ location }), "Cleared ghost resurrected");
        Paths::UpdatePreview();
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(result.error == GameActions::Status::ok, "Normal placement failed");
        auto* path = FindPath(TileCoordsXYZ{ location });
        Require(path && !path->isQueue(), "Normal path not inserted");
        Require(!path->isGhost() && state.park.cash == originalCash - placementCost, "Placement did not charge exactly once");
        info.Element = reinterpret_cast<TileElement*>(path);
        info.interactionType = ViewportInteractionItem::footpath;
        Paths::Begin(true);
        Paths::UpdateTargetFromInfo(info);
        Paths::UpdatePreview();
        path = FindPath(TileCoordsXYZ{ location });
        Require(path && !path->isGhost() && !path->isQueue(), "Conversion preview modified real path");
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(result.error == GameActions::Status::ok, "Queue conversion failed");
        path = FindPath(TileCoordsXYZ{ location });
        Require(path && path->isQueue(), "Queue flag missing");
        // Fixed height survives movement to another tile; slope and removal use native actions.
        const CoordsXY bridgeTile{ 128, 64 };
        surface = MapGetSurfaceElementAt(bridgeTile);
        surface->setOwnership({ OwnershipFlag::landOwned });
        info.Loc = bridgeTile;
        info.Element = reinterpret_cast<TileElement*>(surface);
        info.interactionType = ViewportInteractionItem::terrain;
        Paths::Begin(false);
        Paths::UpdateTargetFromInfo(info);
        const CoordsXYZ groundLocation{ bridgeTile, surface->getBaseZ() };
        Paths::UpdatePreview();
        Require(FindPath(TileCoordsXYZ{ groundLocation })->isGhost(), "Ground preview missing");
        Require(Paths::ChangeHeight(1) && Paths::ChangeHeight(1), "Height selection failed");
        Require(!FindPath(TileCoordsXYZ{ groundLocation }), "Height change left old ghost");
        const CoordsXYZ bridgeLocation{ bridgeTile, groundLocation.z + 2 * kPathHeightStep };
        Require(Paths::CycleSlope() && Paths::RotateSlope(), "Slope selection failed");
        Paths::UpdatePreview();
        Require(FindPath(TileCoordsXYZ{ bridgeLocation }), "Raised preview missing before cursor move");
        const CoordsXY movedTile{ 160, 64 };
        surface = MapGetSurfaceElementAt(movedTile);
        surface->setOwnership({ OwnershipFlag::landOwned });
        info.Loc = movedTile;
        info.Element = reinterpret_cast<TileElement*>(surface);
        Paths::UpdateTargetFromInfo(info);
        Paths::UpdatePreview();
        const CoordsXYZ movedLocation{ movedTile, bridgeLocation.z };
        Require(!FindPath(TileCoordsXYZ{ bridgeLocation }), "Cursor move stranded old ghost");
        auto* slopeGhost = FindPath(TileCoordsXYZ{ movedLocation });
        Require(
            slopeGhost && slopeGhost->isGhost() && slopeGhost->isSloped() && slopeGhost->getSlopeDirection() == 1,
            "Moved slope preview incorrect");
        const auto beforeBridgeCash = state.park.cash;
        const auto bridgeCost = Paths::Action(false).cost;
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(state.park.cash == beforeBridgeCash - bridgeCost, "Slope placement charged incorrectly");
        auto* elevated = FindPath(TileCoordsXYZ{ movedLocation });
        Require(result.error == GameActions::Status::ok && elevated && elevated->isSloped(), "Elevated slope placement failed");
        Require(elevated->getSlopeDirection() == 1, "Slope rotation failed");
        Paths::FollowGround();
        Require(Paths::PlacementName() == "Ground / auto slope", "Ground mode reset failed");
        // Saving while a preview is visible must never persist it or its neighbour connections.
        const auto savedEdges = FindPath(TileCoordsXYZ{ location })->getEdges();
        const CoordsXY previewTile{ 96, 64 };
        surface = MapGetSurfaceElementAt(previewTile);
        surface->setOwnership({ OwnershipFlag::landOwned });
        const CoordsXYZ previewLocation{ previewTile, surface->getBaseZ() };
        info.Loc = previewTile;
        info.Element = reinterpret_cast<TileElement*>(surface);
        Paths::Begin(true);
        Paths::UpdateTargetFromInfo(info);
        Paths::UpdatePreview();
        auto* queueGhost = FindPath(TileCoordsXYZ{ previewLocation });
        Require(queueGhost && queueGhost->isGhost() && queueGhost->isQueue(), "Queue ghost missing");
        Paths::CycleSurface();
        Require(!FindPath(TileCoordsXYZ{ previewLocation }), "Surface change left old ghost");
        Paths::UpdatePreview();
        Paths::ToggleRemove();
        Require(!FindPath(TileCoordsXYZ{ previewLocation }), "Removal mode left ghost");
        Paths::ToggleRemove();
        Paths::UpdatePreview();
        SaveFlags flags{};
        flags.set(SaveFlag::exportObjects);
        const auto save = std::string(argv[2]) + "/paths.park";
        if (argc == 4)
        {
            flags.set(SaveFlag::automatic);
            Require(ScenarioSave(state, save, flags) != 0, "Native autosave failed");
            Require(FindPath(TileCoordsXYZ{ previewLocation }), "Autosave did not restore preview");
            flags.unset(SaveFlag::automatic);
        }
        else
            Require(ScenarioSave(state, save, flags) != 0, "Native save failed");
        Paths::End();
        Ui::ProvisionalElementsRestore();
        Require(!FindPath(TileCoordsXYZ{ previewLocation }), "Tool exit left ghost");
        Require(context->LoadParkFromFile(save), "Native reload failed");
        Require(!FindPath(TileCoordsXYZ{ previewLocation }), "Preview leaked into save");
        path = FindPath(TileCoordsXYZ{ location });
        Require(path && path->isQueue(), "Queue not preserved in save");
        if (argc == 4)
            Require(path->getEdges() == savedEdges, "Ghost neighbour connections leaked into autosave");
        elevated = FindPath(TileCoordsXYZ{ movedLocation });
        Require(elevated && elevated->isSloped() && elevated->getSlopeDirection() == 1, "Elevated slope not preserved in save");
        // Removal ignores the construction height/slope overrides and targets the selected path.
        info.Loc = movedTile;
        info.Element = reinterpret_cast<TileElement*>(elevated);
        info.interactionType = ViewportInteractionItem::footpath;
        Paths::Begin(false);
        Paths::UpdateTargetFromInfo(info);
        Require(Paths::ChangeHeight(1), "Removal override fixture failed");
        Paths::ToggleRemove();
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(
            result.error == GameActions::Status::ok && !FindPath(TileCoordsXYZ{ movedLocation }), "Elevated removal failed");
        path = FindPath(TileCoordsXYZ{ location });
        info.Loc = tile;
        info.Element = reinterpret_cast<TileElement*>(path);
        Paths::Begin(false, true);
        Paths::UpdateTargetFromInfo(info);
        result = Paths::Action(true);
        GameActions::ProcessQueue(state);
        Require(result.error == GameActions::Status::ok, "Removal failed");
        Require(FindPath(TileCoordsXYZ{ location }) == nullptr, "Removed path remains");
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
        Paths::UpdatePreview();
        const CoordsXYZ unownedLocation{ unowned, surface->getBaseZ() };
        Require(!FindPath(TileCoordsXYZ{ unownedLocation }), "Unowned ghost accepted");
        surface->setOwnership({ OwnershipFlag::landOwned });
        Paths::ClearPreview();
        Paths::UpdatePreview();
        Require(FindPath(TileCoordsXYZ{ unownedLocation })->isGhost(), "Owned preview missing");
        MapGetSurfaceElementAt(unowned)->setOwnership({});
        ++state.currentTicks;
        Paths::UpdatePreview();
        Require(!FindPath(TileCoordsXYZ{ unownedLocation }), "Ownership change stranded ghost");
        MapGetSurfaceElementAt(unowned)->setOwnership({ OwnershipFlag::landOwned });
        ++state.currentTicks;
        Paths::UpdatePreview();
        const auto validCash = state.park.cash;
        state.park.cash = 0;
        Paths::UpdatePreview();
        Require(!FindPath(TileCoordsXYZ{ unownedLocation }), "Insufficient funds left ghost");
        state.park.cash = validCash;
        Paths::UpdatePreview();
        gGamePaused = 1;
        Paths::UpdatePreview();
        Require(!FindPath(TileCoordsXYZ{ unownedLocation }), "Paused ghost accepted");
        Require(Paths::Action(false).error == GameActions::Status::gamePaused, "Paused placement accepted");
        const auto priorities = state.researchPriorities;
        GameActions::ParkSetResearchFundingAction funding(priorities, 3);
        Require(GameActions::Execute(&funding, state).error == GameActions::Status::ok, "Funding action rejected while paused");
        GameActions::ProcessQueue(state);
        Require(state.researchFundingLevel == 3 && state.researchPriorities == priorities, "Funding action failed");
        gGamePaused = 0;
        Paths::Begin(false);
        info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(unowned));
        Paths::UpdateTargetFromInfo(info);
        Paths::UpdatePreview();
        Paths::UpdateTargetFromInfo({});
        Require(!FindPath(TileCoordsXYZ{ unownedLocation }), "Invalid target left ghost");
        Require(Paths::Action(true).error != GameActions::Status::ok, "Invalid target accepted");
        Paths::End();
        Require(!Paths::IsActive(), "Tool remained active");
        if (argc == 4)
        {
            // Exercise real UI entry points, not just the shared cleanup helper.
            info.Loc = previewTile;
            info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(previewTile));
            Paths::Begin(false);
            Paths::UpdateTargetFromInfo(info);
            Paths::UpdatePreview();
            Require(FindPath(TileCoordsXYZ{ previewLocation }), "UI lifecycle fixture missing");
            Ui::Windows::HandheldPathOptionsOpen();
            Require(Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }), "Options did not suspend preview");
            Ui::Windows::HandheldMenuBack();
            Paths::UpdatePreview();
            Require(FindPath(TileCoordsXYZ{ previewLocation }), "Options return did not restore preview");
            Ui::Windows::HandheldMenuOpen();
            Require(!FindPath(TileCoordsXYZ{ previewLocation }), "Pause menu left ghost");
            Ui::Windows::HandheldMenuBack();
            Paths::UpdatePreview();
            Require(FindPath(TileCoordsXYZ{ previewLocation }), "Pause return did not restore preview");
            Require(context->LoadParkFromFile(save), "UI reload failed");
            Require(!Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }), "Park load retained preview/tool");
        }
        std::cout << "Native ghost lifecycle / cash / preview save safety / path place / queue / raised slope / height "
                     "persistence / save / reload / remove / ownership / pause and research checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
