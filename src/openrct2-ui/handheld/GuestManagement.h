// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/entity/Guest.h>

namespace OpenRCT2::Ui::Handheld::GuestManagement
{
    struct Target
    {
        EntityId id;
        uint32_t peepId;
        int32_t entryTime;
        uint64_t generation;
        bool operator==(const Target&) const = default;
    };
    enum class View
    {
        overview,
        roster,
        inspection,
        needs,
        thoughts,
        spending,
        inventory,
        text,
        preferences
    };
    GameActions::Result SetName(const Target&, const std::string& baseline, const std::string& value, bool execute);
    GameActions::Result SetTracking(const Target&, PeepFlags baseline, bool value, bool execute);
    ManagementPresentation TrackingPresentation(const Target&, bool value, size_t selected);
    void Invalidate(EntityId id);
    std::optional<Target> Identify(EntityId id);
    const Guest* Resolve(const Target& target);
    enum class GroupMode
    {
        actions,
        thoughts
    };
    struct Group
    {
        GroupMode mode{};
        std::array<uint8_t, 12> arguments{};
        std::string label;
        size_t count{};
        bool operator==(const Group&) const = default;
    };
    struct Filter
    {
        std::optional<Group> group;
        bool tracked{};
    };
    std::vector<Target> Roster(const Filter& filter = {});
    std::vector<Group> Groups(GroupMode mode);
    ManagementPresentation GroupPresentation(GroupMode, const std::vector<Group>&, size_t selected);
    std::string Status(const Guest& guest);
    std::vector<std::string> Thoughts(const Guest& guest);
    std::vector<std::string> Inventory(const Guest& guest);
    std::vector<std::string> TextLines(const std::string& text);
    std::optional<CoordsXYZ> Location(const Target& target);
    bool Locate(const Target& target);
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster = {}, const std::optional<Target>& target = {},
        const std::string& status = {}, const std::string& text = {}, size_t textPage = 0);
} // namespace OpenRCT2::Ui::Handheld::GuestManagement
