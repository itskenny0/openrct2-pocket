// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"
#include "TextEntry.h"

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
        dismissal,
        roles,
        costumes,
        summary
    };
    void Invalidate(EntityId id);
    bool Supported(StaffType type);
    std::string RoleName(StaffType type);
    std::vector<std::pair<ObjectEntryIndex, std::string>> Costumes();
    GameActions::Result SetCostume(const Target& target, ObjectEntryIndex baseline, ObjectEntryIndex value, bool execute);
    GameActions::Result SetUniform(const Target&, Drawing::Colour baseline, Drawing::Colour value, bool execute);
    ManagementPresentation UniformPresentation(const Target&, Drawing::Colour value, size_t selected);
    std::vector<Target> Roster(std::optional<StaffType> filter = {});
    GameActions::Result SetName(const Target&, const std::string& baseline, const std::string& value, bool execute);
    ManagementPresentation NamePresentation(const TextEntry&, size_t selected, bool review, const std::string& status = {});
    std::optional<Target> Identify(EntityId id);
    Staff* Resolve(const Target& target);
    std::vector<std::pair<uint8_t, std::string>> Orders(StaffType type);
    GameActions::Result Hire(
        StaffType type, bool execute, GameActions::GameAction::Callback_t callback = {}, bool manual = false);
    GameActions::Result SetOrders(
        const Target& target, uint8_t baseline, uint8_t value, bool execute, GameActions::GameAction::Callback_t callback = {});
    GameActions::Result Dismiss(const Target& target, bool execute);
    bool Locate(const Target& target);
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster = {}, const std::optional<Target>& target = {},
        StaffType hireType = StaffType::handyman, std::optional<uint8_t> orders = {}, const std::string& status = {});
} // namespace OpenRCT2::Ui::Handheld::StaffManagement
