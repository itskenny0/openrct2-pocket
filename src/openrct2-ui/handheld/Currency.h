// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cctype>
#include <openrct2-ui/UiStringIds.h>
#include <openrct2/core/Money.hpp>
#include <openrct2/localisation/Formatting.h>

namespace OpenRCT2::Ui::Handheld
{
    // Native money is tenths of GBP. The engine owns currency conversion and symbols.
    inline std::string FormatMoney(money64 value)
    {
        if (value == kMoney64Undefined)
            return "Unavailable";
        // Whole amounts drop their zero pence ("£12,000", not "£12,000.00"): on a small screen the
        // extra characters are noise. Prices with pence keep them. Works for "." and "," decimals.
        auto text = FormatStringID(STR_CURRENCY2DP, value);
        for (const char* zero : { ".00", ",00" })
            for (auto at = text.find(zero); at != std::string::npos; at = text.find(zero, at + 1))
                if (at + 3 == text.size() || !std::isdigit(static_cast<unsigned char>(text[at + 3])))
                {
                    text.erase(at, 3);
                    break;
                }
        return text;
    }
} // namespace OpenRCT2::Ui::Handheld
