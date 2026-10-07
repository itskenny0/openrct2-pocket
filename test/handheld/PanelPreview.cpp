// SPDX-License-Identifier: GPL-3.0-only
// Native 512x384 shell capture. Park information is a fixture, not a loaded save.
#include <fstream>
#include <openrct2-ui/handheld/Panel.h>
#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.Sprite.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/G1Element.h>
#include <openrct2/drawing/Palette.h>
#include <openrct2/drawing/RenderTarget.h>
#include <openrct2/drawing/X8DrawingEngine.h>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
int main(int argc, char** argv)
{
    if (argc != 4)
        return 2;
    gCustomOpenRCT2DataPath = argv[1];
    auto context = CreateContext();
    Config::SetDefaults();
    GfxLoadG2PalettesFontsTracks();
    initColourMaps();
    FontSpriteInitialiseCharacters();
    X8DrawingEngine engine(context->GetUiContext());
    engine.Resize(512, 384);
    engine.BeginDraw();
    auto& rt = *engine.getRT();
    GfxClear(rt, PaletteIndex::pi10);
    Ui::Window w;
    w.windowPos = { 9, 21 };
    w.width = 494;
    w.height = 342;
    Ui::Handheld::MenuPresentation model;
    model.title = "PARK / PAUSE";
    model.section = "PARK ACTIONS";
    model.detailTitle = "Evergreen Gardens";
    model.detailSubtitle = "Park paused";
    model.stats = { { "Guests", "812" }, { "Park rating", "728" }, { "Save slots", "3" } };
    model.rows = { "Resume park", "Build", "Save park", "Save and exit", "Load park", "Exit without saving" };
    model.icons = { Ui::Handheld::PanelIcon::play, Ui::Handheld::PanelIcon::terrain, Ui::Handheld::PanelIcon::save,
                    Ui::Handheld::PanelIcon::exit, Ui::Handheld::PanelIcon::folder,  Ui::Handheld::PanelIcon::exit };
    model.help = { "A  Open selected action", "B  Return to your park", "D-pad  Move through menu" };
    model.selected = 1;
    if (std::string(argv[3]) == "terrain")
    {
        model.title = "BUILD / TERRAIN";
        model.section = "LAND TOOLS";
        model.tabs = { "Terrain" };
        model.detailSubtitle = "Terrain / single tile";
        model.rows = { "Terrain: raise land", "Terrain: lower land", "Back to park" };
        model.icons = { Ui::Handheld::PanelIcon::raise, Ui::Handheld::PanelIcon::lower, Ui::Handheld::PanelIcon::back };
        model.help = { "A  Edit tile under cursor", "Y  Switch raise / lower", "B  Finish editing" };
        model.selected = 0;
        model.status = "Select a tool, then move the cursor in your park.";
    }
    Ui::Handheld::DrawMenuShell(rt, w, model);
    GamePalette palette{};
    const auto* pal = GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
    for (int i = 0; i < pal->numColours; i++)
        palette[pal->startIndex + i] = { pal->palette[i].blue, pal->palette[i].green, pal->palette[i].red, 255 };
    std::ofstream out(argv[2], std::ios::binary);
    out << "P6\n512 384\n255\n";
    for (int y = 0; y < 384; y++)
        for (int x = 0; x < 512; x++)
        {
            auto c = palette[static_cast<unsigned>(rt.bits[y * rt.LineStride() + x])];
            char rgb[] = { static_cast<char>(c.red), static_cast<char>(c.green), static_cast<char>(c.blue) };
            out.write(rgb, 3);
        }
}
