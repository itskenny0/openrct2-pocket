// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2-ui/handheld/SceneryClear.h>
#include <openrct2/world/tile_element/LargeSceneryElement.h>
template<typename Capture>
void TestSceneryClear(
    GameState_t& state, ObjectEntryIndex large, ObjectEntryIndex small, ObjectEntryIndex surface, ObjectEntryIndex railings,
    H::ControllerInput& controller, const H::Frame& confirm, bool& clearPanel, CoordsXYZ& captureFocus, Capture capture)
{
    namespace Clear = H::SceneryClear;
    const CoordsXY tile{ 864, 864 };
    const auto height = MapGetSurfaceElementAt(tile)->getBaseZ();
    S::Settings placement;
    placement.category = S::Category::large;
    placement.object = large;
    placement.direction = 0;
    Check(S::Place(tile, placement, true).error == GameActions::Status::ok, "Area large fixture failed");
    GameActions::ProcessQueue(state);
    placement.category = S::Category::small;
    placement.object = small;
    Check(S::Place(tile + CoordsXY{ 32, 0 }, placement, true).error == GameActions::Status::ok, "Area small fixture failed");
    GameActions::ProcessQueue(state);
    GameActions::FootpathPlaceAction path({ tile + CoordsXY{ 32, 32 }, height }, FootpathSlope{}, surface, railings);
    Check(GameActions::Execute(&path, state).error == GameActions::Status::ok, "Area unselected path fixture failed");
    GameActions::ProcessQueue(state);
    Clear::Settings area;
    area.width = 2;
    area.height = 2;
    Check(Clear::Query(CoordsXY{ 0, 0 }, area).error != GameActions::Status::ok, "Invalid area bounds accepted");
    auto empty = area;
    empty.items.clearAll();
    Check(!Clear::Begin(tile, empty), "Empty categories accepted");
    auto draft = Clear::Begin(tile, area);
    Check(draft.has_value(), "Native clear review absent");
    const auto cash = state.park.cash;
    auto stale = *draft;
    auto queryMismatch = *draft;
    ++queryMismatch.cost;
    Check(Clear::Action(queryMismatch, true).error != GameActions::Status::ok, "Changed clear quote accepted");
    for (auto* e : TileElementsView<>(tile))
    {
        InteractionInfo info{};
        info.Element = e;
        info.Loc = tile;
        auto target = S::Capture(info);
        if (target && target->category == S::Category::large)
        {
            auto colours = target->colours;
            colours[0] = Drawing::Colour::brightRed;
            Check(
                S::Modify(*target, S::Mode::recolour, colours, true).error == GameActions::Status::ok,
                "Stale area colour action failed");
            GameActions::ProcessQueue(state);
            break;
        }
    }
    Check(Clear::Action(stale, true).error != GameActions::Status::ok, "Changed area accepted");
    Clear::settings = area;
    H::BeginSceneryClear();
    clearPanel = true;
    captureFocus = { tile + CoordsXY{ 16, 16 }, height };
    const auto range = H::AreaTools::Range(tile, 2, 2);
    gMapSelectPositionA = { range->getX1(), range->getY1() };
    gMapSelectPositionB = { range->getX2(), range->getY2() };
    gMapSelectType = MapSelectType::full;
    gMapSelectFlags.set(MapSelectFlag::enable);
    capture("parity-04-area-footprint.png", false);
    W::HandheldSceneryClearReviewOpen(CoordsXY{ 0, 0 });
    capture("parity-13-invalid-area-bounds.png", true);
    Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
    W::HandheldSceneryClearOptionsOpen();
    W::HandheldConstructionMove(0, 7);
    capture("parity-05-clear-dense-options.png", true);
    Ui::GetWindowManager()->CloseByClass(WindowClass::handheldMenu);
    W::HandheldSceneryClearReviewOpen(tile);
    capture("parity-06-clear-native-review.png", true);
    H::HandleMenuInput(controller, confirm);
    Check(state.park.cash == cash, "Cancel charged clear");
    draft = Clear::Begin(tile, area);
    Check(draft.has_value(), "Clear review recovery failed");
    const auto cost = draft->cost;
    Check(Clear::Action(*draft, true).error == GameActions::Status::ok, "Native clear execution failed");
    Check(Clear::Action(*draft, true).error != GameActions::Status::ok, "Repeated clear accepted");
    GameActions::ProcessQueue(state);
    Check(state.park.cash == cash - cost, "Clear query/execute accounting mismatch");
    Check(MapGetFootpathElement({ tile + CoordsXY{ 32, 32 }, height }), "Unselected path removed");
    Check(!Clear::Begin(tile, area), "Empty area still offered clear");
    capture("parity-07-area-after-clear.png", false);
    H::EndTerrain();
    gMapSelectFlags.clearAll();
    const CoordsXY partialTile{ 1056, 864 };
    placement.category = S::Category::large;
    placement.object = large;
    Check(S::Place(partialTile, placement, true).error == GameActions::Status::ok, "Partial area fixture failed");
    GameActions::ProcessQueue(state);
    Clear::Settings one;
    one.items = { GameActions::ClearableItem::largeScenery };
    auto partial = Clear::Begin(partialTile, one);
    Check(partial && partial->cost != 0, "Multi-tile quote omitted native refund");
    const auto partialCash = state.park.cash;
    const auto partialCost = partial->cost;
    Check(Clear::Action(*partial, true).error == GameActions::Status::ok, "Partial area clear failed");
    GameActions::ProcessQueue(state);
    Check(state.park.cash == partialCash - partialCost, "Partial area cost mismatch");
    for (const auto* e : TileElementsView<LargeSceneryElement>(partialTile + CoordsXY{ 0, 32 }))
        Check(e->isGhost(), "Whole large object survived outside area");
    Clear::Settings paths;
    paths.items = { GameActions::ClearableItem::footpaths };
    auto pathDraft = Clear::Begin(tile + CoordsXY{ 32, 32 }, paths);
    Check(pathDraft.has_value(), "Explicit path removal absent");
    Check(Clear::Action(*pathDraft, true).error == GameActions::Status::ok, "Explicit clear path failed");
    GameActions::ProcessQueue(state);
    Check(!MapGetFootpathElement({ tile + CoordsXY{ 32, 32 }, height }), "Explicit clear path survived");
    clearPanel = false;
    std::cout << "Native clear categories/bounds, stale reviews, whole multi-tile refunds, cancellation, paths "
                 "preserved/opt-in passed\n";
}
