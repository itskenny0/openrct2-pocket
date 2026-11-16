// SPDX-License-Identifier: GPL-3.0-only
// Isolated native scenario: build a native shop, then capture production management UI.
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <openrct2-ui/handheld/Explorer.h>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/ShopManagement.h>
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

    namespace M = Ui::Handheld::ShopManagement;
    const auto cost = Shops::Action(tile, selected, direction, false).cost;
    const auto cash = state.park.cash;
    if (Shops::Action(tile, selected, direction, true).error != GameActions::Status::ok)
        return 5;
    GameActions::ProcessQueue(state);
    if (state.park.cash != cash - cost)
        return 7;
    const auto id = (*TileElementsView<TrackElement>(tile).begin())->getRideIndex();
    const auto target = M::Capture(id);
    if (!target)
        return 8;
    const auto prices = M::Prices(*target);
    if (prices.empty())
        return 9;
    const auto draft = std::min(prices[0].value + 1, kRideMaxPrice);
    for (int mode = 0; mode < 3; ++mode)
    {
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
        WindowBase window{};
        window.viewport = &viewport;
        bool found = false;
        for (int dy = -20; dy <= 10 && !found; ++dy)
        {
            const auto hit = GetMapCoordinatesFromPosWindow(&window, { 256, 192 + dy }, { ViewportInteractionItem::ride });
            if (const auto inspected = M::FromInfo(hit))
                found = inspected->id == id;
        }
        if (!found)
            return 10;
        M::Draw(
            rt, { 512, 384 }, target,
            mode == 0       ? M::View::inspection
                : mode == 1 ? M::View::prices
                            : M::View::demolition,
            mode == 2 ? 1 : 0, mode == 1 ? std::optional<money64>{ draft } : std::nullopt);
        const char* names[] = { "shop-inspection.png", "shop-prices.png", "shop-demolition.png" };
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
