/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <SDL_gamecontroller.h>
#include <cstdint>
#include <vector>

union SDL_Event;

namespace OpenRCT2::Ui::Handheld
{
    bool IsEnabled();

    enum class Action
    {
        inspect,
        back,
        rotate,
        zoomOut,
        zoomIn,
        pause,
    };

    struct Frame
    {
        int32_t panX{};
        int32_t panY{};
        std::vector<Action> actions;
    };

    // Owns one SDL mapped controller. No game state, mouse emulation, or widget indices.
    class ControllerInput
    {
    public:
        ControllerInput() = default;
        ~ControllerInput();
        ControllerInput(const ControllerInput&) = delete;
        ControllerInput& operator=(const ControllerInput&) = delete;

        bool handleEvent(const SDL_Event& event);
        Frame update(bool focused, uint32_t now);
        void suspend();

    private:
        SDL_GameController* _controller{};
        SDL_JoystickID _instance{ -1 };
        bool _initialised{};
        bool _scan{ true };
        bool _armed{};
        uint32_t _lastTick{};
        float _panX{};
        float _panY{};
        std::vector<Action> _actions;

        void refresh();
    };
} // namespace OpenRCT2::Ui::Handheld
