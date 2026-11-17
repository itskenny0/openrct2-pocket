// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ControllerInput.h"
#include "Layout.h"
#include "Panel.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld::Guide
{
    // The controls guide (Park menu > Controls) and the first-run intro share one picture of the
    // device: face buttons, D-pad, shoulders, SELECT and START, with the buttons that the current
    // page talks about ringed. Button names follow the live mapping (A/B can be swapped), so the
    // picture and the list always agree with what the buttons do.
    struct Line
    {
        std::string key; // As printed: "A", "D-pad", "L1/R1", "SELECT"
        std::string text;
    };

    inline std::string Key(Action action)
    {
        return std::string(ActionKey(action));
    }

    inline constexpr std::array<const char*, 4> kTabs{ "Park", "Build", "Menus", "Shortcuts" };

    inline std::vector<Line> Tab(size_t tab)
    {
        switch (tab)
        {
            case 0:
                return { { "D-pad", "Move the cross" },
                         { Key(Action::confirm), "Open what is under it" },
                         { Key(Action::back), "Back" },
                         { Key(Action::contextPrimary), "Turn the view" },
                         { Key(Action::contextSecondary), "Next alert" },
                         { "L1/R1", "Zoom" },
                         { Key(Action::openBuild), "Build" },
                         { Key(Action::openMenu), "Park menu" } };
            case 1:
                return { { "D-pad", "Move or aim" },
                         { Key(Action::confirm), "Place or apply" },
                         { Key(Action::back), "Leave the tool" },
                         { Key(Action::contextSecondary), "Tool options" },
                         { "L2/R2", "Height, slope or brush" },
                         { Key(Action::openBuild), "More options" },
                         { "L1/R1", "Zoom" } };
            case 2:
                return { { "D-pad", "Choose, or change a value" },
                         { Key(Action::confirm), "Select" },
                         { Key(Action::back), "Back" },
                         { "L1/R1", "Tabs" },
                         { Key(Action::contextSecondary), "Category or filter" },
                         { Key(Action::openMenu), "Close the menu" } };
            default:
                return { { "L2", "Hold: quick menu" },
                         { Key(Action::back), "Hold in a tool: undo" },
                         { Key(Action::contextSecondary), "Hold in the park: inbox" },
                         { Key(Action::contextSecondary), "Placing an entrance: auto" },
                         { Key(Action::confirm), "On a path end: extend it" },
                         { Key(Action::confirm), "Riding along: next train" },
                         { "L2", "Then " + Key(Action::contextSecondary) + ": photo mode" } };
        }
    }

    // Does this printed key involve that button? "L1/R1" rings both shoulders.
    inline bool Mentions(const std::vector<std::string>& keys, const std::string& button)
    {
        for (const auto& key : keys)
        {
            if (key == button)
                return true;
            if (const auto slash = key.find('/'); slash != std::string::npos)
                if (key.substr(0, slash) == button || key.substr(slash + 1) == button)
                    return true;
        }
        return false;
    }

    // The device, 200 x 168: shoulders above the body, screen at the top of the body, D-pad
    // bottom left, face buttons bottom right, SELECT and START between them.
    inline constexpr ScreenSize kDeviceSize{ 200, 168 };
    inline void DrawDevice(Drawing::RenderTarget& rt, ScreenCoordsXY p, const std::vector<std::string>& ringed)
    {
        const auto edge = static_cast<Drawing::PaletteIndex>(PocketPalette::edge);
        const auto ring = static_cast<Drawing::PaletteIndex>(PocketPalette::selection);
        const auto muted = static_cast<Drawing::PaletteIndex>(PocketPalette::separator);
        auto pill = [&](ScreenCoordsXY at, const std::string& name) {
            const int width = ControlKeyWidth(name);
            if (Mentions(ringed, name))
                FillRound(rt, at - ScreenCoordsXY{ 3, 3 }, { width + 6, 21 }, ring);
            DrawCommand(rt, at + ScreenCoordsXY{ 0, 3 }, 0, name.c_str(), "");
        };
        // Shoulders sit above the body, as on the back edge of the device.
        pill(p + ScreenCoordsXY{ 6, 0 }, "L2");
        pill(p + ScreenCoordsXY{ 40, 0 }, "L1");
        pill(p + ScreenCoordsXY{ kDeviceSize.width - 64, 0 }, "R1");
        pill(p + ScreenCoordsXY{ kDeviceSize.width - 30, 0 }, "R2");
        // Body and screen.
        const auto body = p + ScreenCoordsXY{ 0, 24 };
        FillRound(rt, body, { kDeviceSize.width, kDeviceSize.height - 24 }, edge);
        FillRound(rt, body + ScreenCoordsXY{ 2, 2 }, { kDeviceSize.width - 4, kDeviceSize.height - 28 }, Theme::raised);
        Fill(rt, body + ScreenCoordsXY{ 40, 10 }, { kDeviceSize.width - 80, 50 }, muted);
        // D-pad: a cross, ringed as a whole.
        const auto pad = body + ScreenCoordsXY{ 38, 96 };
        if (Mentions(ringed, "D-pad"))
            FillRound(rt, pad - ScreenCoordsXY{ 22, 22 }, { 45, 45 }, ring);
        Fill(rt, pad - ScreenCoordsXY{ 6, 18 }, { 13, 37 }, Theme::text);
        Fill(rt, pad - ScreenCoordsXY{ 18, 6 }, { 37, 13 }, Theme::text);
        Fill(rt, pad - ScreenCoordsXY{ 2, 2 }, { 5, 5 }, edge);
        // Face buttons in the usual diamond: X top, Y left, A right, B bottom.
        const auto face = body + ScreenCoordsXY{ kDeviceSize.width - 40, 96 };
        const std::array<std::pair<const char*, ScreenCoordsXY>, 4> buttons{
            { { "X", { 0, -17 } }, { "Y", { -17, 0 } }, { "A", { 17, 0 } }, { "B", { 0, 17 } } }
        };
        for (const auto& [name, offset] : buttons)
        {
            const auto centre = face + offset;
            if (Mentions(ringed, name))
                FillRound(rt, centre - ScreenCoordsXY{ 9, 9 }, { 19, 19 }, ring);
            DrawButtonGlyph(rt, centre, name);
        }
        // SELECT and START between the controls.
        pill(body + ScreenCoordsXY{ 70, 74 }, "SELECT");
        pill(body + ScreenCoordsXY{ 78, 102 }, "START");
    }

    // The guide page: tabs, the device, and the list for the chosen tab.
    inline void DrawPage(Drawing::RenderTarget& rt, ScreenSize canvas, size_t tab)
    {
        const int w = canvas.width - 16, h = Layout::ContentHeight(canvas);
        DrawSurface(rt, { 8, 8 }, { w, h });
        DrawHeadingText(rt, { 20, 19 }, "{WHITE}Controls", w - 24);
        DrawTabs(rt, { 20, 44 }, w - 24, { kTabs.begin(), kTabs.end() }, tab);
        const auto lines = Tab(tab);
        std::vector<std::string> keys;
        for (const auto& line : lines)
            keys.push_back(line.key);
        DrawDevice(rt, { 20, 92 }, keys);
        int y = 96;
        const int step = lines.size() > 7 ? 28 : 31;
        for (const auto& line : lines)
        {
            DrawCommand(rt, { 236, y }, 0, line.key.c_str(), line.text.c_str());
            y += step;
        }
        DrawModernEllipsised(rt, { 20, h - 10 }, w - 24, "{MUTED}Every screen lists its own buttons along the bottom.");
        DrawFooter(
            rt, canvas, { { "D-pad", "Section" }, { Key(Action::contextSecondary), "Intro" }, { Key(Action::back), "Back" } });
    }

    // The first-run intro: four cards, each with the buttons it is about ringed on the device.
    struct Card
    {
        const char* title;
        std::string body;
        std::vector<std::string> keys;
    };
    inline std::vector<Card> Cards()
    {
        return {
            { "Welcome to your park",
              "Move the cross over the park with the D-pad. L1 and R1 zoom, and " + Key(Action::contextPrimary)
                  + " turns the view.",
              { "D-pad", "L1/R1", Key(Action::contextPrimary) } },
            { "Point and open",
              Key(Action::confirm) + " opens whatever is under the cross: a ride, a shop or a guest. " + Key(Action::back)
                  + " goes back.",
              { Key(Action::confirm), Key(Action::back) } },
            { "Build and run the park",
              Key(Action::openBuild) + " opens Build. " + Key(Action::openMenu)
                  + " opens the park menu: rides, guests, staff, money and settings.",
              { Key(Action::openBuild), Key(Action::openMenu) } },
            { "Shortcuts",
              "Hold L2 for the quick menu. " + Key(Action::contextSecondary)
                  + " steps through alerts; hold it for the inbox. In a tool, hold " + Key(Action::back) + " to undo.",
              { "L2", Key(Action::contextSecondary), Key(Action::back) } },
        };
    }
    inline void DrawIntro(Drawing::RenderTarget& rt, ScreenSize canvas, size_t index)
    {
        const auto cards = Cards();
        index = std::min(index, cards.size() - 1);
        const auto& card = cards[index];
        const int w = canvas.width - 16, h = Layout::ContentHeight(canvas);
        DrawSurface(rt, { 8, 8 }, { w, h });
        DrawModernText(
            rt, { 20, 20 }, "{MUTED}Controls intro  " + std::to_string(index + 1) + " / " + std::to_string(cards.size()));
        DrawDevice(rt, { 20, 60 }, card.keys);
        DrawHeadingText(rt, { 236, 66 }, std::string("{WHITE}") + card.title, w - 236);
        DrawWrappedLabel(rt, { 236, 96 }, w - 236, card.body, 7);
        // Progress dots.
        for (size_t i = 0; i < cards.size(); ++i)
            FillRound(
                rt, { 236 + static_cast<int>(i) * 16, h - 34 }, { 9, 9 },
                static_cast<Drawing::PaletteIndex>(i == index ? PocketPalette::selection : PocketPalette::separator));
        const bool last = index + 1 == cards.size();
        DrawFooter(
            rt, canvas,
            { { Key(Action::confirm), last ? "Start playing" : "Next" },
              { "D-pad", "Previous / next" },
              { Key(Action::back), last ? "Close" : "Skip" } });
    }
} // namespace OpenRCT2::Ui::Handheld::Guide
