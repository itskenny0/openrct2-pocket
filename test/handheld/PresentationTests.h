// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2-ui/handheld/Hud.h>

static void TestPocketPresentation()
{
    namespace H = OpenRCT2::Ui::Handheld;
    namespace L = H::Layout;
    const ScreenSize screen{ 512, 384 };
    Require(L::ShowsHud(L::Context::park) && L::ShowsHud(L::Context::construction), "Park HUD visibility lost");
    for (auto context :
         { L::Context::picker, L::Context::inspection, L::Context::dashboard, L::Context::review, L::Context::pause })
        Require(!L::ShowsHud(context), "Dense/modal content must own the HUD safe area");
    Require(L::toolOrigin.y >= L::hudTop + L::hudHeight + L::gutter, "Tool overlaps HUD");
    Require(L::toolOrigin.y + H::kTerrainOverlaySize.height < screen.height - L::actionHeight, "Paths overlaps footer");
    Require(
        L::menuOrigin.y + L::ContentHeight(screen) == screen.height - L::actionHeight - L::margin,
        "Menu safe area overlaps action strip");
    for (auto p : { ScreenCoordsXY{ -500, -500 }, ScreenCoordsXY{ 1000, 1000 }, ScreenCoordsXY{ 288, 224 } })
    {
        const auto fitted = L::ContextOrigin(screen, p, H::kContextSize);
        Require(fitted.x >= L::margin && fitted.y >= L::toolOrigin.y, "Feedback escapes top/left safe area");
        Require(
            fitted.x + H::kContextSize.width <= screen.width - L::margin
                && fitted.y + H::kContextSize.height <= screen.height - L::actionHeight - L::gutter,
            "Feedback escapes right/bottom safe area");
    }
    auto& state = getGameState();
    const auto endpointFeedback = L::ContextAnchor(screen, H::kContextSize, true);
    Require(
        endpointFeedback.x + H::kContextSize.width == screen.width - L::margin
            && endpointFeedback.y + H::kContextSize.height == screen.height - L::actionHeight - L::gutter,
        "Endpoint feedback/redraw anchor escapes construction safe area");
    const auto oldDate = state.date;
    const auto oldCash = state.park.cash;
    const auto oldGuests = state.park.numGuestsInPark;
    const auto oldFlags = state.park.flags;
    const auto oldPause = gGamePaused, oldSpeed = gGameSpeed;
    state.park.flags.unset(ParkFlag::noMoney);
    state.park.numGuestsInPark = 100000;
    state.date = Date::FromYMD(32, MONTH_OCTOBER, 30);
    for (money64 cash : { money64{ 0 }, money64{ -10000000 }, money64{ 100000000000 } })
        for (uint8_t speed : { 1, 2, 3, 4, 8 })
            for (uint8_t paused : { 0, 1, 2 })
            {
                state.park.cash = cash;
                gGameSpeed = speed;
                gGamePaused = paused;
                const auto hud = H::NativeHud();
                Require(
                    hud.cash == H::FormatMoney(cash) && hud.negativeCash == (cash < 0),
                    "HUD currency diverged from native formatting");
                Require(
                    hud.guests == "100000" && hud.paused == GameIsPaused() && hud.speed == (1u << (speed - 1)),
                    "HUD invented guest/simulation state");
                Require(
                    hud.date.find(FormatStringID(DateGameShortMonthNames[state.date.GetMonth()])) != std::string::npos
                        && hud.date.ends_with("Y33"),
                    "HUD native date/year mismatch");
            }
    state.park.flags.set(ParkFlag::noMoney);
    Require(H::NativeHud().noMoney && H::NativeHud().cash == "No money", "No-money HUD reported spendable cash");
    state.date = oldDate;
    state.park.cash = oldCash;
    state.park.numGuestsInPark = oldGuests;
    state.park.flags = oldFlags;
    gGamePaused = oldPause;
    gGameSpeed = oldSpeed;
    std::cout << "Pocket native HUD / dates / cash / guests / pause-speed / no-money / park-menu safe areas passed\n";
}
