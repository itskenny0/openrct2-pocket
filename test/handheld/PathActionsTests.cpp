// SPDX-License-Identifier: GPL-3.0-only
#include <SDL.h>
#include <array>
#include <cstring>
#include <iostream>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Finances.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/SceneryRemoval.h>
#include <openrct2-ui/handheld/ShopManagement.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2-ui/handheld/Surface.h>
#include <openrct2-ui/handheld/TreePicker.h>
#include <openrct2-ui/handheld/Trees.h>
#include <openrct2-ui/handheld/Water.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/actions/park/ParkSetEntranceFeeAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/localisation/CurrencyTypes.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/windows/Intent.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/MapOwnership.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
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
static void TestWater(IContext& context, const std::string& profile)
{
    namespace Water = Ui::Handheld::Water;
    auto& state = getGameState();
    const CoordsXY tile{ 224, 64 }, neighbour{ 256, 64 };
    auto* surface = MapGetSurfaceElementAt(tile);
    surface->setOwnership({ OwnershipFlag::landOwned });
    const auto landHeight = surface->getBaseZ();
    const auto neighbourWater = MapGetSurfaceElementAt(neighbour)->getWaterHeight();
    const auto initialCash = state.park.cash;
    Require(Water::Action({}, true, true).error == GameActions::Status::invalidParameters, "Missing water target accepted");
    Require(surface->getWaterHeight() == 0, "Water fixture not dry");
    auto query = Water::Action(tile, true, false);
    Require(query.error == GameActions::Status::ok && query.cost > 0, "Water raise query failed");
    Require(state.park.cash == initialCash && surface->getWaterHeight() == 0, "Water query changed park");
    auto result = Water::Action(tile, true, true);
    Require(result.error == GameActions::Status::ok && surface->getWaterHeight() == 0, "Water action bypassed queue");
    GameActions::ProcessQueue(state);
    const auto firstHeight = surface->getWaterHeight();
    Require(firstHeight > landHeight && state.park.cash == initialCash - query.cost, "Water raise/cash incorrect");
    const auto beforeSecond = state.park.cash;
    query = Water::Action(tile, true, false);
    Require(Water::Action(tile, true, true).error == GameActions::Status::ok, "Second water raise failed");
    GameActions::ProcessQueue(state);
    Require(
        surface->getWaterHeight() > firstHeight && state.park.cash == beforeSecond - query.cost, "Second raise/cash incorrect");
    const auto beforeLower = state.park.cash;
    query = Water::Action(tile, false, false);
    Require(Water::Action(tile, false, true).error == GameActions::Status::ok, "Water lower failed");
    GameActions::ProcessQueue(state);
    Require(
        surface->getWaterHeight() == firstHeight && state.park.cash == beforeLower - query.cost, "Water lower/cash incorrect");
    Require(
        surface->getBaseZ() == landHeight && MapGetSurfaceElementAt(neighbour)->getWaterHeight() == neighbourWater,
        "Water changed land or neighbouring tile");
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/water.park";
    const auto savedCash = state.park.cash;
    Require(ScenarioSave(state, save, flags) != 0 && context.LoadParkFromFile(save), "Water save/reload failed");
    surface = MapGetSurfaceElementAt(tile);
    Require(
        surface->getWaterHeight() == firstHeight && surface->getBaseZ() == landHeight && state.park.cash == savedCash,
        "Water height/land/cash not preserved");
    query = Water::Action(tile, false, false);
    Require(Water::Action(tile, false, true).error == GameActions::Status::ok, "Water removal failed");
    GameActions::ProcessQueue(state);
    Require(surface->getWaterHeight() == 0 && state.park.cash == savedCash - query.cost, "Water removal/cash incorrect");
    const auto validCash = state.park.cash;
    surface->setOwnership({});
    Require(
        Water::Action(tile, true, false).error == GameActions::Status::disallowed
            && Water::Action(tile, true, true).error == GameActions::Status::disallowed,
        "Unowned water accepted");
    surface->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    Require(
        Water::Action(tile, true, false).error == GameActions::Status::insufficientFunds
            && Water::Action(tile, true, true).error == GameActions::Status::insufficientFunds,
        "Unaffordable water accepted");
    state.park.cash = validCash;
    gGamePaused = 1;
    Require(
        Water::Action(tile, true, false).error == GameActions::Status::gamePaused
            && Water::Action(tile, true, true).error == GameActions::Status::gamePaused,
        "Paused water accepted");
    gGamePaused = 0;
    // Native clearance: water cannot be raised through an actual ground path.
    InteractionInfo info;
    info.Loc = tile;
    info.Element = reinterpret_cast<TileElement*>(surface);
    info.interactionType = ViewportInteractionItem::terrain;
    Paths::Begin(false);
    Paths::UpdateTargetFromInfo(info);
    Require(Paths::Action(true).error == GameActions::Status::ok, "Clearance path fixture failed");
    GameActions::ProcessQueue(state);
    Paths::End();
    const auto blockedCash = state.park.cash;
    Require(
        Water::Action(tile, true, false).error != GameActions::Status::ok
            && Water::Action(tile, true, true).error != GameActions::Status::ok,
        "Water ignored path clearance");
    GameActions::ProcessQueue(state);
    Require(
        MapGetSurfaceElementAt(tile)->getWaterHeight() == 0 && state.park.cash == blockedCash, "Rejected water changed park");
    Ui::Handheld::EndTerrain();
    std::cout << "Native water raise/lower / exact cash / single tile / clearance / ownership / funds / pause / save/reload "
                 "checks passed\n";
}
static void TestSurface(IContext& context, const std::string& profile)
{
    namespace Surface = Ui::Handheld::Surface;
    auto& objects = context.GetObjectManager();
    Require(objects.LoadObject("rct2.terrain_surface.grass"), "Grass surface missing");
    Require(objects.LoadObject("rct2.terrain_surface.sand"), "Sand surface missing");
    auto& state = getGameState();
    const CoordsXY tile{ 320, 64 }, neighbour{ 352, 64 };
    auto* surface = MapGetSurfaceElementAt(tile);
    surface->setOwnership({ OwnershipFlag::landOwned });
    surface->setSlope(1);
    surface->setWaterHeight(surface->getBaseZ() + 16);
    surface->setPrimarySurfaceColour(Drawing::Colour::brightRed);
    surface->setPrimaryEdgeColour(Drawing::Colour::brightPurple);
    const auto originalStyle = surface->getSurfaceObjectIndex();
    const auto height = surface->getBaseZ(), water = surface->getWaterHeight();
    const auto slope = surface->getSlope();
    const auto edge = surface->getEdgeObjectIndex();
    const auto edgeColour = surface->getPrimaryEdgeColour(), colour = surface->getPrimarySurfaceColour();
    const auto neighbourStyle = MapGetSurfaceElementAt(neighbour)->getSurfaceObjectIndex();
    auto selected = Surface::Next(kObjectEntryIndexNull);
    const auto first = selected;
    int count = 0;
    do
    {
        Require(objects.GetLoadedObject<TerrainSurfaceObject>(selected), "Cycle selected unloaded surface");
        ++count;
        selected = Surface::Next(selected);
        Require(count <= kMaxTerrainSurfaceObjects, "Surface cycle failed to wrap");
    } while (selected != first);
    Require(count >= 2, "Surface cycle fixture incomplete");
    selected = first;
    while (selected == originalStyle
           || objects.GetLoadedObject<TerrainSurfaceObject>(selected)->Flags.has(TerrainSurfaceFlag::hasPrimaryColour))
        selected = Surface::Next(selected);
    const auto initialCash = state.park.cash;
    auto query = Surface::Action(tile, selected, false);
    Require(query.error == GameActions::Status::ok && query.cost > 0, "Paint query failed");
    Require(surface->getSurfaceObjectIndex() == originalStyle && state.park.cash == initialCash, "Paint query changed park");
    Require(Surface::Action(tile, selected, true).error == GameActions::Status::ok, "Paint execution failed");
    Require(surface->getSurfaceObjectIndex() == originalStyle && state.park.cash == initialCash, "Paint bypassed queue");
    GameActions::ProcessQueue(state);
    auto preserved = [&]() {
        return surface->getBaseZ() == height && surface->getSlope() == slope && surface->getWaterHeight() == water
            && surface->getEdgeObjectIndex() == edge && surface->getPrimaryEdgeColour() == edgeColour
            && surface->getPrimarySurfaceColour() == colour;
    };
    Require(
        surface->getSurfaceObjectIndex() == selected && state.park.cash == initialCash - query.cost && preserved(),
        "Paint/cash/metadata incorrect");
    Require(MapGetSurfaceElementAt(neighbour)->getSurfaceObjectIndex() == neighbourStyle, "Paint changed neighbour");
    const auto paintedCash = state.park.cash;
    Require(
        Surface::Action(tile, selected, false).cost == 0
            && Surface::Action(tile, selected, true).error == GameActions::Status::ok,
        "Same-style paint rejected");
    GameActions::ProcessQueue(state);
    Require(state.park.cash == paintedCash && preserved(), "Repeated paint charged or changed metadata");
    Require(Surface::Action({}, first, true).error == GameActions::Status::invalidParameters, "Missing paint target accepted");
    Require(
        Surface::Action(CoordsXY{ -32, 64 }, first, true).error == GameActions::Status::invalidParameters,
        "Off-map paint target accepted");
    Require(
        Surface::Action(CoordsXY{ 0, 64 }, first, true).error == GameActions::Status::invalidParameters,
        "Border paint target accepted");
    Require(
        Surface::Action(tile, kObjectEntryIndexNull, true).error == GameActions::Status::invalidParameters,
        "Missing paint surface accepted");
    ObjectEntryIndex unloaded = 0;
    while (objects.GetLoadedObject<TerrainSurfaceObject>(unloaded))
        ++unloaded;
    Require(
        Surface::Action(tile, unloaded, true).error == GameActions::Status::invalidParameters,
        "Unloaded paint surface accepted");
    surface->setOwnership({});
    Require(
        Surface::Action(tile, originalStyle, false).cost == 0
            && Surface::Action(tile, originalStyle, true).error == GameActions::Status::ok,
        "Native unowned no-op changed");
    GameActions::ProcessQueue(state);
    Require(
        surface->getSurfaceObjectIndex() == selected && state.park.cash == paintedCash && preserved(),
        "Unowned paint changed park");
    surface->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    Require(
        Surface::Action(tile, originalStyle, true).error == GameActions::Status::insufficientFunds,
        "Unaffordable paint accepted");
    state.park.cash = paintedCash;
    gGamePaused = 1;
    Require(Surface::Action(tile, originalStyle, true).error == GameActions::Status::gamePaused, "Paused paint accepted");
    gGamePaused = 0;
    state.park.flags.set(ParkFlag::forbidLandscapeChanges);
    Require(
        Surface::Action(tile, originalStyle, true).error == GameActions::Status::disallowed, "Forbidden landscaping accepted");
    state.park.flags.unset(ParkFlag::forbidLandscapeChanges);
    GameActions::ProcessQueue(state);
    Require(
        surface->getSurfaceObjectIndex() == selected && state.park.cash == paintedCash && preserved(),
        "Rejected paint changed park");
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/surface.park";
    Require(ScenarioSave(state, save, flags) != 0 && context.LoadParkFromFile(save), "Paint save/reload failed");
    surface = MapGetSurfaceElementAt(tile);
    Require(
        surface->getSurfaceObjectIndex() == selected && state.park.cash == paintedCash && preserved(),
        "Paint metadata/cash not preserved after reload");
    Ui::Handheld::EndTerrain();
    std::cout << "Native surface paint / exact cash / loaded cycle / no-op / invalid target / ownership / funds / pause / "
                 "height-edge-water / save-reload checks passed\n";
}
static void TestSceneryRemoval(IContext& context, const std::string& profile)
{
    namespace Removal = Ui::Handheld::SceneryRemoval;
    auto& objects = context.GetObjectManager();
    auto* object = objects.LoadObject("rct2.scenery_small.tcl");
    Require(object, "Scenery fixture object missing");
    const auto index = objects.GetLoadedObjectEntryIndex(object);
    auto& state = getGameState();
    const CoordsXY tile{ 384, 64 }, neighbour{ 416, 64 };
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    MapGetSurfaceElementAt(neighbour)->setOwnership({ OwnershipFlag::landOwned });
    const auto z = MapGetSurfaceElementAt(tile)->getBaseZ();
    auto insert = [&](CoordsXY xy, int height, uint8_t quadrant, bool ghost = false) {
        auto* element = TileElementInsert<SmallSceneryElement>({ xy, height }, 15);
        Require(element, "Scenery fixture insert failed");
        element->setEntryIndex(index);
        element->setSceneryQuadrant(quadrant);
        element->setClearanceZ(height + 8);
        element->setGhost(ghost);
    };
    // Deliberately exercise native identities sharing a tile/type or a quadrant.
    insert(tile, z, 0);
    insert(tile, z, 1);
    insert(tile, z + 8, 0);
    insert(neighbour, z, 0);
    Removal::Target target{ { tile, z }, 0, index };
    Removal::Target sibling{ { tile, z }, 1, index }, upper{ { tile, z + 8 }, 0, index }, next{ { neighbour, z }, 0, index };
    InteractionInfo info;
    info.Loc = tile;
    info.Element = Removal::Find(target)->as<TileElement>();
    info.interactionType = ViewportInteractionItem::scenery;
    auto selected = Removal::FromInfo(info);
    Require(selected && selected->location.z == z && selected->quadrant == 0, "Scenery hit target incorrect");
    auto metadata = [](SmallSceneryElement* e) {
        return std::array<int, 7>{ e->getBaseZ(),           e->getClearanceZ(), e->getEntryIndex(),
                                   e->getSceneryQuadrant(), e->getAge(),        static_cast<int>(e->getPrimaryColour()),
                                   e->getDirection() };
    };
    const auto siblingBefore = metadata(Removal::Find(sibling)), upperBefore = metadata(Removal::Find(upper)),
               nextBefore = metadata(Removal::Find(next));
    auto query = Removal::Action(selected, false);
    const auto initialCash = state.park.cash;
    Require(query.error == GameActions::Status::ok && query.cost > 0 && Removal::Find(target), "Scenery query failed");
    Require(state.park.cash == initialCash, "Removal query spent money");
    Require(
        Removal::Action(selected, true).error == GameActions::Status::ok && Removal::Find(target), "Removal bypassed queue");
    GameActions::ProcessQueue(state);
    Require(!Removal::Find(target) && state.park.cash == initialCash - query.cost, "Exact removal/cash incorrect");
    Require(Removal::Find(sibling) && Removal::Find(upper) && Removal::Find(next), "Removal changed another element");
    Require(Removal::Action(selected, true).error != GameActions::Status::ok, "Stale removal target accepted");
    Require(Removal::Action({}, true).error != GameActions::Status::ok, "Missing removal target accepted");
    info.Element = MapGetSurfaceElementAt(tile)->as<TileElement>();
    for (auto type : { ViewportInteractionItem::terrain, ViewportInteractionItem::ride, ViewportInteractionItem::footpath,
                       ViewportInteractionItem::largeScenery })
    {
        info.interactionType = type;
        Require(!Removal::FromInfo(info), "Unsupported removal hit accepted");
    }
    info.interactionType = ViewportInteractionItem::scenery;
    Require(!Removal::FromInfo(info), "Wrong element type accepted as scenery");
    info.Element = nullptr;
    Require(!Removal::FromInfo(info), "Null removal hit accepted");
    insert(tile, z, 0, true);
    auto* ghost = Removal::Find(target);
    info.Element = ghost->as<TileElement>();
    Require(
        !Removal::FromInfo(info) && Removal::Action(target, true).error != GameActions::Status::ok, "Removal accepted ghost");
    // A native non-ghost action can match a preceding ghost: reject ambiguous hits.
    insert(tile, z, 0);
    SmallSceneryElement* real = nullptr;
    for (auto* e : TileElementsView<SmallSceneryElement>(tile))
        if (!e->isGhost() && e->getBaseZ() == z && e->getSceneryQuadrant() == 0)
            real = e;
    info.Element = real->as<TileElement>();
    Require(!Removal::FromInfo(info), "Ambiguous real hit could remove ghost");
    Require(Removal::Find(target)->isGhost(), "Ghost changed by rejection");
    TileElementRemove(Removal::Find(target)->as<TileElement>());
    Require(Removal::Find(target) && !Removal::Find(target)->isGhost(), "Ambiguity fixture failed");
    const auto validCash = state.park.cash;
    MapGetSurfaceElementAt(tile)->setOwnership({});
    Require(Removal::Action(target, true).error != GameActions::Status::ok, "Unowned removal accepted");
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    Require(Removal::Action(target, true).error == GameActions::Status::insufficientFunds, "Unaffordable removal accepted");
    state.park.cash = validCash;
    gGamePaused = 1;
    Require(Removal::Action(target, true).error == GameActions::Status::gamePaused, "Paused removal accepted");
    gGamePaused = 0;
    state.park.flags.set(ParkFlag::forbidTreeRemoval);
    Require(Removal::Action(target, true).error != GameActions::Status::ok, "Forbidden tree removal accepted");
    state.park.flags.unset(ParkFlag::forbidTreeRemoval);
    GameActions::ProcessQueue(state);
    Require(Removal::Find(target) && state.park.cash == validCash, "Rejected removal changed park");
    // Remove the restored fixture target so the save proves permanent deletion.
    query = Removal::Action(target, false);
    Require(Removal::Action(target, true).error == GameActions::Status::ok, "Second removal failed");
    GameActions::ProcessQueue(state);
    Require(state.park.cash == validCash - query.cost, "Second removal charge incorrect");
    // A real loaded refund-priced object proves signed native pricing, even at zero cash.
    auto* refundObject = objects.LoadObject("rct2.scenery_small.corroof");
    Require(refundObject, "Refund scenery fixture missing");
    const CoordsXY refundTile{ 384, 96 };
    auto* refundSurface = MapGetSurfaceElementAt(refundTile);
    refundSurface->setOwnership({ OwnershipFlag::landOwned });
    const Removal::Target refund{ { refundTile, refundSurface->getBaseZ() },
                                  0,
                                  objects.GetLoadedObjectEntryIndex(refundObject) };
    auto* roof = TileElementInsert<SmallSceneryElement>(refund.location, 15);
    Require(roof, "Refund fixture insert failed");
    roof->setEntryIndex(refund.object);
    roof->setSceneryQuadrant(0);
    roof->setClearanceZ(refund.location.z + 8);
    state.park.cash = 0;
    query = Removal::Action(refund, false);
    Require(query.error == GameActions::Status::ok && query.cost < 0, "Native refund query failed");
    Require(Removal::Action(refund, true).error == GameActions::Status::ok, "Refund removal rejected at zero cash");
    GameActions::ProcessQueue(state);
    Require(!Removal::Find(refund) && state.park.cash == -query.cost, "Native refund cash incorrect");
    const auto savedCash = state.park.cash;
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/scenery-removal.park";
    Require(ScenarioSave(state, save, flags) != 0 && context.LoadParkFromFile(save), "Scenery removal save/reload failed");
    Require(
        !Removal::Find(target) && Removal::Find(sibling) && Removal::Find(upper) && Removal::Find(next)
            && state.park.cash == savedCash,
        "Removed/surviving scenery or cash not preserved");
    Require(
        metadata(Removal::Find(sibling)) == siblingBefore && metadata(Removal::Find(upper)) == upperBefore
            && metadata(Removal::Find(next)) == nextBefore,
        "Neighbour metadata changed");
    Ui::Handheld::EndTerrain();
    std::cout << "Native scenery removal / exact target-cash / unsupported-stale hits / ghost protection / "
                 "ownership-funds-pause-tree rules / neighbours / save-reload checks passed\n";
}
static void TestTrees(IContext& context, const std::string& profile)
{
    namespace Trees = Ui::Handheld::Trees;
    auto& state = getGameState();
    auto& objects = context.GetObjectManager();
    // Fixture-only object/research setup. Production never loads objects or unlocks research.
    auto* object = objects.LoadObject("rct2.scenery_small.tcl");
    Require(object, "Tree fixture missing");
    const auto index = objects.GetLoadedObjectEntryIndex(object);
    const ScenerySelection selection{ SCENERY_TYPE_SMALL, index };
    ScenerySetInvented(selection);
    Require(Trees::Eligible(index), "Full-tile researched tree excluded");
    const auto roofIndex = objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.scenery_small.corroof"));
    Require(!Trees::Eligible(roofIndex) && !Trees::Eligible(kObjectEntryIndexNull), "Non-tree/unloaded tree included");
    Require(Trees::Next(index) == index, "Single eligible tree cycle failed");
    auto* secondObject = objects.LoadObject("rct2.scenery_small.tpm");
    Require(secondObject, "Second tree fixture missing");
    const auto secondIndex = objects.GetLoadedObjectEntryIndex(secondObject);
    const ScenerySelection secondSelection{ SCENERY_TYPE_SMALL, secondIndex };
    ScenerySetInvented(secondSelection);
    Require(Trees::Next(index) == secondIndex && Trees::Next(secondIndex) == index, "Tree cycle failed to wrap/skip non-trees");
    ScenerySetNotInvented(secondSelection);
    const CoordsXY tile{ 448, 96 }, neighbour{ 416, 96 };
    auto* surface = MapGetSurfaceElementAt(tile);
    surface->setOwnership({ OwnershipFlag::landOwned });
    surface->setBaseZ(surface->getBaseZ() + 16);
    surface->setClearanceZ(surface->getBaseZ());
    const auto land = surface->getBaseZ();
    auto snapshot = [](CoordsXY xy) {
        std::vector<TileElement> result;
        for (auto* e : TileElementsView(xy))
            result.push_back(*e);
        return result;
    };
    auto same = [](const std::vector<TileElement>& a, const std::vector<TileElement>& b) {
        return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size() * sizeof(TileElement)) == 0;
    };
    const auto neighbourBefore = snapshot(neighbour);
    auto ground = [&]() {
        const auto* e = MapGetSurfaceElementAt(tile);
        return std::array<int, 9>{ e->getBaseZ(),
                                   e->getSlope(),
                                   e->getWaterHeight(),
                                   e->getSurfaceObjectIndex(),
                                   e->getEdgeObjectIndex(),
                                   static_cast<int>(e->getPrimarySurfaceColour()),
                                   static_cast<int>(e->getPrimaryEdgeColour()),
                                   e->getGrassLength(),
                                   e->getParkFences() };
    };
    const auto groundBefore = ground();
    auto count = [&](CoordsXY xy) {
        int n = 0;
        for (auto* e : TileElementsView<SmallSceneryElement>(xy))
            n += !e->isGhost();
        return n;
    };
    state.park.cash = 100000;
    const auto originalCash = state.park.cash;
    auto query = Trees::Action(tile, index, false);
    Require(query.error == GameActions::Status::ok && query.cost > 0, "Tree query failed");
    Require(count(tile) == 0 && state.park.cash == originalCash, "Tree query changed park");
    ScenerySetNotInvented(selection);
    Require(!Trees::Eligible(index) && Trees::Next(index) == kObjectEntryIndexNull, "Unresearched tree available");
    Require(Trees::Action(tile, index, true).error != GameActions::Status::ok, "Unresearched tree placed");
    ScenerySetInvented(selection);
    SetSceneryItemRestricted(selection, true);
    Require(
        !Trees::Eligible(index) && Trees::Action(tile, index, true).error != GameActions::Status::ok, "Restricted tree placed");
    SetSceneryItemRestricted(selection, false);
    Require(
        Trees::Action({}, index, true).error != GameActions::Status::ok
            && Trees::Action(CoordsXY{ 0, 96 }, index, true).error != GameActions::Status::ok
            && Trees::Action(CoordsXY{ 449, 96 }, index, true).error != GameActions::Status::ok,
        "Invalid tree target accepted");
    surface = MapGetSurfaceElementAt(tile);
    surface->setOwnership({});
    Require(
        Trees::Action(tile, index, false).error != GameActions::Status::ok
            && Trees::Action(tile, index, true).error != GameActions::Status::ok,
        "Unowned tree placement accepted");
    surface->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    Require(
        Trees::Action(tile, index, true).error == GameActions::Status::insufficientFunds, "Unfunded tree placement accepted");
    state.park.cash = originalCash;
    gGamePaused = 1;
    Require(Trees::Action(tile, index, true).error == GameActions::Status::gamePaused, "Paused tree placement accepted");
    gGamePaused = 0;
    Require(Trees::Action(tile, index, true).error == GameActions::Status::ok, "Tree placement failed");
    Require(count(tile) == 0 && state.park.cash == originalCash, "Tree bypassed native queue");
    GameActions::ProcessQueue(state);
    Require(count(tile) == 1 && state.park.cash == originalCash - query.cost, "Tree count/cash incorrect");
    auto* placed = *TileElementsView<SmallSceneryElement>(tile).begin();
    Require(
        placed->getEntryIndex() == index && placed->getBaseZ() == land && !placed->isGhost(), "Tree identity/ground incorrect");
    Require(groundBefore == ground() && same(neighbourBefore, snapshot(neighbour)), "Tree changed ground or neighbours");
    const auto placedBefore = *placed;
    const auto placedCash = state.park.cash;
    Require(
        Trees::Action(tile, index, false).error != GameActions::Status::ok
            && Trees::Action(tile, index, true).error != GameActions::Status::ok,
        "Overlapping tree accepted");
    GameActions::ProcessQueue(state);
    Require(count(tile) == 1 && state.park.cash == placedCash, "Invalid placement changed cash/tree count");
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/trees.park";
    Require(ScenarioSave(state, save, flags) != 0 && context.LoadParkFromFile(save), "Tree save/reload failed");
    placed = *TileElementsView<SmallSceneryElement>(tile).begin();
    Require(
        count(tile) == 1 && state.park.cash == placedCash
            && std::memcmp(&placedBefore, placed, sizeof(SmallSceneryElement)) == 0
            && same(neighbourBefore, snapshot(neighbour)),
        "Tree/cash/neighbours not preserved on reload");
    Require(groundBefore == ground(), "Ground changed on reload");
    Ui::Handheld::EndTerrain();
    std::cout << "Native tree placement / availability-research-restrictions / one tree-exact cash / ground-neighbours / "
                 "invalid-ownership-clearance-funds-pause / save-reload checks passed\n";
}

