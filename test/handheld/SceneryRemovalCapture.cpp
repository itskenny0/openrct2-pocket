// SPDX-License-Identifier: GPL-3.0-only
#include <openrct2-ui/handheld/Currency.h>
// Isolated loaded scenario, real targeted scenery removal, native renderer and production UI.
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2-ui/handheld/SceneryRemoval.h>
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
#include <openrct2/ui/UiContext.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SurfaceElement.h>
#include <openrct2/world/tile_element/TileElement.h>
#include <sstream>
using namespace OpenRCT2;
namespace Removal = Ui::Handheld::SceneryRemoval;
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
    Viewport viewport{};
    viewport.width = 512;
    viewport.height = 384;
    WindowBase window{};
    window.viewport = &viewport;
    std::optional<Removal::Target> selected;
    Viewport chosen{};
    int best = -1;
    int validCount = 0, hitCount = 0;
    for (int y = 32; y < (state.mapSize.y - 1) * 32; y += 32)
        for (int x = 32; x < (state.mapSize.x - 1) * 32; x += 32)
            for (auto* element : TileElementsView<SmallSceneryElement>(CoordsXY{ x, y }))
            {
                const auto* entry = element->getEntry();
                if (!entry || element->isGhost())
                    continue;
                const Removal::Target candidate{ { x, y, element->getBaseZ() },
                                                 element->getSceneryQuadrant(),
                                                 element->getEntryIndex() };
                if (Removal::Action(candidate, false).error != GameActions::Status::ok)
                    continue;
                ++validCount;
                int score = 0;
                for (int dy = -128; dy <= 128; dy += 32)
                    for (int dx = -128; dx <= 128; dx += 32)
                        for (auto* e : TileElementsView(CoordsXY{ x + dx, y + dy }))
                            score += e->getType() == TileElementType::track ? 4 : e->getType() == TileElementType::path ? 2 : 0;
                if (score <= best)
                    continue;
                const auto focus = Translate3DTo2DWithZ(
                    0, CoordsXYZ{ x + 16, y + 16, element->getBaseZ() + entry->height / 2 });
                viewport.viewPos = { focus.x - 256, focus.y - 192 };
                // Require a real visible sprite hit, using exactly the production filter.
                bool found = false;
                for (int dy = -8; dy <= 8 && !found; dy += 4)
                    for (int dx = -8; dx <= 8 && !found; dx += 4)
                    {
                        const auto hit = Removal::FromInfo(
                            GetMapCoordinatesFromPosWindow(&window, { 256 + dx, 192 + dy }, kViewportInteractionItemAll));
                        hitCount += hit.has_value();
                        if (hit && hit->location == candidate.location && hit->quadrant == candidate.quadrant
                            && hit->object == candidate.object)
                        {
                            viewport.viewPos += ScreenCoordsXY{ dx, dy };
                            chosen = viewport;
                            selected = hit;
                            best = score;
                            found = true;
                        }
                    }
            }
    std::cout << "Valid candidates: " << validCount << "; hit candidates: " << hitCount << "\n";
    if (!selected)
        return 4;
    viewport = chosen;
    const auto original = *selected;
    std::filesystem::create_directories(argv[4]);
    std::cout << "Park: " << state.park.name << "; target " << selected->location.x << "," << selected->location.y << " "
              << Removal::Name(selected) << "\n";
    // The camera search performs temporary real targeted scenery removal in this isolated park.
    // Delivered frames then show one raise and one lower on the chosen tile.
    for (int mode = 0; mode < 2; ++mode)
    {
        if (mode == 1)
        {
            if (Removal::Action(original, true).error != GameActions::Status::ok)
                return 5;
            GameActions::ProcessQueue(state);
        }
        selected = Removal::FromInfo(GetMapCoordinatesFromPosWindow(&window, { 256, 192 }, kViewportInteractionItemAll));
        const auto result = Removal::Action(selected, false);
        Drawing::X8DrawingEngine engine(context->GetUiContext());
        engine.Resize(512, 384);
        engine.BeginDraw();
        auto& rt = *engine.getRT();
        ResetAllSpriteQuadrantPlacements();
        Drawing::UpdatePaletteEffects();
        ViewportRender(rt, &viewport);
        Ui::Handheld::DrawSceneryRemovalOverlay(
            rt, Ui::Handheld::Concept::kOrigin, Removal::Name(selected), selected.has_value());
        Ui::Handheld::DrawSceneryRemovalFooter(rt, { 512, 384 });
        const auto cost = Ui::Handheld::FormatMoney(result.cost);
        if (result.error == GameActions::Status::ok)
            Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, cost);
        else
            Ui::Handheld::DrawContextMessage(rt, { 288, 224 }, result.getErrorMessage());
        Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
        const char* names[] = { "scenery-target.png", "scenery-removed.png" };
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
