// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/GameState.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/interface/Window.h>
#include <openrct2/interface/WindowBase.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/Vehicle.h>
#include <optional>
#include <string>

namespace OpenRCT2::Ui::Handheld::RideFollow
{
    // Ride along: the park camera follows one train of a ride, through the same native follow
    // that the guest follow uses. A cycles trains, B returns to the ride, panning ends it.
    //
    // The camera is zoomed fully in while riding, because at the usual overview zoom a train is a
    // few pixels long; the previous zoom comes back when the ride ends.
    struct State
    {
        RideId ride;
        size_t train{};
        EntityId vehicle;
        ZoomLevel zoom{};
    };
    inline std::optional<State> current;

    inline EntityId Train(const Ride& ride, size_t index)
    {
        if (index >= ride.numTrains)
            return EntityId::GetNull();
        const auto id = ride.vehicles[index];
        return getGameState().entities.getEntity<Vehicle>(id) ? id : EntityId::GetNull();
    }
    inline size_t Trains(const Ride& ride)
    {
        size_t count = 0;
        for (size_t i = 0; i < ride.numTrains; ++i)
            if (!Train(ride, i).IsNull())
                ++count;
        return count;
    }
    // A ride can be ridden along once its trains exist, which is from its first test onwards.
    inline bool Available(RideId id)
    {
        const auto* ride = GetRide(id);
        return ride && Trains(*ride) > 0;
    }
    inline void Stop()
    {
        if (!current)
            return;
        if (auto* main = WindowGetMain(); main && main->viewport)
        {
            if (main->viewportSmartFollowSprite == current->vehicle)
                WindowUnfollowSprite(*main);
            if (main->viewport->zoom != current->zoom)
                WindowZoomSet(*main, current->zoom, false);
        }
        current.reset();
    }
    inline void Update()
    {
        if (!current)
            return;
        const auto* ride = GetRide(current->ride);
        const auto* main = WindowGetMain();
        if (gLegacyScene != LegacyScene::playing || !ride || !main || !main->viewport
            || Train(*ride, current->train) != current->vehicle)
            Stop();
        else if (main->viewportSmartFollowSprite != current->vehicle)
        {
            // Something else took the camera (a pan, a locate): give the zoom back and step aside.
            auto zoom = current->zoom;
            current.reset();
            if (auto* window = WindowGetMain(); window && window->viewport && window->viewport->zoom != zoom)
                WindowZoomSet(*window, zoom, false);
        }
    }
    inline std::optional<State> Current()
    {
        Update();
        return current;
    }
    inline bool Start(RideId id, size_t train = 0)
    {
        const auto* ride = GetRide(id);
        auto* main = WindowGetMain();
        if (gLegacyScene != LegacyScene::playing || !ride || !main || !main->viewport)
            return false;
        const size_t count = ride->numTrains;
        // The requested train, or the next one that exists.
        for (size_t step = 0; step < count; ++step)
        {
            const size_t index = (train + step) % count;
            const auto vehicle = Train(*ride, index);
            if (vehicle.IsNull())
                continue;
            const auto zoom = current ? current->zoom : main->viewport->zoom;
            if (current && main->viewportSmartFollowSprite == current->vehicle)
                WindowUnfollowSprite(*main);
            current = State{ id, index, vehicle, zoom };
            if (main->viewport->zoom != ZoomLevel{ 0 })
                WindowZoomSet(*main, ZoomLevel{ 0 }, false);
            WindowFollowSprite(*main, vehicle);
            return true;
        }
        return false;
    }
    inline bool Next()
    {
        return current && Start(current->ride, current->train + 1);
    }
    inline std::string Label()
    {
        if (!current)
            return {};
        const auto* ride = GetRide(current->ride);
        if (!ride)
            return {};
        const auto count = Trains(*ride);
        std::string text = "Riding " + ride->getName();
        if (count > 1)
            text += " (train " + std::to_string(current->train + 1) + "/" + std::to_string(ride->numTrains) + ")";
        return text;
    }
} // namespace OpenRCT2::Ui::Handheld::RideFollow
