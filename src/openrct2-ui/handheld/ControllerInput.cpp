/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ControllerInput.h"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace OpenRCT2::Ui::Handheld
{
    bool IsEnabled()
    {
        static const bool enabled = [] {
            const auto* value = std::getenv("OPENRCT2_HANDHELD");
            return value != nullptr && std::string_view(value) == "1";
        }();
        return enabled;
    }

    ControllerInput::~ControllerInput()
    {
        if (_controller != nullptr)
            SDL_GameControllerClose(_controller);
        if (_initialised)
            SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    void ControllerInput::suspend()
    {
        _armed = false;
        _actions.clear();
        _panX = _panY = 0;
        _lastTick = 0;
    }

    void ControllerInput::refresh()
    {
        if (!_initialised)
        {
            if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
                return;
            _initialised = true;
        }
        if (_controller != nullptr && !SDL_GameControllerGetAttached(_controller))
        {
            SDL_GameControllerClose(_controller);
            _controller = nullptr;
            _instance = -1;
            suspend();
            _scan = true;
        }
        if (_controller == nullptr && _scan)
        {
            _scan = false;
            for (int i = 0; i < SDL_NumJoysticks(); ++i)
            {
                if (!SDL_IsGameController(i))
                    continue;
                _controller = SDL_GameControllerOpen(i);
                if (_controller != nullptr)
                {
                    _instance = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(_controller));
                    SDL_Log("Handheld controller: %s (instance %d)", SDL_GameControllerName(_controller), _instance);
                    suspend();
                    break;
                }
            }
        }
    }

    bool ControllerInput::handleEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST || event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                    suspend();
                return false;
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                _scan = true;
                suspend();
                return true;
            case SDL_CONTROLLERBUTTONDOWN:
                if (_armed && event.cbutton.which == _instance)
                {
                    switch (event.cbutton.button)
                    {
                        case SDL_CONTROLLER_BUTTON_A:
                            _actions.push_back(Action::inspect);
                            break;
                        case SDL_CONTROLLER_BUTTON_B:
                            _actions.push_back(Action::back);
                            break;
                        case SDL_CONTROLLER_BUTTON_X:
                            _actions.push_back(Action::rotate);
                            break;
                        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                            _actions.push_back(Action::zoomOut);
                            break;
                        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                            _actions.push_back(Action::zoomIn);
                            break;
                        case SDL_CONTROLLER_BUTTON_START:
                            _actions.push_back(Action::pause);
                            break;
                        default:
                            break;
                    }
                }
                return true;
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERAXISMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYHATMOTION:
            case SDL_JOYAXISMOTION:
            case SDL_JOYBALLMOTION:
                // SDL emits both raw and mapped events. Consume both; use only mapped input.
                return true;
            default:
                return false;
        }
    }

    Frame ControllerInput::update(bool focused, uint32_t now)
    {
        refresh();
        Frame result;
        if (!focused || _controller == nullptr)
        {
            suspend();
            return result;
        }
        auto held = [this](SDL_GameControllerButton button) { return SDL_GameControllerGetButton(_controller, button) != 0; };
        if (!_armed)
        {
            bool any = false;
            for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
                any |= held(static_cast<SDL_GameControllerButton>(i));
            _armed = !any;
            _lastTick = now;
            return result;
        }
        const auto elapsed = std::min<uint32_t>(now - _lastTick, 50);
        _lastTick = now;
        result.actions = std::exchange(_actions, {});
        const int x = held(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) - held(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        const int y = held(SDL_CONTROLLER_BUTTON_DPAD_DOWN) - held(SDL_CONTROLLER_BUTTON_DPAD_UP);
        const float speed = 120.0f * (x != 0 && y != 0 ? 0.70710678f : 1.0f);
        _panX = x == 0 ? 0 : _panX + x * speed * elapsed / 1000.0f;
        _panY = y == 0 ? 0 : _panY + y * speed * elapsed / 1000.0f;
        result.panX = static_cast<int32_t>(_panX);
        result.panY = static_cast<int32_t>(_panY);
        _panX -= result.panX;
        _panY -= result.panY;
        return result;
    }
} // namespace OpenRCT2::Ui::Handheld
