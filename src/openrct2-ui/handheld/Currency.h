// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2-ui/UiStringIds.h>
#include <openrct2/core/Money.hpp>
#include <openrct2/localisation/Formatting.h>

namespace OpenRCT2::Ui::Handheld
{
    // Native money is tenths of GBP. The engine owns currency conversion and symbols.
    inline std::string FormatMoney(money64 value)
    {
        return value == kMoney64Undefined ? "Unavailable" : FormatStringID(STR_CURRENCY2DP, value);
    }
} // namespace OpenRCT2::Ui::Handheld
