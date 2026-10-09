// SPDX-License-Identifier: GPL-3.0-only
// Genuine production drawing components over a public-metadata native test park.
// Original scenery/world sprites are absent; no park backdrop is fabricated.
#pragma once
#include <openrct2-ui/handheld/AreaTools.h>
#include <openrct2-ui/handheld/ConstructionPanel.h>
#include <openrct2-ui/handheld/Hud.h>
#include <openrct2-ui/handheld/Paths.h>
#include <openrct2-ui/handheld/RightsTools.h>
#include <openrct2-ui/handheld/SceneryTools.h>
#include <openrct2/actions/footpath/FootpathPlaceAction.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <stdexcept>
namespace ConstructionPreview
{
    using namespace OpenRCT2;
    namespace H = Ui::Handheld;
    namespace S = H::SceneryTools;
    namespace A = H::AreaTools;
    namespace P = H::Paths;
    inline ObjectEntryIndex Load(const char* id, uint8_t type)
    {
        auto& manager = GetContext()->GetObjectManager();
        auto* object = manager.LoadObject(id);
        if (!object)
            throw std::runtime_error("Capture metadata unavailable");
        const auto index = manager.GetLoadedObjectEntryIndex(object);
        ScenerySetInvented({ type, index });
        return index;
    }
    inline void Setup()
    {
        auto& state = getGameState();
        gameStateInitAll(state, { 32, 32 });
        gLegacyScene = LegacyScene::playing;
        state.park.cash = 1000000; // Initial scenario fixture cash, never a presentation-only value.
        state.park.flags.unset(ParkFlag::noMoney, ParkFlag::noMoneyScenario);
        for (int y = 32; y < 992; y += 32)
            for (int x = 32; x < 992; x += 32)
                MapGetSurfaceElementAt(CoordsXY{ x, y })->setOwnership({ OwnershipFlag::landOwned });
        Load("rct2.scenery_small.corroof", SCENERY_TYPE_SMALL);
        Load("rct2.scenery_large.ssig4", SCENERY_TYPE_LARGE);
        Load("rct2.scenery_wall.wbrg", SCENERY_TYPE_WALL);
        Load("rct2.footpath_banner.bn1", SCENERY_TYPE_BANNER);
        Load("rct2.footpath_item.bench1", SCENERY_TYPE_PATH_ITEM);
        Load("rct2.footpath_item.litter1", SCENERY_TYPE_PATH_ITEM);
        Load("rct2.footpath_item.lamp1", SCENERY_TYPE_PATH_ITEM);
        auto& manager = GetContext()->GetObjectManager();
        auto* surface = manager.LoadObject("rct2.footpath_surface.tarmac");
        auto* railing = manager.LoadObject("rct2.footpath_railings.wood");
        if (!surface || !railing)
            throw std::runtime_error("Capture path metadata unavailable");
        for (CoordsXY tile : { CoordsXY{ 512, 512 }, CoordsXY{ 544, 512 } })
        {
            GameActions::FootpathPlaceAction action(
                { tile, MapGetSurfaceElementAt(tile)->getBaseZ() }, FootpathSlope{}, manager.GetLoadedObjectEntryIndex(surface),
                manager.GetLoadedObjectEntryIndex(railing));
            if (GameActions::Execute(&action, state).error != GameActions::Status::ok)
                throw std::runtime_error("Capture path action failed");
            GameActions::ProcessQueue(state);
        }
    }
    inline void Menu(
        Drawing::RenderTarget& rt, const char* title, const std::vector<std::string>& rows, size_t focus,
        const std::string& status = {})
    {
        Ui::Window window;
        auto size = H::OverlaySize(std::min<size_t>(8, rows.size()));
        window.width = size.width;
        window.height = size.height;
        window.windowPos = { 8, std::max(8, (384 - size.height) / 2) };
        H::MenuPresentation model;
        model.title = title;
        model.rows = rows;
        model.selected = focus;
        model.status = status;
        H::DrawOverlayShell(rt, window, model);
    }
    inline void Cost(Drawing::RenderTarget& rt, const GameActions::Result& q)
    {
        if (q.error == GameActions::Status::ok)
            H::DrawCostBadge(rt, { 288, 224 }, H::FormatMoney(q.cost));
        else
            H::DrawContextMessage(rt, { 288, 224 }, q.getErrorMessage());
    }
    inline void Draw(Drawing::RenderTarget& rt, const std::string& page)
    {
        const CoordsXY tile{ 512, 512 };
        if (page == "park-build-categories")
            Menu(rt, "SCENERY", S::Categories(), 2);
        else if (page == "park-build-terrain-brush")
        {
            A::settings.width = 4;
            A::settings.height = 3;
            A::settings.operation = A::Operation::smoothRaise;
            A::settings.selection = 5;
            Menu(rt, "TERRAIN / WATER", A::Options(), 2, A::SizeName(A::settings));
        }
        else if (page == "park-build-rights-review")
        {
            namespace R = H::RightsTools;
            R::settings = { 2, 2, true };
            MapGetSurfaceElementAt(tile)->setOwnership({ OwnershipFlag::constructionRightsForSale });
            MapGetSurfaceElementAt(CoordsXY{ 544, 512 })->setOwnership({});
            auto draft = R::Begin(tile, R::settings);
            if (!draft)
                throw std::runtime_error("Capture native rights review failed");
            H::DrawManagementPanel(rt, { 512, 384 }, R::Presentation(*draft, 1));
        }
        else if (page == "park-build-water")
        {
            A::settings = { A::Operation::waterRaise, 4, 3 };
            auto q = A::Action(CoordsXY{ 736, 736 }, A::settings, true);
            if (q.error != GameActions::Status::ok)
                throw std::runtime_error("Capture water action failed");
            GameActions::ProcessQueue(getGameState());
            H::DrawHud(rt, { 512, 384 }, H::NativeHud());
            H::DrawConstructionOverlay(
                rt, H::Concept::kOrigin, "Terrain / Water", A::operations[static_cast<size_t>(A::settings.operation)],
                "Area brush", A::SizeName(A::settings), A::SelectionName(A::settings));
            H::DrawConstructionFooter(rt, { 512, 384 }, "Apply", "Brush");
            H::DrawMapFocus(rt, { 256, 192 });
            Cost(rt, A::Action(CoordsXY{ 736, 736 }, A::settings, false));
        }
        else if (page == "park-build-connected")
        {
            P::BeginConnected(false);
            InteractionInfo info{};
            info.Loc = tile;
            info.Element = reinterpret_cast<TileElement*>(
                MapGetFootpathElement({ tile, MapGetSurfaceElementAt(tile)->getBaseZ() }));
            info.interactionType = ViewportInteractionItem::footpath;
            P::UpdateTargetFromInfo(info);
            if (P::Action(true).error != GameActions::Status::ok || !P::SlopeConnected(1)
                || P::Action(true).error != GameActions::Status::ok)
                throw std::runtime_error("Capture connected start failed");
            GameActions::ProcessQueue(getGameState());
            auto q = P::Action(false);
            P::UpdatePreview();
            H::DrawHud(rt, { 512, 384 }, H::NativeHud());
            P::DrawOverlay(rt, H::Concept::kOrigin);
            H::DrawConstructionFooter(rt, { 512, 384 }, "Extend", "Turn");
            H::DrawMapFocus(rt, { 256, 192 });
            Cost(rt, q);
            P::End();
        }
        else
        {
            auto category = page == "park-build-large" ? S::Category::large
                : page == "park-build-wall"            ? S::Category::walls
                : page == "park-build-furniture"       ? S::Category::additions
                                                       : S::Category::small;
            S::SelectCategory(category);
            auto entries = S::Entries(category);
            if (entries.empty() || !S::Select(entries.front().identity.index))
                throw std::runtime_error("Capture picker empty");
            if (page == "park-build-colours" || page == "park-build-dense")
            {
                S::Configure(10); // Recolour; actual production setting.
                S::Configure(7);
                S::Configure(8);
                S::Configure(9);
                if (page == "park-build-dense")
                    for (int i = 0; i < 255; ++i)
                        S::Configure(4);
                Menu(rt, "SCENERY OPTIONS", S::Options(), page == "park-build-dense" ? 11 : 7, S::Name(S::Current()));
            }
            else
            {
                if (category == S::Category::large || category == S::Category::walls)
                    S::Configure(2);
                auto placement = category == S::Category::additions ? tile : CoordsXY{ 320, 320 };
                auto q = S::Place(placement, S::Current(), false);
                S::UpdatePreview(placement);
                H::DrawHud(rt, { 512, 384 }, H::NativeHud());
                S::DrawOverlay(rt, H::Concept::kOrigin);
                H::DrawConstructionFooter(rt, { 512, 384 }, "Apply", "Options");
                H::DrawMapFocus(rt, { 256, 192 });
                Cost(rt, q);
                S::ClearPreview();
            }
        }
        H::DrawModernEllipsised(rt, { 268, 42 }, 236, "{MUTED}COMPONENT FIXTURE");
        H::DrawModernEllipsised(rt, { 268, 56 }, 236, "{MUTED}Public metadata / no world sprites");
    }
} // namespace ConstructionPreview
