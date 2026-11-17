// SPDX-License-Identifier: GPL-3.0-only
#include "Alerts.h"

#include "MenuFeel.h"

#include <algorithm>
#include <array>
#include <openrct2/GameState.h>
#include <openrct2/entity/EntityList.h>
#include <openrct2/entity/Guest.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/world/Map.h>

namespace OpenRCT2::Ui::Handheld::Alerts
{
    namespace
    {
        std::vector<Alert> cache;
        uint64_t refreshed{};
        bool valid{};
        constexpr uint64_t kRefreshMs = 2000;

        CoordsXYZ RideLocation(const Ride& ride)
        {
            for (const auto& station : ride.getStations())
                if (!station.start.isNull())
                    return station.getStart();
            return { ride.overallView, TileElementHeight(ride.overallView) };
        }
        CoordsXYZ QueueLocation(const Ride& ride)
        {
            for (const auto& station : ride.getStations())
                if (!station.entrance.isNull())
                    return station.entrance.toCoordsXYZ();
            return RideLocation(ride);
        }

        struct ThoughtGroup
        {
            Kind kind;
            const char* title;
            const char* fix;
            int count{};
            CoordsXYZ where{};
        };

        std::vector<Alert> Collect()
        {
            std::vector<Alert> alerts;
            const auto& state = getGameState();
            for (const auto& ride : RideManager(state))
            {
                if (ride.getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility))
                    continue;
                const auto name = ride.getName();
                if (ride.flags.has(RideFlag::crashed))
                    alerts.push_back(
                        { Kind::crash, name + " has crashed", "Close it, then reopen to repair", RideLocation(ride),
                          ride.id.ToUnderlying() });
                else if (ride.flags.has(RideFlag::brokenDown))
                    alerts.push_back(
                        { Kind::breakdown, name + " broke down", "Hire mechanics to fix rides faster", RideLocation(ride),
                          ride.id.ToUnderlying() });
                else if (ride.flags.has(RideFlag::queueFull))
                    alerts.push_back(
                        { Kind::queueFull, name + " has a full queue", "Raise its price or build another ride",
                          QueueLocation(ride), ride.id.ToUnderlying() });
            }

            std::array groups{
                ThoughtGroup{ Kind::hungry, "guests are hungry", "Build a food stall nearby" },
                ThoughtGroup{ Kind::thirsty, "guests are thirsty", "Build a drinks stall nearby" },
                ThoughtGroup{ Kind::toilet, "guests need a toilet", "Build toilets nearby" },
                ThoughtGroup{ Kind::dirty, "guests say paths are dirty", "Hire a handyman" },
                ThoughtGroup{ Kind::vandalism, "guests saw vandalism", "Hire security" },
                ThoughtGroup{ Kind::lost, "guests are lost", "Simplify paths near here" },
            };
            int guests{};
            for (const auto* guest : EntityList<Guest>())
            {
                if (guest->outsideOfPark)
                    continue;
                ++guests;
                for (const auto& thought : guest->thoughts)
                {
                    if (thought.type == PeepThoughtType::none)
                        break;
                    // The same freshness window the guest window uses for "what they think now".
                    if (thought.freshness > 5)
                        continue;
                    ThoughtGroup* group = nullptr;
                    switch (thought.type)
                    {
                        case PeepThoughtType::hungry:
                            group = &groups[0];
                            break;
                        case PeepThoughtType::thirsty:
                            group = &groups[1];
                            break;
                        case PeepThoughtType::toilet:
                            group = &groups[2];
                            break;
                        case PeepThoughtType::badLitter:
                        case PeepThoughtType::pathDisgusting:
                            group = &groups[3];
                            break;
                        case PeepThoughtType::vandalism:
                            group = &groups[4];
                            break;
                        case PeepThoughtType::lost:
                        case PeepThoughtType::cantFindExit:
                            group = &groups[5];
                            break;
                        default:
                            break;
                    }
                    if (group)
                    {
                        ++group->count;
                        group->where = guest->getLocation();
                        break;
                    }
                }
            }
            // A handful of grumbles is normal in any park; only raise it when it is a pattern.
            const int threshold = std::max(4, guests / 25);
            for (const auto& group : groups)
                if (group.count >= threshold)
                    alerts.push_back(
                        { group.kind, std::to_string(group.count) + " " + group.title, group.fix, group.where, std::nullopt });
            std::stable_sort(alerts.begin(), alerts.end(), [](const auto& a, const auto& b) { return a.kind < b.kind; });
            return alerts;
        }
    } // namespace

    const std::vector<Alert>& Current()
    {
        const auto now = MenuFeel::Clock();
        if (!valid || now - refreshed >= kRefreshMs)
        {
            cache = Collect();
            refreshed = now;
            valid = true;
        }
        return cache;
    }
    void Invalidate()
    {
        valid = false;
    }
    std::string HintLabel()
    {
        const auto count = Current().size();
        return count == 0 ? "News" : "Alerts (" + std::to_string(count) + ")";
    }
} // namespace OpenRCT2::Ui::Handheld::Alerts
