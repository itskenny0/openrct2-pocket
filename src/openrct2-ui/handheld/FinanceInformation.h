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
        Date period;
        period.monthsElapsed = month;
        return ParkInformation::PlainText(FormatStringIDLegacy(STR_FINANCES_SUMMARY_MONTH_HEADING, ft.Data()), true)
            + " / Year " + std::to_string(period.GetYear() + 1);
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
        model.breadcrumb = "Finances / Ledger";
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
    inline const money64* Series(size_t graph)
    {
        const auto& park = getGameState().park;
        if (graph == 0)
            return park.cashHistory; // Native cash less loan, not raw cash.
        if (graph == 1)
            return park.weeklyProfitHistory;
        return park.valueHistory;
    }
    inline money64 Current(size_t graph)
    {
        const auto& park = getGameState().park;
        if (graph == 0)
            return park.cash == kMoney64Undefined || park.bankLoan == kMoney64Undefined ? kMoney64Undefined
                                                                                        : park.cash - park.bankLoan;
        return graph == 1 ? park.currentProfit : park.value;
    }
    inline ManagementPresentation History(size_t graph, size_t sample, size_t selected)
    {
        static constexpr const char* names[] = { "Cash less loan history", "Weekly profit history", "Park value history" };
        graph %= 3;
        sample = std::min<size_t>(63, sample);
        ManagementPresentation model;
        model.breadcrumb = "Finances / History";
        model.title = names[graph];
        model.selected = selected;
        model.stats = { { "Current", Money(Current(graph)) },
                        { "Sample " + std::to_string(sample) + " weeks back", Money(Series(graph)[sample]) } };
        model.rows = { "Next financial graph", "Older sample", "Newer sample", "Back to Finances" };
        const bool hasData = std::any_of(Series(graph), Series(graph) + 64, [](auto v) { return v != kMoney64Undefined; });
        model.status = getGameState().park.flags.has(ParkFlag::noMoney) ? "No-money park; financial values not applicable"
            : hasData                                                   ? "Left/Right selects week"
                                                                        : "Waiting for weekly history";
        return model;
    }
    inline void DrawHistory(
        Drawing::RenderTarget& rt, size_t graph, ScreenRect bounds = { { 138, 150 }, { 476, 234 } }, int32_t sample = -1)
    {
        if (getGameState().park.flags.has(ParkFlag::noMoney))
            return;
        graph %= 3;
        Graph::GraphProperties<money64> props{};
        props.series = Series(graph);
        props.hoverIdx = -1; // Controller-selected value is displayed above the native plot.
        // Same magnitude/leading-digit range as desktop Finances, within ordinary native monetary bounds.
        money64 magnitude = 0;
        bool negative = false, positive = false;
        for (size_t i = 0; i < 64; ++i)
            if (props.series[i] != kMoney64Undefined)
            {
                magnitude = std::max(magnitude, std::abs(props.series[i]));
                negative |= props.series[i] < 0;
                positive |= props.series[i] > 0;
            }
        money64 oom = 10;
        while (magnitude / oom >= 10)
            oom *= 10;
        const auto limit = std::max(10.00_GBP, ((magnitude + oom - 1) / oom) * oom);
        props.min = graph == 2 || !negative ? 0 : -limit;
        props.max = positive || !negative ? limit : 0;
        props.darkBackground = PocketPalette::style == PocketPalette::Style::slate;
        props.breakOnMissing = true;
        props.lineCol = ColourWithFlags{ Drawing::Colour::grey };
        props.RecalculateLayout(bounds, 5, 64);
        Graph::DrawFinanceGraph(rt, props);
        if (sample >= 0 && sample < 64 && props.series[sample] != kMoney64Undefined)
        {
            const int x = props.internalBounds.getRight() - sample * props.xStepPx;
            GfxDrawDashedLine(
                rt, { { x, props.internalBounds.getTop() }, { x, props.internalBounds.getBottom() } }, 3, Theme::gold);
        }
    }
} // namespace OpenRCT2::Ui::Handheld::FinanceInformation
