// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"

#include <functional>
#include <openrct2/actions/GameActionResult.h>
#include <openrct2/world/Location.hpp>
#include <optional>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld::AutoAccess
{
    // Y while placing an entrance or exit: put both beside the station and lay a level queue and
    // exit path to the nearest footpath.
    //
    // Each free side of the station is a candidate. From the tile just outside it, a breadth-first
    // search over level, buildable tiles finds the shortest walk to an existing path at the same
    // height. The entrance takes the side with the shortest walk; the exit takes the next best side
    // whose walk does not cross the queue. If no level walk exists (an elevated station, a river in
    // the way), the entrance and exit are still placed and the plan says the paths need building.
    struct Plan
    {
        CoordsXYZD entrance, exit;
        bool placeEntrance{}, placeExit{};
        std::vector<CoordsXYZ> queue, path; // from beside the ride outwards
        bool queueConnects{}, pathConnects{};
        money64 cost{};
        // Every tile the plan touches, for undo.
        std::vector<CoordsXY> Tiles() const;
    };
    constexpr int kMaxWalk = 24;

    std::optional<Plan> Make(const RideManagement::Target& target, StationIndex station, std::string& error);
    // Queues every action; done runs once the last of them has been processed.
    GameActions::Result Execute(
        const Plan& plan, const RideManagement::Target& target, StationIndex station, std::function<void()> done = {});
    // One line for the status bar: what was built and what is left to do.
    std::string Summary(const Plan& plan);
} // namespace OpenRCT2::Ui::Handheld::AutoAccess
