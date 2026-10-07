// SPDX-License-Identifier: GPL-3.0-only
#include <array>
#include <cstring>
#include <iostream>
#include <openrct2-ui/ProvisionalElements.h>
#include <openrct2-ui/UiContext.h>
#include <openrct2-ui/handheld/ControllerInput.h>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/SceneryRemoval.h>
#include <openrct2-ui/handheld/Surface.h>
#include <openrct2-ui/handheld/Trees.h>
#include <openrct2-ui/handheld/Water.h>
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
        TestTreeGhost(*context, argv[2], argc == 4);
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
