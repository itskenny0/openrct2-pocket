// SPDX-License-Identifier: GPL-3.0-only
#include <openrct2-ui/handheld/Currency.h>
// Isolated native scenario, production picker previews and shop ghost and real placement.
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/Shops.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/audio/AudioContext.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Imaging.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/X8DrawingEngine.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/paint/Paint.h>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/PathElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <sstream>
using namespace OpenRCT2;
namespace Shops = Ui::Handheld::Shops;
int main(int argc, char** argv)
{
    if (argc != 6)
        return 2;
    gCustomOpenRCT2DataPath = argv[1];
    gCustomUserDataPath = argv[5];
    gOpenRCT2Headless = true;
    gOpenRCT2NoGraphics = false;
    auto env = CreatePlatformEnvironment();
    env->SetBasePath(DirBase::rct2, argv[2]);
    auto context = CreateContext(std::move(env), Audio::CreateDummyAudioContext(), Ui::CreateDummyUiContext());
    Config::SetDefaults();
    Config::Get().general.rct2Path = argv[2];
    if (!context->Initialise() || !context->LoadParkFromFile(argv[3], false, true))
        return 3;
    gGamePaused = 0;
    auto& state = getGameState();
    const auto entries = Shops::Entries();
    const auto selected = entries.empty() ? kObjectEntryIndexNull : entries.front().identity.index;
    if (selected == kObjectEntryIndexNull)
        return 6;
    CoordsXY tile{};
    Direction direction{};
    int best = -1;
    for (int y = 32; y < (state.mapSize.y - 1) * 32; y += 32)
        for (int x = 32; x < (state.mapSize.x - 1) * 32; x += 32)
        {
            const CoordsXY p{ x, y };
            auto* surface = MapGetSurfaceElementAt(p);
            if (!surface || !surface->getOwnership().has(OwnershipFlag::landOwned) || surface->getWaterHeight()
                || surface->getSlope() != 0)
                continue;
            bool clear = true;
            for (auto* e : TileElementsView(p))
                if (e->getType() != TileElementType::surface)
                    clear = false;
            if (!clear)
                continue;
            std::optional<Direction> facingPath;
            const auto sides = TrackMetadata::GetTrackElementDescriptor(
                                   GetRideTypeDescriptor(ShopPlacement::Type(selected)).StartTrackPiece)
                                   .sequenceData.sequences[0]
                                   .getEntranceConnectionSides();
            for (Direction d = 0; d < 4; ++d)
                for (Direction side = 0; side < 4; ++side)
                    if (sides & (1 << side))
                        for (const auto* path : TileElementsView<PathElement>(p + CoordsDirectionDelta[(d + side) & 3]))
                            if (!path->isGhost() && !path->isQueue() && !path->isSloped()
                                && path->getBaseZ() == surface->getBaseZ())
                                facingPath = d;
            if (!facingPath)
                continue;
            if (Shops::Action(p, selected, *facingPath, false).error != GameActions::Status::ok)
                continue;
            int score = 0;
            for (int dy = -128; dy <= 128; dy += 32)
                for (int dx = -128; dx <= 128; dx += 32)
                    for (auto* e : TileElementsView(CoordsXY{ x + dx, y + dy }))
                        score += e->getType() == TileElementType::track ? 4 : e->getType() == TileElementType::path ? 2 : 0;
            if (score > best)
            {
                Viewport viewport{};
                viewport.width = 512;
                viewport.height = 384;
                const auto focus = Translate3DTo2DWithZ(0, CoordsXYZ{ x + 16, y + 16, MapGetSurfaceElementAt(p)->getBaseZ() });
                viewport.viewPos = { focus.x - 256, focus.y - 192 };
                WindowBase window{};
                window.viewport = &viewport;
                ResetAllSpriteQuadrantPlacements();
                const auto hit = GetMapCoordinatesFromPosWindow(&window, { 256, 192 }, kViewportInteractionItemAll);
                if (hit.interactionType != ViewportInteractionItem::terrain || hit.Loc.x != x || hit.Loc.y != y)
                    continue;
                best = score;
                tile = p;
                direction = *facingPath;
            }
        }
    if (best < 0)
        return 4;
    std::filesystem::create_directories(argv[4]);
    std::cout << "Park: " << state.park.name << "; shop " << Shops::Name(selected) << "; target " << tile.x << "," << tile.y
              << "\n";

    Ui::Handheld::ObjectPicker picker;
    picker.setEntries(entries, entries.front().identity);
    const auto initialCash = state.park.cash;
    std::cout << entries.size() << " eligible shops / facilities; " << picker.pages() << " pages\n";
    const auto cost = Shops::Action(tile, selected, direction, false).cost;
    for (int mode = 0; mode < 3; ++mode)
    {
        gGamePaused = mode == 0 ? 1 : 0;
        Shops::ClearPreview();
        const auto chosen = selected;
        if (mode == 1)
        {
            Ui::Handheld::SetShopSelection(chosen);
            Shops::UpdatePreview(tile, chosen, direction);
        }
        if (mode == 2)
        {
            if (Shops::Action(tile, chosen, direction, true).error != GameActions::Status::ok)
                return 5;
            GameActions::ProcessQueue(state);
            if (state.park.cash != initialCash - cost)
                return 7;
            bool placed = false;
            for (const auto* e : TileElementsView<TrackElement>(tile))
                if (!e->isGhost() && GetRide(e->getRideIndex())->status == RideStatus::open)
                    placed = true;
            if (!placed)
                return 8;
            bool connected = false;
            const auto sides = TrackMetadata::GetTrackElementDescriptor(
                                   GetRideTypeDescriptor(ShopPlacement::Type(chosen)).StartTrackPiece)
                                   .sequenceData.sequences[0]
                                   .getEntranceConnectionSides();
            for (Direction side = 0; side < 4; ++side)
                if (sides & (1 << side))
                {
                    const Direction edge = (direction + side) & 3;
                    for (const auto* path : TileElementsView<PathElement>(tile + CoordsDirectionDelta[edge]))
                        if (!path->isGhost() && (path->getEdges() & (1 << DirectionReverse(edge))))
                            connected = true;
                }
            if (!connected)
                return 9;
        }
        else if (state.park.cash != initialCash)
            return 7;
        auto* surface = MapGetSurfaceElementAt(tile);
        Viewport viewport{};
        viewport.width = 512;
        viewport.height = 384;
        const auto focus = Translate3DTo2DWithZ(0, CoordsXYZ{ tile.x + 16, tile.y + 16, surface->getBaseZ() });
        viewport.viewPos = { focus.x - 256, focus.y - 192 };
        Drawing::X8DrawingEngine engine(context->GetUiContext());
        engine.Resize(512, 384);
        engine.BeginDraw();
        auto& rt = *engine.getRT();
        ResetAllSpriteQuadrantPlacements();
        Drawing::UpdatePaletteEffects();
        ViewportRender(rt, &viewport);
        if (mode == 0)
            Ui::Handheld::DrawObjectPicker(rt, { 512, 384 }, picker, "Shops / Facilities");
        else
        {
            Ui::Handheld::DrawShopOverlay(rt, Ui::Handheld::Concept::kOrigin, Shops::Name(chosen), true, direction);
            Ui::Handheld::DrawShopFooter(rt, { 512, 384 });
            const auto amount = Ui::Handheld::FormatMoney(cost);
            if (mode == 1)
                Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, amount);
            else
                Ui::Handheld::DrawContextMessage(rt, { 288, 224 }, "Placed / Open");
            Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
        }
        const char* names[] = { "shop-picker.png", "shop-ghost.png", "shop-placed.png" };
        Image image;
        image.Width = 512;
        image.Height = 384;
        image.Depth = 8;
        image.Stride = rt.LineStride();
        image.Palette = Drawing::gPalette;
        auto* pixels = reinterpret_cast<uint8_t*>(rt.bits);
        image.Pixels.assign(pixels, pixels + rt.LineStride() * rt.height);
        Imaging::WriteToFile((std::filesystem::path(argv[4]) / names[mode]).string(), image, ImageFormat::png);
        engine.EndDraw();
    }
    Shops::ClearPreview();
    return 0;
}
