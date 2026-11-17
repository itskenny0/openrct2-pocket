// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"
#include "RideTabs.h"

#include <optional>
namespace OpenRCT2::Ui::Handheld::RideNavigation
{
    // Transient navigation only; native identity is revalidated before every return.
    struct Context
    {
        RideManagement::Target target;
        RideTabs::State tabs;
        bool fromPark = true;
        size_t rosterFocus{};
    };
    inline std::optional<Context> construction;
    inline std::optional<Context> Take()
    {
        auto context = construction;
        construction.reset();
        return context;
    }
} // namespace OpenRCT2::Ui::Handheld::RideNavigation