static void TestShops(IContext& context, const std::string& profile, bool realUi)
{
    namespace H = Ui::Handheld;
    namespace S = H::Shops;
    namespace W = Ui::Windows;
    auto& state = getGameState();
    H::EndTerrain();
    auto& objects = context.GetObjectManager();
    // Test fixtures only. The production picker neither loads nor invents anything.
    for (const auto* name : { "burgb", "drnks", "infok", "atm1", "faid1", "coffs", "hatst", "souvs" })
    {
        auto* object = objects.LoadObject(std::string("rct2.ride.") + name);
        Require(object, "Shop fixture missing");
        const auto index = objects.GetLoadedObjectEntryIndex(object);
        ResearchInsertRideEntry(index, true);
        RideEntrySetInvented(index);
        RideTypeSetInvented(ShopPlacement::Type(index));
    }
    auto entries = S::Entries();
    Require(entries.size() > H::ObjectPicker::pageSize, "Shop paging fixture missing");
    for (const auto& e : entries)
        Require(
            e.identity.type == ObjectType::ride && S::Eligible(e.identity.index)
                && GetRideTypeDescriptor(ShopPlacement::Type(e.identity.index)).flags.has(RtdFlag::isShopOrFacility),
            "Shop picker included unsupported/unresearched object");
    H::ObjectPicker picker;
    picker.setEntries(entries, entries.front().identity);
    picker.move(0, 1);
    picker.move(0, 1);
    Require(picker.page() == 1 && picker.focus() == 6, "Shared shop grid paging failed");
    picker.move(0, -1);
    Require(picker.page() == 0 && picker.focus() == 3, "Shop grid backward paging failed");
    const auto index = entries.front().identity.index;
    const CoordsXY tile{ 448, 224 }, next{ 416, 224 }, neighbour{ 448, 256 };
    for (auto xy : { tile, next, neighbour })
        MapGetSurfaceElementAt(xy)->setOwnership({ OwnershipFlag::landOwned });
    auto tracks = [](CoordsXY xy, bool ghost) {
        int n = 0;
        for (auto* e : TileElementsView<TrackElement>(xy))
            n += e->isGhost() == ghost;
        return n;
    };
    auto rides = [&]() {
        size_t n = 0;
        for (const auto& ride : state.rides)
            n += !ride.id.IsNull();
        return n;
    };
    auto pathAction = GameActions::FootpathPlaceAction(
        { neighbour, MapGetSurfaceElementAt(neighbour)->getBaseZ() }, {},
        objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.footpath_surface.tarmac")),
        objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.footpath_railings.wood")));
    Require(GameActions::Execute(&pathAction, state).error == GameActions::Status::ok, "Adjacent shop path fixture failed");
    GameActions::ProcessQueue(state);
    const auto pathBefore = *MapGetFootpathElement({ neighbour, MapGetSurfaceElementAt(neighbour)->getBaseZ() });
    const auto baselineRides = rides();
    const auto baselineRange = state.ridesEndOfUsedRange;
    const auto baselineCash = state.park.cash = 1000000;
    const auto neighbourBefore = *MapGetSurfaceElementAt(neighbour);
    auto query = S::Action(tile, index, 0, false);
    Require(query.error == GameActions::Status::ok && query.cost > 0, "Shop query failed");
    Require(
        rides() == baselineRides && state.ridesEndOfUsedRange == baselineRange && state.park.cash == baselineCash,
        "Shop query stranded temporary ride/changed cash");
    S::UpdatePreview(tile, index, 0);
    Require(
        tracks(tile, true) == 1 && tracks(tile, false) == 0 && rides() == baselineRides + 1 && state.park.cash == baselineCash,
        "Native shop ghost/count/cash failed");
    auto* first = *TileElementsView<TrackElement>(tile).begin();
    first->setColourScheme(static_cast<RideColourScheme>(1));
    S::UpdatePreview(tile, index, 0);
    Require(
        tracks(tile, true) == 1 && (*TileElementsView<TrackElement>(tile).begin())->getColourScheme() == 1,
        "Unchanged shop ghost unnecessarily replaced");
    Require(S::Action(tile, index, 0, false).cost == query.cost, "Shop ghost changed native price");
    S::UpdatePreview(tile, index, 1);
    Require(
        tracks(tile, true) == 1 && (*TileElementsView<TrackElement>(tile).begin())->getDirection() == 1,
        "Shop rotation failed");
    S::UpdatePreview(next, index, 2);
    Require(tracks(tile, true) == 0 && tracks(next, true) == 1, "Shop target change left old ghost");
    Ui::ProvisionalElementsRemove();
    Ui::ProvisionalElementsRemove();
    Require(tracks(next, true) == 0 && rides() == baselineRides, "Shop provisional suspension left ride/ghost");
    Ui::ProvisionalElementsRestore();
    Require(tracks(next, true) == 0, "Nested shop suspension restored early");
    Ui::ProvisionalElementsRestore();
    Require(tracks(next, true) == 1, "Shop provisional restore failed");
    S::ClearPreview();
    Require(rides() == baselineRides, "Shop cleanup left empty native ride");
    Require(
        std::memcmp(
            &pathBefore, MapGetFootpathElement({ neighbour, MapGetSurfaceElementAt(neighbour)->getBaseZ() }),
            sizeof(pathBefore))
            == 0,
        "Shop ghost cleanup changed neighbouring path edges/metadata");
    S::UpdatePreview({}, index, 0);
    Require(tracks(next, true) == 0 && rides() == baselineRides, "Invalid target left shop ghost");
    MapGetSurfaceElementAt(tile)->setOwnership({});
    Require(S::Action(tile, index, 0, true).error != GameActions::Status::ok, "Unowned shop accepted");
    S::UpdatePreview(tile, index, 0);
    Require(tracks(tile, true) == 0, "Unowned shop preview accepted");
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 0;
    Require(S::Action(tile, index, 0, true).error == GameActions::Status::insufficientFunds, "Shop funds bypassed");
    S::UpdatePreview(tile, index, 0);
    Require(tracks(tile, true) == 0, "Unaffordable shop ghost");
    state.park.cash = baselineCash;
    gGamePaused = 1;
    S::UpdatePreview(tile, index, 0);
    Require(tracks(tile, true) == 0, "Paused shop ghost misleading");
    Require(S::Action(tile, index, 0, true).error != GameActions::Status::ok, "Paused shop placed");
    gGamePaused = 0;
    S::UpdatePreview(tile, index, 0);
    SetEveryRideEntryNotInvented();
    Require(!S::Eligible(index) && S::Entries().empty(), "Shop research filtering failed");
    Require(
        S::Action(tile, index, 0, true).error != GameActions::Status::ok && tracks(tile, true) == 0,
        "Unresearched shop confirmed/ghost stranded");
    for (auto& e : entries)
        RideEntrySetInvented(e.identity.index);
    SetEveryRideTypeNotInvented();
    Require(S::Entries().empty(), "Unresearched ride types leaked into picker");
    for (auto& e : entries)
        RideTypeSetInvented(ShopPlacement::Type(e.identity.index));
    Require(!S::Eligible(kObjectEntryIndexNull), "Unloaded shop accepted");
    if (realUi)
    {
        W::HandheldBuildOpen();
        for (int i = 0; i < 7; ++i)
            W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        Require(W::HandheldObjectPickerIsOpen() && !H::IsShopToolActive(), "Build did not open shop picker");
        W::HandheldMenuBack();
        Require(!W::HandheldObjectPickerIsOpen() && !H::IsShopToolActive(), "Initial shop cancellation activated placement");
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        Require(H::SetShopSelection(index), "Shop selection failed");
        S::UpdatePreview(tile, index, 0);
        W::HandheldShopPickerOpen();
        Require(tracks(tile, true) == 0 && rides() == baselineRides && gGamePaused, "Shop picker retained ghost/ride");
        W::HandheldObjectPickerMove(1, 0);
        W::HandheldMenuBack();
        Require(H::ShopSelection() == index && !gGamePaused, "Shop cancel changed selection/pause");
        W::HandheldShopPickerOpen();
        H::ControllerInput input;
        H::Frame frame;
        frame.actions = { H::Action::openMenu };
        H::HandleMenuInput(input, frame);
        Require(!W::HandheldObjectPickerIsOpen() && H::ShopSelection() == index && gGamePaused, "Shop picker Start failed");
        W::HandheldMenuConfirm();
        W::HandheldShopPickerOpen();
        W::HandheldObjectPickerMove(1, 0);
        const auto focused = W::HandheldObjectPickerFocus();
        SetEveryRideEntryNotInvented();
        W::HandheldMenuConfirm();
        Require(W::HandheldObjectPickerIsOpen() && H::ShopSelection() == index, "Stale research selection accepted");
        W::HandheldMenuBack();
        for (auto& e : entries)
            RideEntrySetInvented(e.identity.index);
        W::HandheldShopPickerOpen();
        W::HandheldObjectPickerMove(1, 0);
        frame.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(input, frame);
        Require(
            !W::HandheldObjectPickerIsOpen() && H::ShopSelection() == focused && tracks(tile, false) == 0
                && state.park.cash == baselineCash,
            "Shop selection A spilled into placement");
        H::RotateShop();
        Require(H::ShopDirection() == 1, "Semantic shop rotation failed");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        Require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Shop controller init failed");
        {
            H::ControllerInput controller;
            controller.update(true, 1000);
            const auto device = SDL_JoystickAttachVirtual(
                SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
            Require(device >= 0, "Shop controller attach failed");
            auto* joystick = SDL_JoystickOpen(device);
            Require(joystick, "Shop controller open failed");
            auto pump = [&]() {
                SDL_Event e;
                while (SDL_PollEvent(&e))
                    controller.handleEvent(e);
            };
            pump();
            controller.update(true, 1000);
            pump();
            controller.update(true, 1010);
            auto press = [&](bool down) {
                Require(SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, down) == 0, "Shop A failed");
                SDL_JoystickUpdate();
                pump();
            };
            W::HandheldShopPickerOpen();
            press(true);
            const auto selecting = controller.update(true, 1020);
            Require(
                selecting.actions.size() == 1 && H::HandleMenuInput(controller, selecting) && !W::HandheldObjectPickerIsOpen()
                    && tracks(tile, false) == 0 && state.park.cash == baselineCash,
                "Shop selecting A planted or spent money");
            Require(controller.update(true, 1030).actions.empty(), "Shop held selecting A leaked");
            press(false);
            Require(controller.update(true, 1040).actions.empty(), "Shop A release planted");
            press(true);
            const auto fresh = controller.update(true, 1050);
            Require(
                fresh.actions.size() == 1 && fresh.actions[0] == H::Action::confirm && !H::HandleMenuInput(controller, fresh),
                "Fresh A not handed to shop placement");
            press(false);
            SDL_JoystickClose(joystick);
            SDL_JoystickDetachVirtual(device);
        }
        H::SetShopSelection(index);
        S::UpdatePreview(tile, index, 0);
        H::UpdateExplorer(input); // Dummy native UI is unfocused: production focus cleanup.
        Require(tracks(tile, true) == 0 && rides() == baselineRides, "Focus loss retained shop ghost/ride");
        H::EndTerrain();
    }
    // Save/autosave with an active native track ghost must contain neither ghost nor its empty ride record.
    S::UpdatePreview(tile, index, 3);
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/shop-preview.park";
    flags.set(SaveFlag::automatic);
    if (!realUi)
        Ui::ProvisionalElementsRemove();
    Require(ScenarioSave(state, save, flags) != 0, "Shop ghost save failed");
    if (!realUi)
        Ui::ProvisionalElementsRestore();
    Require(tracks(tile, true) == 1 && state.park.cash == baselineCash, "Shop ghost did not return after save");
    S::ClearPreview();
    Require(context.LoadParkFromFile(save), "Shop preview reload failed");
    Require(
        tracks(tile, true) == 0 && rides() == baselineRides && state.park.cash == baselineCash,
        "Save serialized ghost/temporary shop ride");
    query = S::Action(tile, index, 3, false);
    Require(query.error == GameActions::Status::ok, "Shop real query failed");
    Require(
        S::Action(tile, index, 3, true).error == GameActions::Status::ok && tracks(tile, false) == 0,
        "Shop action failed/bypassed queue");
    GameActions::ProcessQueue(state);
    Require(
        tracks(tile, false) == 1 && tracks(tile, true) == 0 && rides() == baselineRides + 1
            && state.park.cash == baselineCash - query.cost,
        "Shop real placement/count/exact cash failed");
    const auto* placed = *TileElementsView<TrackElement>(tile).begin();
    const auto rideId = placed->getRideIndex();
    Require(
        placed->getDirection() == 3 && GetRide(rideId)->subtype == index && GetRide(rideId)->status == RideStatus::open,
        "Placed shop identity/orientation/operating state wrong");
    Require(S::Action(tile, index, 3, true).error != GameActions::Status::ok, "Occupied shop target accepted");
    S::UpdatePreview(tile, index, 3);
    S::ClearPreview();
    Require(tracks(tile, false) == 1 && tracks(tile, true) == 0 && GetRide(rideId), "Invalid ghost cleanup removed real shop");
    Require(
        std::memcmp(&neighbourBefore, MapGetSurfaceElementAt(neighbour), sizeof(neighbourBefore)) == 0,
        "Shop changed neighbouring terrain");
    const auto placedCash = state.park.cash;
    const auto realSave = profile + "/shop-placed.park";
    flags.unset(SaveFlag::automatic);
    Require(ScenarioSave(state, realSave, flags) != 0 && context.LoadParkFromFile(realSave), "Placed shop save/reload failed");
    Require(
        tracks(tile, false) == 1 && GetRide(rideId)->status == RideStatus::open && state.park.cash == placedCash,
        "Placed shop did not survive reload");
    if (realUi)
    {
        gameStateUpdateLogic();
        const auto controlCash = state.park.cash;
        const auto controlTicks = state.currentTicks;
        Require(context.LoadParkFromFile(realSave), "Shop tick fixture reload failed");
        Require(H::SetShopSelection(index), "Shop tick selection failed");
        S::UpdatePreview(next, index, 0);
        Require(tracks(next, true) == 1 && rides() == baselineRides + 2, "Shop tick ghost fixture missing");
        gameStateUpdateLogic();
        Require(
            tracks(next, true) == 1 && tracks(next, false) == 0 && rides() == baselineRides + 2
                && state.park.cash == controlCash && state.currentTicks == controlTicks,
            "Shop preview changed simulation cash/ride count or failed restoration");
        state.cheats.buildInPauseMode = true;
        gGamePaused = 1;
        const auto pausedCash = state.park.cash;
        gameStateTick();
        Require(
            tracks(next, true) == 1 && rides() == baselineRides + 2 && state.park.cash == pausedCash,
            "Paused native tick stranded shop record/changed cash");
        gGamePaused = 0;
        state.cheats.buildInPauseMode = false;
        H::EndTerrain();
        Require(tracks(next, true) == 0 && rides() == baselineRides + 1, "Shop tool exit stranded temporary ride");
        H::SetShopSelection(index);
        S::UpdatePreview(next, index, 0);
        Require(context.LoadParkFromFile(realSave), "Active shop tool park reload failed");
        Require(
            !H::IsShopToolActive() && tracks(next, true) == 0 && rides() == baselineRides + 1,
            "Park change retained shop tool/ghost/ride");
        H::SetShopSelection(index);
        S::UpdatePreview(next, index, 0);
        context.GetUiContext().CloseWindow();
        Require(
            !H::IsShopToolActive() && tracks(next, true) == 0 && rides() == baselineRides + 1,
            "Shutdown retained shop ghost/ride");
    }
    H::EndTerrain();
    const auto facility = std::find_if(entries.begin(), entries.end(), [](const auto& e) {
        return GetRideTypeDescriptor(ShopPlacement::Type(e.identity.index)).StartTrackPiece == TrackElemType::flatTrack1x1B;
    });
    Require(facility != entries.end(), "Information kiosk facility fixture missing");
    const auto facilityIndex = facility->identity.index;
    for (Direction direction = 0; direction < 4; ++direction)
    {
        S::UpdatePreview(next, facilityIndex, direction);
        Require(
            tracks(next, true) == 1 && (*TileElementsView<TrackElement>(next).begin())->getDirection() == direction,
            "Native facility preview rotation failed");
    }
    S::ClearPreview();
    const auto facilityCash = state.park.cash;
    const auto facilityRides = rides();
    query = S::Action(next, facilityIndex, 2, false);
    Require(
        query.error == GameActions::Status::ok && S::Action(next, facilityIndex, 2, true).error == GameActions::Status::ok,
        "Facility placement failed");
    GameActions::ProcessQueue(state);
    const auto facilityId = (*TileElementsView<TrackElement>(next).begin())->getRideIndex();
    Require(
        tracks(next, false) == 1 && tracks(next, true) == 0 && rides() == facilityRides + 1
            && state.park.cash == facilityCash - query.cost && GetRide(facilityId)->status == RideStatus::open,
        "Facility placement/count/open/exact cash failed");
    const auto facilitySave = profile + "/facility-placed.park";
    const auto savedFacilityCash = state.park.cash;
    Require(
        ScenarioSave(state, facilitySave, flags) != 0 && context.LoadParkFromFile(facilitySave), "Facility save/reload failed");
    Require(
        tracks(next, false) == 1 && GetRide(facilityId)->subtype == facilityIndex
            && GetRide(facilityId)->status == RideStatus::open && state.park.cash == savedFacilityCash,
        "Facility identity/status/cash not preserved");
    std::cout << "Native shops / eligibility-grid-selection-cancel-input / ghost-rotation-provisional-save / exact cash-real "
                 "placement-open-reload passed\n";
}

static void TestTreePicker(bool realUi)
{
    namespace H = Ui::Handheld;
    namespace W = Ui::Windows;
    auto& state = getGameState();
    auto& objects = GetContext()->GetObjectManager();
    for (const auto* id : { "tcl", "tpm", "tct", "tsp", "tcf", "tww", "tco", "tns", "tmp" })
    {
        auto* object = objects.LoadObject(std::string("rct2.scenery_small.") + id);
        Require(object, "Picker fixture object missing");
        ScenerySetInvented({ SCENERY_TYPE_SMALL, objects.GetLoadedObjectEntryIndex(object) });
    }
    const auto entries = H::TreePickerEntries();
    Require(entries.size() > H::ObjectPicker::pageSize, "Picker fixture needs multiple pages");
    for (const auto& e : entries)
        Require(
            e.identity.type == ObjectType::smallScenery && H::Trees::Eligible(e.identity.index),
            "Picker included unavailable tree");
    H::ObjectPicker model;
    model.setEntries(entries, entries.front().identity);
    model.move(-1, 0);
    Require(model.focus() == 0, "Picker wrapped across left edge");
    model.move(1, 0);
    Require(model.focus() == 1, "Picker horizontal navigation failed");
    model.move(0, 1);
    model.move(0, 1);
    Require(model.page() == 1 && model.focus() == 7, "Picker page navigation failed");
    const auto focused = model.selected()->identity;
    model.setEntries(entries);
    Require(model.selected()->identity == focused, "Picker refresh lost focus identity");
    for (int i = 0; i < 10; ++i)
        model.move(0, 1);
    Require(model.focus() < entries.size(), "Partial last page navigation exceeded list");
    model.setEntries({});
    model.move(1, 1);
    Require(!model.selected() && model.pages() == 1, "Empty picker unsafe");
    if (!realUi)
        return;
    const CoordsXY tile{ 448, 160 };
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    state.park.cash = 100000;
    const auto cash = state.park.cash;
    auto count = [&](bool ghosts) {
        int n = 0;
        for (auto* e : TileElementsView<SmallSceneryElement>(tile))
            n += e->isGhost() == ghosts;
        return n;
    };
    H::BeginTrees();
    const auto original = H::TreeSelection();
    H::Trees::UpdatePreview(tile, original);
    Require(count(true) == 1, "Picker preview fixture missing");
    W::HandheldTreePickerOpen();
    Require(
        W::HandheldTreePickerIsOpen() && W::HandheldTreePickerFocus() == original && count(true) == 0 && gGamePaused,
        "Picker entry did not focus selection/clear ghost/pause");
    W::HandheldTreePickerMove(1, 0);
    const auto choice = W::HandheldTreePickerFocus();
    Require(choice != original, "Picker did not navigate to next tree");
    W::HandheldMenuBack();
    Require(
        !W::HandheldTreePickerIsOpen() && H::TreeSelection() == original && !gGamePaused,
        "Picker cancellation changed selection/pause");
    H::Trees::UpdatePreview(tile, original);
    Require(count(true) == 1 && state.park.cash == cash, "Cancelled picker failed preview/cash restoration");
    W::HandheldTreePickerOpen();
    W::HandheldTreePickerMove(1, 0);
    SetSceneryItemRestricted({ SCENERY_TYPE_SMALL, choice }, true);
    W::HandheldMenuConfirm();
    Require(
        W::HandheldTreePickerIsOpen() && H::TreeSelection() == original && count(false) == 0,
        "Picker accepted newly restricted selection");
    W::HandheldMenuBack();
    SetSceneryItemRestricted({ SCENERY_TYPE_SMALL, choice }, false);
    W::HandheldTreePickerOpen();
    W::HandheldTreePickerMove(1, 0);
    H::ControllerInput menuInput;
    H::Frame pauseFrame;
    pauseFrame.actions = { H::Action::openMenu };
    Require(
        H::HandleMenuInput(menuInput, pauseFrame) && !W::HandheldTreePickerIsOpen()
            && Ui::GetWindowManager()->FindByClass(WindowClass::handheldMenu) && gGamePaused && H::TreeSelection() == original,
        "Picker Start failed to open pause/preserve selection");
    W::HandheldMenuConfirm(); // Resume the existing pause screen.
    Require(!gGamePaused, "Picker pause resume did not unpause");
    for (const auto& e : entries)
        SetSceneryItemRestricted({ SCENERY_TYPE_SMALL, e.identity.index }, true);
    W::HandheldTreePickerOpen();
    Require(W::HandheldTreePickerFocus() == kObjectEntryIndexNull, "Empty tree picker retained a focus");
    W::HandheldMenuConfirm();
    Require(
        W::HandheldTreePickerIsOpen() && H::TreeSelection() == original && count(false) == 0, "Empty picker activated a tree");
    W::HandheldMenuBack();
    for (const auto& e : entries)
        SetSceneryItemRestricted({ SCENERY_TYPE_SMALL, e.identity.index }, false);

    // A native SDL virtual controller exercises the actual menu dispatcher/suspend handoff.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    Require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Picker controller init failed");
    {
        H::ControllerInput input;
        input.update(true, 100);
        const auto device = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Require(device >= 0, "Picker virtual controller attach failed");
        auto* joystick = SDL_JoystickOpen(device);
        Require(joystick, "Picker joystick open failed");
        auto pump = [&]() {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                input.handleEvent(e);
        };
        pump();
        input.update(true, 100);
        pump();
        input.update(true, 110);
        auto press = [&](bool down) {
            Require(SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, down) == 0, "Picker button failed");
            SDL_JoystickUpdate();
            pump();
        };
        W::HandheldTreePickerOpen();
        W::HandheldTreePickerMove(1, 0);
        const auto selected = W::HandheldTreePickerFocus();
        press(true);
        auto frame = input.update(true, 120);
        Require(frame.actions.size() == 1 && frame.actions[0] == H::Action::confirm, "Picker A mapping wrong");
        Require(
            H::HandleMenuInput(input, frame) && !W::HandheldTreePickerIsOpen() && H::TreeSelection() == selected
                && count(false) == 0 && count(true) == 0 && state.park.cash == cash && !gGamePaused,
            "Picker A also placed/charged or selected wrong tree");
        H::Trees::UpdatePreview(tile, selected);
        Require(count(true) == 1, "Selected tree preview failed");
        Require(input.update(true, 130).actions.empty(), "Held selecting A leaked into placement");
        press(false);
        Require(input.update(true, 140).actions.empty(), "A release emitted placement");
        press(true);
        frame = input.update(true, 150);
        Require(
            frame.actions.size() == 1 && frame.actions[0] == H::Action::confirm && !H::HandleMenuInput(input, frame),
            "Fresh A not handed to placement");
        const auto query = H::Trees::Action(tile, selected, false);
        Require(
            H::Trees::Action(tile, selected, true).error == GameActions::Status::ok, "Picker selection real placement failed");
        GameActions::ProcessQueue(state);
        Require(
            count(false) == 1 && count(true) == 0 && state.park.cash == cash - query.cost,
            "Picker selection real placement/count/cash failed");
        Require(input.update(true, 160).actions.empty(), "Held placement A repeated");
        press(false);
        SDL_JoystickClose(joystick);
        SDL_JoystickDetachVirtual(device);
    }
    H::EndTerrain();
    std::cout
        << "Native tree picker / grid-pages-empty-availability / cancel-pause / SDL A selection-release-placement handoff / "
           "ghost cleanup-exact cash checks passed\n";
}

