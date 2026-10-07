// SPDX-License-Identifier: GPL-3.0-only
// Isolated loaded scenario, real surface actions, native renderer and production UI.
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/Surface.h>
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
#include <openrct2/paint/Paint.h>
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <sstream>
using namespace OpenRCT2;
namespace Surface = Ui::Handheld::Surface;
static std::vector<Drawing::PaletteIndex> RenderWorld(CoordsXY tile)
{
    Viewport viewport{};
    viewport.width = 512;
    viewport.height = 384;
    const auto focus = Translate3DTo2DWithZ(0, CoordsXYZ{ tile.x + 16, tile.y + 16, MapGetSurfaceElementAt(tile)->getBaseZ() });
    viewport.viewPos = { focus.x - 256, focus.y - 192 };
    Drawing::X8DrawingEngine engine(GetContext()->GetUiContext());
    engine.Resize(512, 384);
    engine.BeginDraw();
    ResetAllSpriteQuadrantPlacements();
    auto& rt = *engine.getRT();
    ViewportRender(rt, &viewport);
    std::vector<Drawing::PaletteIndex> pixels;
    for (int y = 0; y < 384; ++y)
        pixels.insert(pixels.end(), rt.bits + y * rt.LineStride(), rt.bits + y * rt.LineStride() + 512);
    engine.EndDraw();
    return pixels;
}
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
    auto selected = Surface::Next(kObjectEntryIndexNull);
    const auto first = selected;
    while (Surface::Name(selected) != "Grass")
    {
        selected = Surface::Next(selected);
        if (selected == first)
            return 6;
    }
    CoordsXY tile{};
    int best = -1;
    for (int y = 32; y < (state.mapSize.y - 1) * 32; y += 32)
        for (int x = 32; x < (state.mapSize.x - 1) * 32; x += 32)
        {
            const CoordsXY p{ x, y };
            auto* surface = MapGetSurfaceElementAt(p);
            if (!surface || !surface->getOwnership().has(OwnershipFlag::landOwned) || surface->getWaterHeight()
                || surface->getSlope() != 0 || surface->getSurfaceObjectIndex() == selected)
                continue;
            bool clear = true;
            for (auto* e : TileElementsView(p))
                if (e->getType() != TileElementType::surface)
                    clear = false;
            // Avoid foreground scenery hiding the preview in these comparison captures.
            for (int dy = -64; dy <= 96; dy += 32)
                for (int dx = -64; dx <= 96; dx += 32)
                    for (auto* e : TileElementsView(CoordsXY{ x + dx, y + dy }))
                        if (e->getType() == TileElementType::largeScenery || e->getType() == TileElementType::track)
                            clear = false;
            if (!clear)
                continue;
            InteractionInfo info;
            info.Loc = p;
            info.Element = reinterpret_cast<TileElement*>(surface);
            info.interactionType = ViewportInteractionItem::terrain;
            if (Surface::Action(p, selected, false).error != GameActions::Status::ok)
                continue;
            int score = 0;
            for (int dy = -128; dy <= 128; dy += 32)
                for (int dx = -128; dx <= 128; dx += 32)
                    for (auto* e : TileElementsView(CoordsXY{ x + dx, y + dy }))
                        score += e->getType() == TileElementType::track ? 4 : e->getType() == TileElementType::path ? 2 : 0;
            if (score > best)
            {
                const auto before = RenderWorld(p);
                const auto original = surface->getSurfaceObjectIndex();
                Surface::Action(p, selected, true);
                GameActions::ProcessQueue(state);
                const auto after = RenderWorld(p);
                int visible = 0;
                for (int sy = 160; sy < 224; ++sy)
                    for (int sx = 224; sx < 288; ++sx)
                        visible += before[sy * 512 + sx] != after[sy * 512 + sx];
                Surface::Action(p, original, true);
                GameActions::ProcessQueue(state);
                if (visible >= 500)
                {
                    best = score;
                    tile = p;
                }
            }
        }
    if (best < 0)
        return 4;
    std::filesystem::create_directories(argv[4]);
    std::cout << "Park: " << state.park.name << "; target " << tile.x << "," << tile.y << "\n";
    // The camera search performs temporary real surface actions in this isolated park.
    // Delivered frames then show one raise and one lower on the chosen tile.
    for (int mode = 0; mode < 2; ++mode)
    {
        if (mode == 1)
        {
            if (Surface::Action(tile, selected, true).error != GameActions::Status::ok)
                return 5;
            GameActions::ProcessQueue(state);
        }
        const auto result = Surface::Action(tile, selected, false);
        if (result.error != GameActions::Status::ok)
            return 5;
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
        Ui::Handheld::DrawSurfaceOverlay(rt, Ui::Handheld::Concept::kOrigin, Surface::Name(selected));
        Ui::Handheld::DrawGameplayFooter(rt, { 512, 384 }, "", true, true);
        std::ostringstream cost;
        cost << "$" << std::fixed << std::setprecision(2) << result.cost / 100.0;
        Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, cost.str());
        Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
        const char* names[] = { "surface-ready.png", "surface-painted.png" };
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
    return 0;
}
