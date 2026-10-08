// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/ride/Ride.h>

namespace OpenRCT2::Ui::Handheld::RideManagement
{
    struct Target
    {
        RideId id;
        ride_type_t type;
        ObjectEntryIndex object;
        int32_t buildDate;
        CoordsXY location;
        uint64_t generation;
        bool operator==(const Target&) const = default;
    };
    enum class View
    {
        roster,
        inspection,
        statistics,
        price,
        confirmation
    };
    struct Draft
    {
        Target target;
        RideStatus baselineStatus;
        money64 baselinePrice;
        bool pricesUnlocked;
        bool noMoney;
        std::optional<RideStatus> status;
        money64 price;
    };
    void Invalidate(RideId id);
    std::optional<Target> Identify(RideId id);
    const Ride* Resolve(const Target& target);
    std::vector<Target> Roster();
    std::optional<CoordsXYZ> Location(const Target& target);
    bool Locate(const Target& target);
    bool PricePermitted(const Ride& ride);
    std::optional<Draft> Begin(const Target& target, std::optional<RideStatus> status = {});
    void AdjustPrice(Draft& draft, int direction);
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback = {});
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster = {}, const std::optional<Target>& target = {},
        const std::optional<Draft>& draft = {}, const std::string& feedback = {});
} // namespace OpenRCT2::Ui::Handheld::RideManagement