static void TestTreeGhost(IContext& context, const std::string& profile, bool realUi)
{
    namespace Trees = Ui::Handheld::Trees;
    auto& state = getGameState();
    auto& objects = context.GetObjectManager();
    const auto index = objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.scenery_small.tcl"));
    const auto second = objects.GetLoadedObjectEntryIndex(objects.LoadObject("rct2.scenery_small.tpm"));
    ScenerySetInvented({ SCENERY_TYPE_SMALL, index });
    ScenerySetInvented({ SCENERY_TYPE_SMALL, second });
    const CoordsXY tile{ 448, 128 }, next{ 416, 128 };
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    MapGetSurfaceElementAt(next)->setOwnership({ OwnershipFlag::landOwned });
    auto count = [](CoordsXY xy, bool ghosts) {
        int n = 0;
        for (auto* e : TileElementsView<SmallSceneryElement>(xy))
            n += e->isGhost() == ghosts;
        return n;
    };
    auto firstGhost = [](CoordsXY xy) -> SmallSceneryElement* {
        for (auto* e : TileElementsView<SmallSceneryElement>(xy))
            if (e->isGhost())
                return e;
        return nullptr;
    };
    auto clear = [&]() { return count(tile, true) == 0 && count(next, true) == 0; };
    state.park.cash = 100000;
    const auto cash = state.park.cash;
    const auto groundBefore = *MapGetSurfaceElementAt(tile);
    const auto nextBefore = *MapGetSurfaceElementAt(next);
    auto query = Trees::Action(tile, index, false);
    Trees::UpdatePreview(tile, index);
    Require(count(tile, true) == 1 && count(tile, false) == 0 && state.park.cash == cash, "Tree ghost/cash incorrect");
    Require(
        firstGhost(tile)->getBaseZ() == query.position.z && firstGhost(tile)->getEntryIndex() == index,
        "Tree preview identity/height incorrect");
    // Detect replacement on unchanged updates by setting harmless ghost metadata.
    firstGhost(tile)->setAge(7);
    Trees::UpdatePreview(tile, index);
    Require(firstGhost(tile)->getAge() == 7 && count(tile, true) == 1, "Unchanged tree preview replaced/duplicated");
    const auto withGhost = Trees::Action(tile, index, false);
    Require(
        withGhost.error == GameActions::Status::ok && withGhost.cost == query.cost && firstGhost(tile)->getAge() == 7
            && state.park.cash == cash,
        "Ghost changed normal query/feedback");
    Trees::UpdatePreview(next, index);
    Require(count(tile, true) == 0 && count(next, true) == 1, "Old target ghost retained");
    Trees::UpdatePreview(next, second);
    Require(count(next, true) == 1 && firstGhost(next)->getEntryIndex() == second, "Tree selection ghost stale");
    Trees::RemovePreviewTemporarily();
    Trees::RemovePreviewTemporarily();
    Require(clear() && state.park.cash == cash, "Suspension left ghost or charged cash");
    Trees::RestorePreview();
    Require(clear(), "Nested suspension restored too early");
    Trees::RestorePreview();
    Require(count(next, true) == 1, "Tree provisional restore failed");
    const auto base = MapGetSurfaceElementAt(next)->getBaseZ();
    const auto clearance = MapGetSurfaceElementAt(next)->getClearanceZ();
    MapGetSurfaceElementAt(next)->setBaseZ(base + 8);
    MapGetSurfaceElementAt(next)->setClearanceZ(clearance + 8);
    Trees::UpdatePreview(next, second);
    Require(firstGhost(next)->getBaseZ() == base + 8, "Ghost did not follow changed ground height");
    MapGetSurfaceElementAt(next)->setBaseZ(base);
    MapGetSurfaceElementAt(next)->setClearanceZ(clearance);
    Trees::UpdatePreview(next, second);
    Trees::UpdatePreview({}, index);
    Require(clear(), "Invalid target left tree ghost");
    Trees::UpdatePreview(tile, index);
    ScenerySetNotInvented({ SCENERY_TYPE_SMALL, index });
    Trees::UpdatePreview(tile, index);
    Require(clear(), "Research change left ghost");
    ScenerySetInvented({ SCENERY_TYPE_SMALL, index });
    Trees::UpdatePreview(tile, index);
    SetSceneryItemRestricted({ SCENERY_TYPE_SMALL, index }, true);
    Trees::UpdatePreview(tile, index);
    Require(clear(), "Restriction change left ghost");
    SetSceneryItemRestricted({ SCENERY_TYPE_SMALL, index }, false);
    Trees::UpdatePreview(tile, index);
    MapGetSurfaceElementAt(tile)->setOwnership({});
    state.park.flags.set(ParkFlag::forbidTreeRemoval);
    Trees::UpdatePreview(tile, index);
    Require(clear(), "Ownership/tree-removal rule stranded ghost");
    state.park.flags.unset(ParkFlag::forbidTreeRemoval);
    MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
    Trees::UpdatePreview(tile, index);
    state.park.cash = 0;
    Trees::UpdatePreview(tile, index);
    Require(clear() && state.park.cash == 0, "Insufficient funds left ghost");
    state.park.cash = cash;
    Trees::UpdatePreview(tile, index);
    gGamePaused = 1;
    Trees::UpdatePreview(tile, index);
    Require(clear(), "Pause left invalid ghost");
    gGamePaused = 0;
    Trees::UpdatePreview(tile, index);
    Trees::RemovePreviewTemporarily();
    Trees::ClearPreview();
    Trees::RestorePreview();
    Require(clear(), "Cleared suspension resurrected ghost");
    Require(
        std::memcmp(&groundBefore, MapGetSurfaceElementAt(tile), sizeof(SurfaceElement)) == 0
            && std::memcmp(&nextBefore, MapGetSurfaceElementAt(next), sizeof(SurfaceElement)) == 0 && count(tile, false) == 0
            && count(next, false) == 0 && state.park.cash == cash,
        "Preview lifecycle permanently changed park");
    if (realUi)
    {
        Ui::Handheld::BeginTrees();
        Trees::UpdatePreview(tile, index);
        Ui::Windows::HandheldMenuOpen();
        Require(clear(), "Pause menu retained tree ghost");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Trees::UpdatePreview(tile, index);
        Ui::Windows::HandheldBuildOpen();
        Require(clear(), "Build menu retained tree ghost");
        Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
        Trees::UpdatePreview(tile, index);
        Ui::Handheld::BeginWater();
        Require(clear(), "Other tool retained tree ghost");
        Ui::Handheld::BeginTrees();
        Trees::UpdatePreview(tile, index);
        Ui::GetWindowManager()->BroadcastIntent(Intent(INTENT_ACTION_MAP));
        Require(clear(), "Park-change intent retained tree ghost");
        Ui::Handheld::BeginTrees();
        Trees::UpdatePreview(tile, index);
        Require(!ContextHasFocus(), "SDL dummy focus fixture unexpectedly focused");
        Ui::Handheld::ControllerInput input;
        Ui::Handheld::UpdateExplorer(input);
        Require(clear(), "Unfocused explorer retained tree ghost");
    }
    Trees::UpdatePreview(tile, index);
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects, SaveFlag::automatic);
    const auto save = profile + "/tree-ghost.park";
    // The headless dummy context does not dispatch provisional intents.
    if (!realUi)
        Trees::RemovePreviewTemporarily();
    Require(ScenarioSave(state, save, flags) != 0, "Tree ghost autosave failed");
    if (!realUi)
        Trees::RestorePreview();
    Require(count(tile, true) == 1 && state.park.cash == cash, "Autosave failed to restore tree ghost/cash");
    if (!realUi)
        Trees::ClearPreview();
    Require(context.LoadParkFromFile(save), "Tree ghost reload failed");
    Require(clear() && count(tile, false) == 0 && state.park.cash == cash, "Ghost or charge leaked into save");
    Trees::UpdatePreview(tile, index);
    Require(Trees::Action(tile, index, true).error == GameActions::Status::ok, "Real tree confirmation failed");
    Require(clear() && count(tile, false) == 0 && state.park.cash == cash, "Confirm retained ghost or bypassed queue");
    GameActions::ProcessQueue(state);
    Require(count(tile, false) == 1 && clear() && state.park.cash == cash - query.cost, "Confirm did not place/charge once");
    Trees::UpdatePreview(tile, index);
    Require(clear(), "Occupied tile got misleading ghost");
    Trees::ClearPreview();
    Require(count(tile, false) == 1, "Preview cleanup deleted real tree");
    Trees::UpdatePreview(next, second);
    if (!realUi)
        Trees::ClearPreview();
    flags.unset(SaveFlag::automatic);
    Require(ScenarioSave(state, save, flags) != 0 && context.LoadParkFromFile(save), "Tree manual save/reload failed");
    Require(
        clear() && count(tile, false) == 1 && count(next, false) == 0 && state.park.cash == cash - query.cost,
        "Real tree/ghost/cash save safety failed");
    Trees::UpdatePreview(next, second);
    Ui::Handheld::EndTerrain();
    Trees::RestorePreview();
    Require(clear(), "Tool exit resurrected ghost");
    if (realUi)
    {
        Trees::UpdatePreview(next, second);
        context.GetUiContext().CloseWindow();
        Trees::RestorePreview();
        Require(clear(), "UI shutdown retained/resurrected tree ghost");
    }
    std::cout
        << "Native tree ghost / unchanged-target-selection / nested suspension / invalid-research-ownership-funds-pause / "
           "menu-tool exit / exact real placement-cash / autosave-manual-save-reload checks passed\n";
}

