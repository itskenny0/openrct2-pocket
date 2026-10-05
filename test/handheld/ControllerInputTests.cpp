// SPDX-License-Identifier: GPL-3.0-only
// Integration checks against SDL virtual controllers; no original game assets required.
#include "../../src/openrct2-ui/handheld/ControllerInput.h"

#include <SDL.h>
#include <cstdlib>
#include <iostream>

using namespace OpenRCT2::Ui::Handheld;

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << " (" << SDL_GetError() << ")\n";
        std::exit(1);
    }
}

static void Pump(ControllerInput& input)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
        input.handleEvent(e);
}

int main()
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    Check(SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "SDL initialises");
    {
        ControllerInput input;
        Check(input.update(true, 100).actions.empty(), "no device is safe");
        const auto index = SDL_JoystickAttachVirtual(
            SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        Check(index >= 0, "virtual device attached");
        auto* joystick = SDL_JoystickOpen(index);
        Check(joystick != nullptr, "virtual joystick opened");
        Pump(input);
        input.update(true, 100);
        Pump(input);
        input.update(true, 110);

        auto press = [&](SDL_GameControllerButton button, bool down) {
            Check(SDL_JoystickSetVirtualButton(joystick, button, down ? 1 : 0) == 0, "set virtual button");
            SDL_JoystickUpdate();
            Pump(input);
        };

        press(SDL_CONTROLLER_BUTTON_A, true);
        auto frame = input.update(true, 120);
        Check(frame.actions.size() == 1 && frame.actions[0] == Action::inspect, "A emits exactly one mapped action");
        Check(input.update(true, 130).actions.empty(), "held A does not repeat");
        press(SDL_CONTROLLER_BUTTON_A, false);
        input.update(true, 140);

        press(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, true);
        frame = input.update(true, 190);
        Check(frame.panX == 6 && frame.panY == 0, "D-pad pans at time-based speed");
        frame = input.update(true, 1190);
        Check(frame.panX == 6, "long stalls are clamped");
        press(SDL_CONTROLLER_BUTTON_DPAD_LEFT, true);
        Check(input.update(true, 1240).panX == 0, "opposed directions cancel");
        press(SDL_CONTROLLER_BUTTON_DPAD_LEFT, false);
        input.update(false, 1250);
        Check(input.update(true, 1300).panX == 0, "focus regain cannot resume held movement");
        press(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, false);
        input.update(true, 1310);
        press(SDL_CONTROLLER_BUTTON_DPAD_UP, true);
        Check(input.update(true, 1360).panY == -6, "release rearms movement");
        press(SDL_CONTROLLER_BUTTON_DPAD_UP, false);

        press(SDL_CONTROLLER_BUTTON_B, true);
        input.update(false, 1370);
        Check(input.update(true, 1380).actions.empty(), "focus loss discards queued actions");
        press(SDL_CONTROLLER_BUTTON_B, false);
        input.update(true, 1390);
        press(SDL_CONTROLLER_BUTTON_B, true);
        frame = input.update(true, 1400);
        Check(frame.actions.size() == 1 && frame.actions[0] == Action::back, "B rearms after release");
        press(SDL_CONTROLLER_BUTTON_B, false);

        SDL_Event raw{};
        raw.type = SDL_JOYBUTTONDOWN;
        raw.jbutton.which = SDL_JoystickInstanceID(joystick);
        raw.jbutton.button = 0;
        Check(input.handleEvent(raw), "raw duplicate consumed");
        Check(input.update(true, 1410).actions.empty(), "raw duplicate does not execute an action");
        raw.type = SDL_CONTROLLERBUTTONDOWN;
        raw.cbutton.which = 99999;
        raw.cbutton.button = SDL_CONTROLLER_BUTTON_A;
        input.handleEvent(raw);
        Check(input.update(true, 1420).actions.empty(), "unselected controller ignored");
        raw.type = SDL_MOUSEBUTTONDOWN;
        Check(!input.handleEvent(raw), "mouse input preserved");
        raw.type = SDL_KEYDOWN;
        Check(!input.handleEvent(raw), "keyboard input preserved");

        press(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, true);
        SDL_JoystickDetachVirtual(index);
        Pump(input);
        frame = input.update(true, 1500);
        Check(frame.panX == 0 && frame.actions.empty(), "disconnect clears movement/actions");
        SDL_JoystickClose(joystick);
    }
    SDL_Quit();
    std::cout << "PASS: SDL virtual-controller input checks\n";
}
