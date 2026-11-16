// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/ride/Ride.h>

namespace OpenRCT2::Ui::Handheld::MarketingManagement
{
    enum class View
    {
        overview,
        active,
        types,
        editor,
        targets,
        confirmation
    };
    struct Target
    {
        int item{};
        bool ride{};
        ride_type_t type{ kRideTypeNull };
        ObjectEntryIndex object{ kObjectEntryIndexNull };
        int32_t buildDate{};
        CoordsXY location{};
        uint64_t generation{};
        bool operator==(const Target&) const = default;
    };
    struct Draft
    {
        int type{};
        int weeks{ 2 };
        std::optional<Target> target;
    };
    bool NeedsTarget(int type);
    bool Eligible(int type);
    std::string Name(int type);
    std::string TargetName(const Target& target);
    std::vector<int> Types();
    std::vector<int> ActiveTypes();
    std::vector<Target> Targets(int type);
    void RideRemoved(RideId id);
    bool SelectTarget(Draft& draft, const Target& target);
    void AdjustWeeks(Draft& draft, int direction);
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback = {});
    ManagementPresentation Presentation(
        View view, size_t selected, const std::optional<Draft>& draft = {}, int activeType = -1,
        const std::vector<int>& types = {}, const std::vector<Target>& targets = {}, const std::string& status = {});
} // namespace OpenRCT2::Ui::Handheld::MarketingManagement
