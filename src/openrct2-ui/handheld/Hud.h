// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "MenuFeel.h"
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
        int cashTrend{}; // +1 counting up after income, -1 after spending, 0 settled.
    };

    // The cash readout counts toward the real balance instead of jumping, and briefly tints
    // green or gold after a noticeable change. Purely presentational: the park value is untouched.
    class CashTicker
    {
        money64 _from{}, _target{};
        uint64_t _start{};
        bool _ready{};
        int _trend{};

    public:
        static constexpr uint64_t kMs = 450;
        static constexpr money64 kNoticeable = 500; // £50 in money64 units.
        money64 Value(money64 actual, uint64_t now, bool reduceMotion)
        {
            if (!_ready || reduceMotion)
            {
                _from = _target = actual;
                _ready = true;
                _trend = 0;
                return actual;
            }
            if (actual != _target)
            {
                const auto shown = Shown(now);
                const auto delta = actual - shown;
                if (delta >= kNoticeable || delta <= -kNoticeable)
                    _trend = delta > 0 ? 1 : -1;
                _from = shown;
                _target = actual;
                _start = now;
            }
            if (now - _start >= kMs)
                _trend = 0;
            return Shown(now);
        }
        money64 Shown(uint64_t now) const
        {
            if (now - _start >= kMs)
                return _target;
            const double t = static_cast<double>(now - _start) / kMs;
            const double eased = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
            return _from + static_cast<money64>((_target - _from) * eased);
        }
        int Trend() const
        {
            return _trend;
        }
    };
    inline CashTicker cashTicker;
    inline HudPresentation NativeHud()
    {
        const auto& state = getGameState();
        const auto& date = state.date;
        const bool noMoney = state.park.flags.has(ParkFlag::noMoney);
        const auto shown = noMoney ? state.park.cash : cashTicker.Value(state.park.cash, MenuFeel::Clock(), MenuFeel::reduceMotion);
        return { noMoney ? "No money" : FormatMoney(shown),
                 std::to_string(date.GetDay() + 1) + " " + FormatStringID(DateGameShortMonthNames[date.GetMonth()]) + " Y"
                     + std::to_string(date.GetYear() + 1),
                 std::to_string(state.park.numGuestsInPark),
                 GameIsPaused(),
                 state.park.cash < 0,
                 noMoney,
                 static_cast<uint16_t>(gGameSpeed >= 1 && gGameSpeed <= 8 ? 1u << (gGameSpeed - 1) : 0),
                 noMoney ? 0 : cashTicker.Trend() };
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
                model.noMoney             ? "{MUTED}"
                    : model.cashTrend > 0 ? "{POSITIVE}"
                    : model.negativeCash || model.cashTrend < 0 ? "{GOLD}"
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