static void TestShopManagement(IContext& context, const std::string& profile, bool realUi)
{
    namespace H = Ui::Handheld;
    namespace M = H::ShopManagement;
    namespace W = Ui::Windows;
    auto& state = getGameState();
    H::EndTerrain();
    gGamePaused = 0;
    auto& objects = context.GetObjectManager();
    auto objectIndex = [&](const char* name) {
        auto* object = objects.LoadObject(std::string("rct2.ride.") + name);
        Require(object, "Management fixture object missing");
        const auto index = objects.GetLoadedObjectEntryIndex(object);
        ResearchInsertRideEntry(index, true);
        RideEntrySetInvented(index);
        RideTypeSetInvented(ShopPlacement::Type(index));
        return index;
    };
    const auto burger = objectIndex("burgb"), kiosk = objectIndex("infok"), atm = objectIndex("atm1"),
               toilet = objectIndex("tlt1");
    auto place = [&](CoordsXY tile, ObjectEntryIndex object) {
        MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::landOwned });
        Require(
            H::Shops::Action(tile, object, 0, true).error == GameActions::Status::ok, "Management fixture placement failed");
        GameActions::ProcessQueue(state);
        const auto id = (*TileElementsView<TrackElement>(tile).begin())->getRideIndex();
        const auto target = M::Capture(id);
        Require(target.has_value() && M::Find(*target), "Real shop inspection target missing");
        return *target;
    };
    const auto first = place({ 64, 352 }, burger), second = place({ 96, 352 }, burger);
    const auto infoKiosk = place({ 128, 352 }, kiosk), otherKiosk = place({ 160, 352 }, kiosk);
    const auto cashMachine = place({ 192, 352 }, atm);
    const auto restroom = place({ 224, 352 }, toilet), otherRestroom = place({ 256, 352 }, toilet);
    const auto cash = state.park.cash;
    InteractionInfo hit;
    hit.interactionType = ViewportInteractionItem::ride;
    hit.Loc = first.location;
    hit.Element = reinterpret_cast<TileElement*>(*TileElementsView<TrackElement>(first.location).begin());
    Require(M::FromInfo(hit).has_value(), "Native shop hit rejected");
    hit.Element->setGhost(true);
    Require(!M::FromInfo(hit) && !M::Find(first), "Ghost accepted for inspection");
    hit.Element->setGhost(false);
    hit.interactionType = ViewportInteractionItem::terrain;
    Require(!M::FromInfo(hit), "Non-ride target inspected");
    Require(!M::Capture(RideId::GetNull()), "Invalid ride inspected");
    auto stale = first;
    stale.object = kiosk;
    Require(
        !M::Find(stale) && M::SetStatus(stale, false).error != GameActions::Status::ok
            && M::SetPrice(stale, true, 100).error != GameActions::Status::ok
            && M::Demolish(stale, true).error != GameActions::Status::ok,
        "Stale identity accepted");
    Require(M::Money(kMoney64Undefined) == "Unavailable", "Unavailable statistics disguised as zero");
    Require(M::Prices(cashMachine).empty(), "Cash machine exposed phantom admission/product");
    auto prices = M::Prices(first);
    Require(prices.size() == 1 && prices[0].primary, "Burger price slots incorrect");
    Require(M::Prices(infoKiosk).size() == 2, "Kiosk secondary price missing");
    Require(
        M::Prices(restroom).size() == 1 && M::Prices(restroom)[0].item == ShopItem::admission,
        "Toilet native admission price missing");
    state.park.samePriceThroughoutPark |= uint64_t{ 1 } << static_cast<int>(ShopItem::admission);
    Require(M::SetPrice(restroom, true, 10).error == GameActions::Status::ok, "Toilet admission adjustment failed");
    GameActions::ProcessQueue(state);
    Require(
        GetRide(restroom.id)->price[0] == 10 && GetRide(otherRestroom.id)->price[0] == 10,
        "Native toilet common admission rule bypassed");
    Require(M::SetStatus(first, false).error == GameActions::Status::ok, "Native close failed");
    GameActions::ProcessQueue(state);
    Require(GetRide(first.id)->status == RideStatus::closed && state.park.cash == cash, "Closing charged money");
    Require(M::SetStatus(first, true).error == GameActions::Status::ok, "Native open failed");
    GameActions::ProcessQueue(state);
    Require(GetRide(first.id)->status == RideStatus::open, "Opening did not persist");
    state.park.samePriceThroughoutPark &= ~(uint64_t{ 1 } << static_cast<int>(prices[0].item));
    const auto secondPrice = GetRide(second.id)->price[0];
    Require(M::SetPrice(first, true, 170).error == GameActions::Status::ok, "Local price change failed");
    GameActions::ProcessQueue(state);
    Require(
        GetRide(first.id)->price[0] == 170 && GetRide(second.id)->price[0] == secondPrice, "Local price affected neighbour");
    state.park.samePriceThroughoutPark |= uint64_t{ 1 } << static_cast<int>(prices[0].item);
    Require(M::Prices(first)[0].shared, "Common price status not exposed");
    Require(M::SetPrice(first, true, 190).error == GameActions::Status::ok, "Shared price change failed");
    GameActions::ProcessQueue(state);
    Require(GetRide(first.id)->price[0] == 190 && GetRide(second.id)->price[0] == 190, "Native common price not preserved");
    const auto secondary = M::Prices(infoKiosk)[1];
    state.park.samePriceThroughoutPark |= uint64_t{ 1 } << static_cast<int>(secondary.item);
    Require(M::SetPrice(infoKiosk, false, 25).error == GameActions::Status::ok, "Secondary common price failed");
    GameActions::ProcessQueue(state);
    Require(
        GetRide(infoKiosk.id)->price[1] == 25 && GetRide(otherKiosk.id)->price[1] == 25,
        "Secondary shared price not propagated");
    Require(
        M::SetPrice(first, true, kRideMaxPrice + 1).error != GameActions::Status::ok
            && M::SetPrice(first, true, -1).error != GameActions::Status::ok
            && M::SetPrice(first, false, 100).error != GameActions::Status::ok,
        "Invalid/phantom price accepted");
    Require(state.park.cash == cash, "Pricing spent or refunded money");
    if (realUi)
    {
        H::ControllerInput input;
        H::Frame frame;
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        Require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Management controller init failed");
        {
            H::ControllerInput controller;
            controller.update(true, 2000);
            const auto device = SDL_JoystickAttachVirtual(
                SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
            Require(device >= 0, "Management controller attach failed");
            auto* joystick = SDL_JoystickOpen(device);
            Require(joystick, "Management joystick missing");
            auto pump = [&]() {
                SDL_Event e;
                while (SDL_PollEvent(&e))
                    controller.handleEvent(e);
            };
            pump();
            controller.update(true, 2010);
            pump();
            controller.update(true, 2020);
            auto press = [&](bool down) {
                Require(SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, down) == 0, "Management A failed");
                SDL_JoystickUpdate();
                pump();
            };
            W::HandheldRideOpen(first.id);
            press(true);
            auto actions = controller.update(true, 2030);
            Require(actions.actions.size() == 1 && H::HandleMenuInput(controller, actions), "Management A not consumed");
            GameActions::ProcessQueue(state);
            Require(GetRide(first.id)->status == RideStatus::closed, "Management A did not close");
            Require(controller.update(true, 2040).actions.empty(), "Held inspection A leaked");
            press(false);
            Require(controller.update(true, 2050).actions.empty(), "Inspection release triggered action");
            press(true);
            actions = controller.update(true, 2060);
            Require(actions.actions.size() == 1 && H::HandleMenuInput(controller, actions), "Fresh A did not reopen");
            GameActions::ProcessQueue(state);
            Require(GetRide(first.id)->status == RideStatus::open, "Fresh status press failed");
            press(false);
            controller.update(true, 2070);
            W::HandheldMenuMove(1);
            W::HandheldMenuMove(1);
            press(true);
            actions = controller.update(true, 2080);
            Require(H::HandleMenuInput(controller, actions), "Confirmation entry A failed");
            Require(controller.update(true, 2090).actions.empty(), "Held confirmation-entry A leaked");
            GameActions::ProcessQueue(state);
            Require(M::Find(first) && state.park.cash == cash, "Confirmation entry demolished shop");
            press(false);
            Require(controller.update(true, 2100).actions.empty(), "Confirmation release demolished shop");
            W::HandheldMenuBack();
            W::HandheldMenuBack();
            SDL_JoystickClose(joystick);
            SDL_JoystickDetachVirtual(device);
        }
        W::HandheldRideOpen(first.id);
        Require(W::HandheldShopManagementIsOpen() && !gGamePaused, "Inspection incorrectly paused native demolition");
        frame.actions = { H::Action::confirm, H::Action::confirm };
        Require(H::HandleMenuInput(input, frame), "Inspection did not consume A");
        GameActions::ProcessQueue(state);
        Require(GetRide(first.id)->status == RideStatus::closed, "Duplicate confirm toggled status twice");
        W::HandheldShopManagementMove(0, 1);
        W::HandheldMenuConfirm();
        W::HandheldShopManagementMove(1, 0);
        W::HandheldMenuBack();
        Require(GetRide(first.id)->price[0] == 190, "Cancelled price draft applied");
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        for (int i = 0; i < 300; ++i)
            W::HandheldShopManagementMove(1, 0);
        W::HandheldMenuConfirm();
        GameActions::ProcessQueue(state);
        Require(
            GetRide(first.id)->price[0] == kRideMaxPrice && GetRide(second.id)->price[0] == kRideMaxPrice,
            "Controller price upper bound/common price failed");
        for (int i = 0; i < 300; ++i)
            W::HandheldShopManagementMove(-1, 0);
        W::HandheldMenuConfirm();
        GameActions::ProcessQueue(state);
        Require(GetRide(first.id)->price[0] == kRideMinPrice, "Controller price lower bound failed");
        W::HandheldMenuBack();
        W::HandheldMenuMove(1);
        W::HandheldMenuMove(1);
        frame.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(input, frame);
        GameActions::ProcessQueue(state);
        Require(M::Find(first) && state.park.cash == cash, "Entering confirmation demolished shop");
        W::HandheldMenuConfirm(); // Cancel is initially focused.
        Require(M::Find(first) && state.park.cash == cash, "Default confirmation did not cancel");
        W::HandheldMenuMove(1);
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuBack();
        Require(M::Find(first), "B cancellation demolished shop");
        frame.actions = { H::Action::openMenu };
        H::HandleMenuInput(input, frame);
        Require(!W::HandheldShopManagementIsOpen() && gGamePaused, "Management Start did not open pause");
        W::HandheldMenuBack();
        Require(!gGamePaused, "Management pause return lost prior pause state");
        gGamePaused = 1;
        W::HandheldRideOpen(first.id);
        W::HandheldMenuBack();
        Require(gGamePaused == 1, "Inspection unpaused an externally paused park");
        gGamePaused = 0;
        state.park.flags.set(ParkFlag::noMoney);
        W::HandheldRideOpen(first.id);
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldShopManagementMove(1, 0);
        W::HandheldMenuConfirm();
        GameActions::ProcessQueue(state);
        Require(GetRide(first.id)->price[0] == kRideMinPrice, "No-money park exposed editable prices");
        W::HandheldMenuBack();
        state.park.flags.unset(ParkFlag::noMoney);
    }
    GetRide(first.id)->flags.set(RideFlag::indestructible);
    Require(M::Demolish(first, false).error != GameActions::Status::ok, "Protected shop demolition allowed");
    GetRide(first.id)->flags.unset(RideFlag::indestructible);
    gGamePaused = 1;
    Require(M::Demolish(first, false).error == GameActions::Status::gamePaused, "Native demolition pause rule bypassed");
    gGamePaused = 0;
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto saved = profile + "/shop-management.park";
    const auto storedPrice = GetRide(first.id)->price[0];
    const auto storedStatus = GetRide(first.id)->status;
    const auto common = state.park.samePriceThroughoutPark;
    Require(ScenarioSave(state, saved, flags) != 0, "Management save failed");
    if (realUi)
        W::HandheldRideOpen(first.id);
    Require(context.LoadParkFromFile(saved), "Management reload failed");
    if (realUi)
        Require(!W::HandheldShopManagementIsOpen(), "Park load retained stale inspection");
    Require(
        M::Find(first) && GetRide(first.id)->price[0] == storedPrice && GetRide(first.id)->status == storedStatus
            && state.park.samePriceThroughoutPark == common && state.park.cash == cash,
        "Management state not persisted");
    gGamePaused = 0;
    const auto refund = M::Demolish(first, false);
    Require(
        refund.error == GameActions::Status::ok && refund.cost < 0 && state.park.cash == cash,
        "Native refund query missing or mutated cash");
    if (realUi)
    {
        W::HandheldRideOpen(first.id);
        W::HandheldMenuMove(1);
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(-1);
        H::ControllerInput input;
        H::Frame frame;
        frame.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(input, frame);
        Require(!W::HandheldShopManagementIsOpen(), "Demolition kept active confirmation");
    }
    else
        Require(M::Demolish(first, true).error == GameActions::Status::ok, "Native demolition failed");
    GameActions::ProcessQueue(state);
    Require(
        !M::Find(first) && !GetRide(first.id) && M::Find(second) && state.park.cash == cash - refund.cost,
        "Demolition exact refund/target/neighbour safety failed");
    Require(
        M::SetStatus(first, true).error != GameActions::Status::ok
            && M::SetPrice(first, true, 100).error != GameActions::Status::ok
            && M::Demolish(first, true).error != GameActions::Status::ok,
        "Removed target still actionable");
    Require(ScenarioSave(state, saved, flags) != 0 && context.LoadParkFromFile(saved), "Demolition save/reload failed");
    Require(!GetRide(first.id) && M::Find(second) && state.park.cash == cash - refund.cost, "Demolition/refund not persisted");
    std::cout << "Native shop management / targeting-status-price-common-price-bounds / cancellation-input / exact refund / "
                 "stale-pause / save-reload passed\n";
}

