// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConceptStyle.h"
#include "Currency.h"
#include "Layout.h"

#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/localisation/Localisation.Date.h>

namespace OpenRCT2::Ui::Handheld
{
    struct HudPresentation
    {
        std::string cash, date, guests;
        bool paused{}, negativeCash{}, noMoney{};
        uint16_t speed = 1;
    };
    inline HudPresentation NativeHud()
    {
        const auto& state = getGameState();
        const auto& date = state.date;
        const bool noMoney = state.park.flags.has(ParkFlag::noMoney);
        return { noMoney ? "No money" : FormatMoney(state.park.cash),
                 std::to_string(date.GetDay() + 1) + " " + FormatStringID(DateGameShortMonthNames[date.GetMonth()]) + " Y"
                     + std::to_string(date.GetYear() + 1),
                 std::to_string(state.park.numGuestsInPark),
                 GameIsPaused(),
                 state.park.cash < 0,
                 noMoney,
                 static_cast<uint16_t>(gGameSpeed >= 1 && gGameSpeed <= 8 ? 1u << (gGameSpeed - 1) : 0) };
    }
    inline void DrawHud(Drawing::RenderTarget& rt, ScreenSize screen, const HudPresentation& model)
    {
        using namespace Concept;
        const ScreenCoordsXY p{ Layout::margin, Layout::hudTop };
        const auto cells = Layout::HudLayout(screen);
        Box(rt, p, { cells.width, Layout::hudHeight });
        for (int x : { cells.cashEnd, cells.dateEnd, cells.guestsEnd })
            GfxDrawLine(
                rt, { p + ScreenCoordsXY{ x, 6 }, p + ScreenCoordsXY{ x, 17 } },
                static_cast<Drawing::PaletteIndex>(PocketPalette::separator));
        DrawStrongText(
            rt, p + ScreenCoordsXY{ 8, 7 },
            std::string(
                model.noMoney            ? "{MUTED}"
                    : model.negativeCash ? "{GOLD}"
                                         : "{WHITE}")
                + model.cash,
            cells.cashEnd - 16);
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ cells.cashEnd + 8, 7 }, cells.dateEnd - cells.cashEnd - 16, "{WHITE}" + model.date);
        // Small palette-safe people glyph. It uses the same line vocabulary as Pocket tool icons.
        const auto guest = p + ScreenCoordsXY{ cells.dateEnd + 7, 5 };
        for (int i : { 0, 5 })
        {
            GfxDrawLine(
                rt, { guest + ScreenCoordsXY{ i + 1, 1 }, guest + ScreenCoordsXY{ i + 3, 1 } },
                static_cast<Drawing::PaletteIndex>(PocketPalette::secondary));
            for (int x = i; x < i + 5; ++x)
                GfxDrawLine(
                    rt, { guest + ScreenCoordsXY{ x, 5 }, guest + ScreenCoordsXY{ x, 11 } },
                    static_cast<Drawing::PaletteIndex>(PocketPalette::secondary));
        }
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ cells.dateEnd + 23, 7 }, cells.guestsEnd - cells.dateEnd - 30, "{WHITE}" + model.guests);
        DrawModernEllipsised(
            rt, p + ScreenCoordsXY{ cells.guestsEnd + 7, 7 }, cells.width - cells.guestsEnd - 14,
            model.paused      ? "{GOLD}Pause"
                : model.speed ? "{WHITE}" + std::to_string(model.speed) + "x"
                              : "{MUTED}-");
    }
} // namespace OpenRCT2::Ui::Handheld
