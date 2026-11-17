// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "GuestManagement.h"
#include "RideManagement.h"
#include "StaffManagement.h"

#include <variant>
namespace OpenRCT2::Ui::Handheld::MapSubjects
{
    using Target = std::variant<RideManagement::Target, GuestManagement::Target, StaffManagement::Target>;
    inline bool Current(const Target& target)
    {
        return std::visit(
            [](const auto& t) {
                using T = std::decay_t<decltype(t)>;
                if constexpr (std::is_same_v<T, RideManagement::Target>)
                    return RideManagement::Resolve(t) != nullptr;
                else if constexpr (std::is_same_v<T, GuestManagement::Target>)
                    return GuestManagement::Resolve(t) != nullptr;
                else
                    return StaffManagement::Resolve(t) != nullptr;
            },
            target);
    }
    inline std::string Name(const Target& target)
    {
        return std::visit(
            [](const auto& t) -> std::string {
                using T = std::decay_t<decltype(t)>;
                if constexpr (std::is_same_v<T, RideManagement::Target>)
                {
                    const auto* p = RideManagement::Resolve(t);
                    return p ? p->getName() : "Ride unavailable";
                }
                else if constexpr (std::is_same_v<T, GuestManagement::Target>)
                {
                    const auto* p = GuestManagement::Resolve(t);
                    return p ? p->getName() : "Guest unavailable";
                }
                else
                {
                    const auto* p = StaffManagement::Resolve(t);
                    return p ? p->getName() : "Employee unavailable";
                }
            },
            target);
    }
    inline std::optional<CoordsXYZ> Location(const Target& target)
    {
        return std::visit(
            [](const auto& t) -> std::optional<CoordsXYZ> {
                using T = std::decay_t<decltype(t)>;
                if constexpr (std::is_same_v<T, RideManagement::Target>)
                    return RideManagement::Location(t);
                else if constexpr (std::is_same_v<T, GuestManagement::Target>)
                    return GuestManagement::Location(t);
                else
                {
                    const auto* p = StaffManagement::Resolve(t);
                    return p && !p->getLocation().isNull() ? std::optional<CoordsXYZ>{ p->getLocation() } : std::nullopt;
                }
            },
            target);
    }
    inline std::vector<Target> Roster(int mode, int filter, const std::optional<GuestManagement::Filter>& guestFilter = {})
    {
        std::vector<Target> result;
        if (mode == 2)
        {
            auto options = guestFilter.value_or(GuestManagement::Filter{});
            options.tracked = options.tracked || filter != 0;
            for (const auto& target : GuestManagement::Roster(options))
                result.emplace_back(target);
        }
        else if (mode == 3)
        {
            for (const auto& target : StaffManagement::Roster(
                     filter ? std::optional<StaffType>{ static_cast<StaffType>(filter - 1) } : std::nullopt))
                result.emplace_back(target);
        }
        else
            for (const auto& target : RideManagement::Roster())
                result.emplace_back(target);
        return result;
    }
    inline std::string FilterName(int mode, int filter, const std::optional<GuestManagement::Filter>& guestFilter = {})
    {
        if (mode == 2 && guestFilter)
            return GuestManagement::FilterName(*guestFilter) + (filter ? " / Tracked" : "");
        return mode == 2 ? (filter ? "Tracked guests" : "All guests")
            : mode == 3  ? (filter ? StaffManagement::RoleName(static_cast<StaffType>(filter - 1)) : "All staff roles")
                         : "All rides";
    }
    inline ManagementPresentation Presentation(
        const std::vector<Target>& roster, int mode, int filter, size_t selected,
        const std::optional<GuestManagement::Filter>& guestFilter = {})
    {
        ManagementPresentation model;
        model.breadcrumb = "Park map / Native subjects";
        model.title = FilterName(mode, filter, guestFilter);
        model.selected = selected;
        model.stats = { { "Native members", std::to_string(roster.size()) } };
        for (const auto& target : roster)
            model.rows.push_back(Name(target));
        if (mode == 2 || mode == 3)
            model.rows.push_back(mode == 2 && guestFilter ? "Toggle tracked guests" : "Change native filter");
        model.rows.push_back("Return to map");
        model.status = roster.empty() ? "No eligible subjects in this filter" : "";
        return model;
    }
    inline ManagementPresentation Detail(const Target& target, size_t selected, const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park map / Selected subject";
        model.title = Name(target);
        model.selected = selected;
        const auto location = Location(target);
        model.rows = { location ? "Show on map" : "Location unavailable",
                       Current(target) ? "Inspect subject" : "Subject unavailable",
                       location ? "Locate / return to park" : "Location unavailable", "Back to subjects" };
        if (location)
            model.stats = { { "Native location", std::to_string(location->x / 32) + ", " + std::to_string(location->y / 32) } };
        model.status = status.empty() ? "Identity and native location rechecked on selection" : status;
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::MapSubjects
