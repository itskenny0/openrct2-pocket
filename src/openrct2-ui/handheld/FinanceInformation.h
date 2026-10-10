// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"
#include "ParkInformation.h"

#include <openrct2/Date.h>
#include <openrct2/GameState.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/management/Finance.h>
namespace OpenRCT2::Ui::Handheld::FinanceInformation
{
    inline constexpr StringId kCategories[] = { STR_FINANCES_SUMMARY_RIDE_CONSTRUCTION,
                                                STR_FINANCES_SUMMARY_RIDE_RUNNING_COSTS,
                                                STR_FINANCES_SUMMARY_LAND_PURCHASE,
                                                STR_FINANCES_SUMMARY_LANDSCAPING,
                                                STR_FINANCES_SUMMARY_PARK_ENTRANCE_TICKETS,
                                                STR_FINANCES_SUMMARY_RIDE_TICKETS,
                                                STR_FINANCES_SUMMARY_SHOP_SALES,
                                                STR_FINANCES_SUMMARY_SHOP_STOCK,
                                                STR_FINANCES_SUMMARY_FOOD_DRINK_SALES,
                                                STR_FINANCES_SUMMARY_FOOD_DRINK_STOCK,
                                                STR_FINANCES_SUMMARY_STAFF_WAGES,
                                                STR_FINANCES_SUMMARY_MARKETING,
                                                STR_FINANCES_SUMMARY_RESEARCH,
                                                STR_FINANCES_SUMMARY_LOAN_INTEREST };
    static_assert(std::size(kCategories) == EnumValue(ExpenditureType::count));
    inline std::string Money(money64 value)
    {
        return getGameState().park.flags.has(ParkFlag::noMoney) ? "Not applicable" : FormatMoney(value);
    }
    inline std::string Period(uint32_t month)
    {
        auto ft = Formatter();
        ft.Add<uint16_t>(month);
        return ParkInformation::PlainText(FormatStringIDLegacy(STR_FINANCES_SUMMARY_MONTH_HEADING, ft.Data()), true);
    }
    struct Ledger
    {
        uint32_t month{}; // Absolute native calendar period; survives a monthly table shift.
        void Begin()
        {
            month = GetDate().GetMonthsElapsed();
        }
        void Clamp()
        {
            const uint32_t now = GetDate().GetMonthsElapsed();
            month = std::clamp(month, now > 15 ? now - 15 : 0u, now);
        }
        void Move(int direction)
        {
            Clamp();
            if (direction < 0 && month)
                --month;
            else if (direction > 0 && month < GetDate().GetMonthsElapsed())
                ++month;
            Clamp();
        }
        size_t Age() const
        {
            return std::min<uint32_t>(
                15, GetDate().GetMonthsElapsed() - std::min<uint32_t>(month, GetDate().GetMonthsElapsed()));
        }
        money64 Value(size_t category) const
        {
            return category < std::size(kCategories) ? getGameState().park.expenditureTable[Age()][category]
                                                     : kMoney64Undefined;
        }
        money64 Net() const
        {
            money64 result = 0;
            for (size_t i = 0; i < std::size(kCategories); ++i)
            {
                const auto value = Value(i);
                if (value == kMoney64Undefined)
                    return kMoney64Undefined;
                result += value;
            }
            return result;
        }
    };
    inline ManagementPresentation LedgerPresentation(Ledger ledger, size_t selected)
    {
        ledger.Clamp();
        ManagementPresentation model;
        model.breadcrumb = "Finances / Native expenditure ledger";
        model.title = Period(ledger.month);
        model.selected = selected;
        for (auto id : kCategories)
            model.rows.push_back(ParkInformation::PlainText(FormatStringIDLegacy(id, nullptr), true));
        model.stats = { { "Period net", Money(ledger.Net()) } };
        if (selected < std::size(kCategories))
            model.stats.push_back({ "Selected entry", Money(ledger.Value(selected)) });
        else
            model.stats.push_back(
                { "Retained months", std::to_string(std::min<uint32_t>(16, GetDate().GetMonthsElapsed() + 1)) });
        model.rows.insert(model.rows.end(), { "Older month", "Newer month", "Back to Finances" });
        model.status = getGameState().park.flags.has(ParkFlag::noMoney) ? "No-money park; accounting not applicable"
                                                                        : "Signed net categories; Left/Right changes month";
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::FinanceInformation
