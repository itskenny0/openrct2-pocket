// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PeopleTabs.h"
namespace OpenRCT2::Ui::Handheld::PeopleNavigation
{
    struct StaffContext { StaffManagement::Target target; PeopleTabs::State tabs; size_t rosterFocus{}; std::optional<StaffType> filter; bool fromPark{}; };
    struct GuestContext { GuestManagement::Target target; PeopleTabs::State tabs; size_t rosterFocus{}; GuestManagement::Filter filter; bool fromPark{}; int rosterParent{}; std::optional<RideManagement::Target> rideParent; };
    inline std::optional<StaffContext> staff;
    inline std::optional<GuestContext> guest;
}
