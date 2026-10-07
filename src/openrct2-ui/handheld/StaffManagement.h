// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/entity/Staff.h>

namespace OpenRCT2::Ui::Handheld::StaffManagement
{
    struct Target
    {
        EntityId id;
        StaffType type;
        uint32_t peepId;
        int32_t hireDate;
        uint64_t generation;
        bool operator==(const Target&) const = default;
    };
    enum class View
    {
        roster,
        inspection,
        statistics,
        orders,
        hire,
        dismissal
    };
    void Invalidate(EntityId id);
    bool Supported(StaffType type);
    std::vector<Target> Roster();
    std::optional<Target> Identify(EntityId id);
    Staff* Resolve(const Target& target);
    std::vector<std::pair<uint8_t, std::string>> Orders(StaffType type);
    GameActions::Result Hire(StaffType type, bool execute, GameActions::GameAction::Callback_t callback = {});
    GameActions::Result SetOrders(const Target& target, uint8_t baseline, uint8_t value, bool execute);
    GameActions::Result Dismiss(const Target& target, bool execute);
    bool Locate(const Target& target);
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster = {}, const std::optional<Target>& target = {},
        StaffType hireType = StaffType::handyman, std::optional<uint8_t> orders = {}, const std::string& status = {});
} // namespace OpenRCT2::Ui::Handheld::StaffManagement