static void TestFinances(IContext& context, const std::string& profile, bool realUi)
{
    namespace H = Ui::Handheld;
    namespace F = H::Finances;
    namespace W = Ui::Windows;
    auto& state = getGameState();
    auto& park = state.park;
    H::EndTerrain();
    gGamePaused = 0;
    park.flags.unset(ParkFlag::noMoney, ParkFlag::freeEntry, ParkFlag::unlockAllPrices);
    park.cash = 5000.00_GBP;
    park.bankLoan = 1000.00_GBP;
    park.maxBankLoan = 5000.00_GBP;
    park.entranceFee = 10.00_GBP;
    for (auto& value : park.expenditureTable[0])
        value = 0;
    park.expenditureTable[0][EnumValue(ExpenditureType::shopSales)] = 150.00_GBP;
    park.expenditureTable[0][EnumValue(ExpenditureType::wages)] = -42.00_GBP;
    Require(
        F::MonthlyTotals() == std::pair<money64, money64>{ 150.00_GBP, 42.00_GBP }, "Monthly native ledger totals incorrect");
    const auto currency = Config::Get().general.currencyFormat;
    for (const auto choice : { CurrencyType::pounds, CurrencyType::dollars, CurrencyType::euros })
    {
        Config::Get().general.currencyFormat = choice;
        Require(
            H::FormatMoney(10.00_GBP) == FormatStringID(STR_CURRENCY2DP, 10.00_GBP)
                && H::FormatMoney(-10.00_GBP) == FormatStringID(STR_CURRENCY2DP, -10.00_GBP),
            "Shared native currency/sign incorrect");
    }
    Config::Get().general.currencyFormat = currency;
    Require(H::FormatMoney(kMoney64Undefined) == "Unavailable", "Undefined financial value disguised as zero");
    park.expenditureTable[0][0] = kMoney64Undefined;
    Require(F::MonthlyTotals().first == kMoney64Undefined, "Unavailable ledger hidden");
    park.expenditureTable[0][0] = 0;
    const auto ledger = std::vector<money64>(std::begin(park.expenditureTable[0]), std::end(park.expenditureTable[0]));
    auto loan = F::Begin(F::Field::loan);
    const auto originalCash = park.cash;
    F::Adjust(loan, 1);
    Require(
        loan.value == 2000.00_GBP && F::Action(loan, false).error == GameActions::Status::ok && park.cash == originalCash
            && park.bankLoan == loan.baseline,
        "Loan draft/query mutated cash");
    Require(F::Action(loan, true).error == GameActions::Status::ok && park.bankLoan == loan.baseline, "Loan bypassed queue");
    GameActions::ProcessQueue(state);
    Require(park.bankLoan == loan.value && park.cash == originalCash + 1000.00_GBP, "Borrow did not add exact cash once");
    loan = F::Begin(F::Field::loan);
    F::Adjust(loan, -1);
    Require(F::Action(loan, true).error == GameActions::Status::ok, "Repay submission failed");
    GameActions::ProcessQueue(state);
    Require(park.bankLoan == 1000.00_GBP && park.cash == originalCash, "Repay did not deduct exact cash once");
    Require(std::equal(ledger.begin(), ledger.end(), std::begin(park.expenditureTable[0])), "Loan altered operating ledger");
    loan = F::Begin(F::Field::loan);
    for (int i = 0; i < 20; ++i)
        F::Adjust(loan, 1);
    Require(loan.value == park.maxBankLoan, "Loan upper bound failed");
    loan.value = park.maxBankLoan + 1;
    Require(F::Action(loan, true).error != GameActions::Status::ok, "Excess loan allowed");
    loan.value = -1;
    Require(F::Action(loan, true).error != GameActions::Status::ok, "Negative loan allowed");
    loan = F::Begin(F::Field::loan);
    for (int i = 0; i < 20; ++i)
        F::Adjust(loan, -1);
    Require(loan.value == 0, "Loan lower bound failed");
    park.cash = 0;
    Require(F::Action(loan, true).error == GameActions::Status::insufficientFunds, "Unaffordable repayment allowed");
    park.cash = -100.00_GBP;
    loan = F::Begin(F::Field::loan);
    F::Adjust(loan, 1);
    Require(F::Action(loan, true).error == GameActions::Status::ok, "Borrowing while in debt blocked");
    GameActions::ProcessQueue(state);
    Require(park.cash == 900.00_GBP && park.bankLoan == 2000.00_GBP, "Debt borrowing delta wrong");
    park.bankLoan = 6000.00_GBP;
    loan = F::Begin(F::Field::loan);
    F::Adjust(loan, 1);
    Require(loan.value == 6000.00_GBP, "Above-limit current loan increased or silently clamped");
    F::Adjust(loan, -1);
    Require(loan.value == 5000.00_GBP, "Above-limit repayment blocked");
    park.bankLoan = 1000.00_GBP;
    park.cash = originalCash;
    loan = F::Begin(F::Field::loan);
    F::Adjust(loan, 1);
    park.bankLoan += 1;
    Require(F::Action(loan, true).error != GameActions::Status::ok, "Stale loan draft accepted");
    park.bankLoan = 1000.00_GBP;
    auto admission = F::Begin(F::Field::admission);
    F::Adjust(admission, 1);
    Require(
        admission.value == 11.00_GBP && F::Action(admission, false).error == GameActions::Status::ok
            && park.entranceFee == 10.00_GBP,
        "Admission draft/query applied");
    gGamePaused = 1;
    Require(F::Action(admission, true).error == GameActions::Status::ok, "Native paused admission change rejected");
    GameActions::ProcessQueue(state);
    Require(park.entranceFee == 11.00_GBP && park.cash == originalCash, "Admission change charged immediate cash");
    loan = F::Begin(F::Field::loan);
    F::Adjust(loan, 1);
    Require(F::Action(loan, true).error == GameActions::Status::ok, "Native paused loan change rejected");
    GameActions::ProcessQueue(state);
    Require(park.cash == originalCash + 1000.00_GBP, "Paused loan cash incorrect");
    gGamePaused = 0;
    admission = F::Begin(F::Field::admission);
    for (int i = 0; i < 1100; ++i)
        F::Adjust(admission, 1);
    Require(admission.value == kMaxEntranceFee, "Admission upper bound failed");
    admission.value = kMaxEntranceFee + 1;
    Require(F::Action(admission, true).error != GameActions::Status::ok, "Invalid admission high accepted");
    admission.value = -1;
    Require(F::Action(admission, true).error != GameActions::Status::ok, "Invalid admission low accepted");
    admission = F::Begin(F::Field::admission);
    for (int i = 0; i < 1100; ++i)
        F::Adjust(admission, -1);
    Require(admission.value == 0, "Admission lower bound failed");
    auto staleAdmission = F::Begin(F::Field::admission);
    F::Adjust(staleAdmission, 1);
    park.entranceFee += 1;
    Require(F::Action(staleAdmission, true).error != GameActions::Status::ok, "Stale admission draft accepted");
    park.entranceFee -= 1;
    park.flags.set(ParkFlag::freeEntry);
    Require(
        !F::Available(F::Field::admission) && F::Action(admission, true).error != GameActions::Status::ok,
        "Locked admission editable");
    park.flags.set(ParkFlag::unlockAllPrices);
    Require(
        F::Available(F::Field::admission) && F::Action(admission, false).error == GameActions::Status::ok,
        "Native unlock-all-prices rule ignored");
    park.flags.unset(ParkFlag::freeEntry, ParkFlag::unlockAllPrices);
    park.flags.set(ParkFlag::noMoney);
    Require(
        !F::Available(F::Field::loan) && !F::Available(F::Field::admission)
            && F::Action(F::Begin(F::Field::loan), true).error != GameActions::Status::ok
            && F::Action(F::Begin(F::Field::admission), true).error != GameActions::Status::ok,
        "No-money finances changed");
    Require(
        F::Presentation(F::View::overview, 0).stats[0].second == "Not applicable",
        "No-money statistics presented as spendable cash");
    park.flags.unset(ParkFlag::noMoney);
    park.cash = originalCash;
    park.bankLoan = 1000.00_GBP;
    if (realUi)
    {
        H::ControllerInput input;
        H::Frame frame;
        auto open = [&]() {
            W::HandheldMenuOpen();
            W::HandheldMenuMove(1);
            W::HandheldMenuMove(1);
            W::HandheldMenuConfirm();
            W::HandheldMenuMove(1);
            W::HandheldMenuConfirm();
            Require(W::HandheldFinancesIsOpen() && gGamePaused, "Dashboard finance navigation failed");
        };
        open();
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
        W::HandheldMenuBack();
        Require(park.cash == originalCash && park.bankLoan == 1000.00_GBP, "B applied draft");
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
        frame.actions = { H::Action::confirm, H::Action::confirm };
        H::HandleMenuInput(input, frame);
        GameActions::ProcessQueue(state);
        Require(park.bankLoan == 1000.00_GBP, "Review A spilled into confirmation");
        W::HandheldMenuConfirm(); // Default Cancel.
        Require(park.cash == originalCash && park.bankLoan == 1000.00_GBP, "Default Cancel applied loan");
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(-1);
        H::HandleMenuInput(input, frame);
        GameActions::ProcessQueue(state);
        Require(
            park.bankLoan == 2000.00_GBP && park.cash == originalCash + 1000.00_GBP,
            "Confirmation charged twice or wrong delta");
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(-1);
        H::HandleMenuInput(input, frame);
        GameActions::ProcessQueue(state);
        Require(park.entranceFee == 12.00_GBP && park.cash == originalCash + 1000.00_GBP, "Controller admission change failed");
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
        frame.actions = { H::Action::openMenu };
        H::HandleMenuInput(input, frame);
        Require(!W::HandheldFinancesIsOpen(), "Start did not discard finance editor");
        W::HandheldMenuBack();
        Require(!gGamePaused, "Finances pause state leaked");
        Require(park.bankLoan == 2000.00_GBP && park.entranceFee == 12.00_GBP, "Start applied draft");
        open();
        W::HandheldMenuBack();
        W::HandheldMenuConfirm();
        Require(W::HandheldFinancesIsOpen(), "Dashboard did not retain section focus");
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        Require(!gGamePaused, "Finance exit left pause");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        Require(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "Finance controller init failed");
        {
            H::ControllerInput controller;
            controller.update(true, 3000);
            const auto device = SDL_JoystickAttachVirtual(
                SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
            auto* joystick = SDL_JoystickOpen(device);
            Require(joystick, "Finance virtual controller failed");
            auto pump = [&]() {
                SDL_Event e;
                while (SDL_PollEvent(&e))
                    controller.handleEvent(e);
            };
            pump();
            controller.update(true, 3010);
            pump();
            controller.update(true, 3020);
            auto press = [&](bool down) {
                SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, down);
                SDL_JoystickUpdate();
                pump();
            };
            open();
            W::HandheldMenuConfirm();
            W::HandheldFinancesMove(1, 0);
            press(true);
            auto f = controller.update(true, 3030);
            Require(f.actions.size() == 1 && H::HandleMenuInput(controller, f), "Finance review press lost");
            Require(controller.update(true, 3040).actions.empty(), "Held review A leaked");
            press(false);
            Require(controller.update(true, 3050).actions.empty(), "Review release applied finance");
            W::HandheldMenuMove(-1);
            press(true);
            f = controller.update(true, 3060);
            Require(f.actions.size() == 1 && H::HandleMenuInput(controller, f), "Fresh finance confirmation lost");
            GameActions::ProcessQueue(state);
            Require(
                park.bankLoan == 3000.00_GBP && park.cash == originalCash + 2000.00_GBP,
                "Fresh finance confirmation cash wrong");
            Require(controller.update(true, 3070).actions.empty(), "Held confirming A reopened editor");
            press(false);
            controller.update(true, 3080);
            W::HandheldMenuBack();
            W::HandheldMenuBack();
            W::HandheldMenuBack();
            SDL_JoystickClose(joystick);
            SDL_JoystickDetachVirtual(device);
        }
    }
    if (realUi)
    {
        const auto cash = park.cash, loanValue = park.bankLoan, feeValue = park.entranceFee;
        park.flags.set(ParkFlag::noMoney);
        W::HandheldMenuOpen();
        W::HandheldMenuMove(1);
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        GameActions::ProcessQueue(state);
        Require(
            W::HandheldFinancesIsOpen() && park.cash == cash && park.bankLoan == loanValue && park.entranceFee == feeValue,
            "No-money UI changed financial state");
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        park.flags.unset(ParkFlag::noMoney);
        gGamePaused = 1;
        W::HandheldMenuOpen();
        W::HandheldMenuMove(1);
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        W::HandheldMenuBack();
        Require(gGamePaused == 1, "Finance exit unpaused externally paused park");
        gGamePaused = 0;
    }
    SaveFlags flags{};
    flags.set(SaveFlag::exportObjects);
    const auto save = profile + "/finances.park";
    const auto savedCash = park.cash, savedLoan = park.bankLoan, savedFee = park.entranceFee;
    Require(ScenarioSave(state, save, flags) != 0, "Finance save failed");
    if (realUi)
    {
        W::HandheldMenuOpen();
        W::HandheldMenuMove(1);
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuMove(1);
        W::HandheldMenuConfirm();
        W::HandheldMenuConfirm();
        W::HandheldFinancesMove(1, 0);
    }
    Require(context.LoadParkFromFile(save), "Finance reload failed");
    Require(
        park.cash == savedCash && park.bankLoan == savedLoan && park.entranceFee == savedFee
            && std::equal(ledger.begin(), ledger.end(), std::begin(park.expenditureTable[0])),
        "Finance/ledger state not persisted");
    if (realUi)
        Require(!W::HandheldFinancesIsOpen(), "Load retained finance draft/menu");
    std::cout << "Native finances / ledger-currency / bounded loan-exact cash-debt-repay / admission-restrictions / "
                 "cancel-input-pause-no-money / save-reload passed\n";
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
            Ui::Windows::HandheldBuildOpen();
            for (int i = 0; i < 3; ++i)
                Ui::Windows::HandheldMenuMove(1);
            Ui::Windows::HandheldMenuConfirm();
            Require(!Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }), "Water entry retained path tool/ghost");
            Ui::Handheld::EndTerrain();
            Paths::Begin(false);
            info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(previewTile));
            Paths::UpdateTargetFromInfo(info);
            Paths::UpdatePreview();
            Require(Paths::IsActive() && FindPath(TileCoordsXYZ{ previewLocation }), "Surface entry ghost fixture missing");
            Ui::Windows::HandheldBuildOpen();
            for (int i = 0; i < 4; ++i)
                Ui::Windows::HandheldMenuMove(1);
            Ui::Windows::HandheldMenuConfirm();
            Require(
                !Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }), "Surface entry retained path tool/ghost");
            Ui::Handheld::EndTerrain();
            Paths::Begin(false);
            info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(previewTile));
            Paths::UpdateTargetFromInfo(info);
            Paths::UpdatePreview();
            Require(Paths::IsActive() && FindPath(TileCoordsXYZ{ previewLocation }), "Scenery entry ghost fixture missing");
            Ui::Windows::HandheldBuildOpen();
            for (int i = 0; i < 5; ++i)
                Ui::Windows::HandheldMenuMove(1);
            Ui::Windows::HandheldMenuConfirm();
            Require(
                !Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }),
                "Scenery removal entry retained path tool/ghost");
            Ui::Handheld::EndTerrain();
            Paths::Begin(false);
            info.Element = reinterpret_cast<TileElement*>(MapGetSurfaceElementAt(previewTile));
            Paths::UpdateTargetFromInfo(info);
            Paths::UpdatePreview();
            Require(FindPath(TileCoordsXYZ{ previewLocation }), "Tree entry ghost fixture missing");
            Ui::Windows::HandheldBuildOpen();
            for (int i = 0; i < 6; ++i)
                Ui::Windows::HandheldMenuMove(1);
            Ui::Windows::HandheldMenuConfirm();
            Require(!Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }), "Tree entry retained path ghost");
            Require(context->LoadParkFromFile(save), "UI reload failed");
            Require(!Paths::IsActive() && !FindPath(TileCoordsXYZ{ previewLocation }), "Park load retained preview/tool");
        }
        TestWater(*context, argv[2]);
        TestSurface(*context, argv[2]);
        TestSceneryRemoval(*context, argv[2]);
        TestTrees(*context, argv[2]);
        TestTreePicker(argc == 4);
        TestTreeGhost(*context, argv[2], argc == 4);
        TestShops(*context, argv[2], argc == 4);
        TestShopManagement(*context, argv[2], argc == 4);
        TestFinances(*context, argv[2], argc == 4);
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
