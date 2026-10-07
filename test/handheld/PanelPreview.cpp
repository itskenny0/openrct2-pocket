// SPDX-License-Identifier: GPL-3.0-only
// Native 512x384 overlay capture; optional backdrop is an explicitly labelled layout composite.
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
    if (argc != 4 && argc != 5)
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
    GamePalette palette{};
    const auto* pal = GfxGetG1Palette(SPR_GAME_DEFAULT_PALETTE);
    for (int i = 0; i < pal->numColours; i++)
        palette[pal->startIndex + i] = { pal->palette[i].blue, pal->palette[i].green, pal->palette[i].red, 255 };
    if (argc == 5)
    {
        std::ifstream background(argv[4], std::ios::binary);
        std::string magic;
        int width{}, height{}, maxValue{};
        background >> magic >> width >> height >> maxValue;
        background.get();
        if (magic != "P6" || width != 512 || height != 384 || maxValue != 255)
            return 3;
        for (int y = 0; y < 384; ++y)
            for (int x = 0; x < 512; ++x)
            {
                unsigned char rgb[3];
                if (!background.read(reinterpret_cast<char*>(rgb), 3))
                    return 3;
                int nearest = 10, distance = 3 * 256 * 256;
                for (int index = 10; index < 230; ++index)
                {
                    const auto& colour = palette[index];
                    const auto r = rgb[0] - colour.red, g = rgb[1] - colour.green, b = rgb[2] - colour.blue;
                    const auto candidate = r * r + g * g + b * b;
                    if (candidate < distance)
                    {
                        distance = candidate;
                        nearest = index;
                    }
                }
                rt.bits[y * rt.LineStride() + x] = static_cast<PaletteIndex>(nearest);
            }
    }
    Ui::Window w;
    w.windowPos = { 8, 37 };
    const auto size = Ui::Handheld::OverlaySize(7);
    w.width = size.width;
    w.height = size.height;
    Ui::Handheld::MenuPresentation model;
    model.title = "PARK / PAUSE";
    model.rows = { "Resume park", "Build", "Park dashboard", "Save park", "Save and exit", "Load park", "Exit without saving" };
    model.icons = { Ui::Handheld::PanelIcon::play, Ui::Handheld::PanelIcon::terrain, Ui::Handheld::PanelIcon::save,
                    Ui::Handheld::PanelIcon::exit, Ui::Handheld::PanelIcon::folder,  Ui::Handheld::PanelIcon::exit };
    model.selected = 1;
    if (std::string(argv[3]) == "terrain")
    {
        Ui::Handheld::DrawTerrainOverlay(rt, { 8, 52 }, true);
        Ui::Handheld::DrawGameplayFooter(rt, { 512, 384 }, "RAISE LAND", true);
        Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, "$20.00");
        Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
    }
    else if (std::string(argv[3]) == "paths")
    {
        Ui::Handheld::DrawPathOverlay(rt, { 8, 52 }, "Tarmac", false, false, "Height 6 / Rise NE");
        Ui::Handheld::DrawPathFooter(rt, { 512, 384 }, "PLACE PATH");
        Ui::Handheld::DrawCostBadge(rt, { 288, 224 }, "$12.00");
        Ui::Handheld::DrawMapFocus(rt, { 256, 192 });
    }
    else if (std::string(argv[3]) == "path-options")
    {
        Ui::Handheld::DrawPathOptions(
            rt, { 512, 384 },
            { "Resume", "Follow ground", "Raise height", "Lower height", "Flat / slope", "Rotate slope", "Surface" }, 2,
            "Height 6 / Rise NE");
    }
    else if (std::string(argv[3]) == "finances")
    {
        w.width = 330;
        w.height = 310;
        w.windowPos = { 8, 37 };
        model.title = "FINANCES";
        model.detailTitle = "Evergreen Gardens";
        model.stats = { { "Cash", "$12,456.00" },
                        { "Bank loan", "$10,000.00" },
                        { "Current profit", "$3,210.00" },
                        { "Expenditure", "$971.30" },
                        { "Park value", "$98,724.00" } };
        model.rows = { "Back to dashboard" };
        model.selected = 0;
        Ui::Handheld::DrawInformationOverlay(rt, w, model);
    }
    else if (std::string(argv[3]) == "build")
    {
        model.title = "BUILD / TERRAIN";
        model.rows = { "Raise land", "Lower land", "Paths", "Back to park" };
        model.icons = { Ui::Handheld::PanelIcon::raise, Ui::Handheld::PanelIcon::lower, Ui::Handheld::PanelIcon::back };
        model.selected = 0;
        model.status = "Choose a tool, then edit in the park.";
        const auto buildSize = Ui::Handheld::OverlaySize(4);
        w.height = buildSize.height;
        w.windowPos.y = (384 - w.height) / 2;
        Ui::Handheld::DrawOverlayShell(rt, w, model);
    }
    else
        Ui::Handheld::DrawOverlayShell(rt, w, model);
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
