// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"

#include <openrct2/GameState.h>
#include <openrct2/core/BitSet.hpp>
#include <openrct2/core/UnitConversion.h>
#include <openrct2/management/Finance.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/scenario/Scenario.h>

#include <algorithm>
#include <optional>
#include <string>

namespace OpenRCT2::Ui::Handheld::Goal
{
    // The scenario goal as one line of progress for the HUD: what counts, how far along it is,
    // and the one other thing that decides it (time left, or the rating it also needs).
    //
    // The counts repeat the rules of Scenario::Objective's checks (which only answer yes or no),
    // so the bar fills by the same measure the game will judge.
    struct Progress
    {
        std::string text;   // "1,328 / 1,500 guests"
        std::string aside;  // "8 mo left", "Rating 540"
        int percent{};      // 0..100
        bool asideWarns{};  // The aside is what is holding the goal back.
        bool done{}, failed{};
    };

    inline std::string Thousands(int64_t value)
    {
        auto digits = std::to_string(value < 0 ? -value : value);
        for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
            digits.insert(static_cast<size_t>(i), ",");
        return (value < 0 ? "-" : "") + digits;
    }
    inline int Percent(int64_t have, int64_t need)
    {
        if (need <= 0)
            return 100;
        return static_cast<int>(std::clamp<int64_t>(have * 100 / need, 0, 100));
    }

    // Distinct roller coaster designs that are open and at least this exciting (and long).
    inline int Coasters(RideRating_t excitement, int minimumLength = 0)
    {
        auto& state = getGameState();
        BitSet<kMaxRideObjects> counted;
        int count = 0;
        for (const auto& ride : RideManager(state))
        {
            if (ride.status != RideStatus::open || ride.ratings.excitement < excitement || ride.subtype == kObjectEntryIndexNull
                || counted[ride.subtype])
                continue;
            const auto* entry = ride.getRideEntry();
            if (!entry || !RideEntryHasCategory(*entry, RideCategory::rollerCoaster))
                continue;
            if (minimumLength > 0 && ToHumanReadableRideLength(ride.getTotalLength()) < minimumLength)
                continue;
            counted[ride.subtype] = true;
            ++count;
        }
        return count;
    }
    inline int FinishedCoasters(RideRating_t excitement)
    {
        int count = 0;
        for (const auto& ride : RideManager(getGameState()))
        {
            const auto* entry = ride.getRideEntry();
            if (ride.status != RideStatus::closed && ride.ratings.excitement >= excitement && entry
                && ride.flags.has(RideFlag::indestructibleTrack) && RideEntryHasCategory(*entry, RideCategory::rollerCoaster))
                ++count;
        }
        return count;
    }

    inline std::optional<Progress> Current()
    {
        const auto& state = getGameState();
        const auto& park = state.park;
        const auto& objective = state.scenarioOptions.objective;
        Progress p;
        if (state.scenarioCompletedCompanyValue == kCompanyValueOnFailedObjective)
        {
            p.text = "Goal not met";
            p.failed = true;
            return p;
        }
        if (state.scenarioCompletedCompanyValue != kMoney64Undefined)
        {
            p.text = "Goal complete";
            p.percent = 100;
            p.done = true;
            return p;
        }
        // Goals judged at the end of October in a given year.
        const int monthsLeft = objective.Year * MONTH_COUNT - GetDate().GetMonthsElapsed();
        const auto deadline = monthsLeft <= 1 ? std::string("Last month") : std::to_string(monthsLeft) + " mo left";
        switch (objective.Type)
        {
            case Scenario::ObjectiveType::guestsBy:
                p.text = Thousands(park.numGuestsInPark) + " / " + Thousands(objective.NumGuests) + " guests";
                p.percent = Percent(park.numGuestsInPark, objective.NumGuests);
                // The goal also needs a rating of 600 when it is judged.
                p.asideWarns = park.rating < 600;
                p.aside = p.asideWarns ? "Rating " + std::to_string(park.rating) : deadline;
                return p;
            case Scenario::ObjectiveType::guestsAndRating:
                p.text = Thousands(park.numGuestsInPark) + " / " + Thousands(objective.NumGuests) + " guests";
                p.percent = Percent(park.numGuestsInPark, objective.NumGuests);
                p.asideWarns = park.rating < Scenario::Objective::kLowParkRatingThreshold;
                p.aside = "Rating " + std::to_string(park.rating);
                return p;
            case Scenario::ObjectiveType::parkValueBy:
                p.text = FormatMoney(park.value) + " / " + FormatMoney(objective.Currency);
                p.percent = Percent(park.value, objective.Currency);
                p.aside = deadline;
                return p;
            case Scenario::ObjectiveType::repayLoanAndParkValue:
                if (park.bankLoan > 0)
                {
                    p.text = "Repay loan: " + FormatMoney(park.bankLoan);
                    p.percent = Percent(park.value, objective.Currency);
                    p.aside = "then value";
                    p.asideWarns = true;
                }
                else
                {
                    p.text = FormatMoney(park.value) + " / " + FormatMoney(objective.Currency);
                    p.percent = Percent(park.value, objective.Currency);
                    p.aside = "Park value";
                }
                return p;
            case Scenario::ObjectiveType::monthlyRideIncome:
            {
                const auto income = park.expenditureTable[1][EnumValue(ExpenditureType::parkRideTickets)];
                p.text = FormatMoney(income) + " / " + FormatMoney(objective.Currency);
                p.percent = Percent(income, objective.Currency);
                p.aside = "Rides / mo";
                return p;
            }
            case Scenario::ObjectiveType::monthlyFoodIncome:
            {
                const auto* month = park.expenditureTable[1];
                const auto profit = month[EnumValue(ExpenditureType::shopSales)] + month[EnumValue(ExpenditureType::shopStock)]
                    + month[EnumValue(ExpenditureType::foodDrinkSales)] + month[EnumValue(ExpenditureType::foodDrinkStock)];
                p.text = FormatMoney(profit) + " / " + FormatMoney(objective.Currency);
                p.percent = Percent(profit, objective.Currency);
                p.aside = "Shops / mo";
                return p;
            }
            case Scenario::ObjectiveType::tenRollercoasters:
            {
                const int count = Coasters(RideRating::make(6, 0));
                p.text = std::to_string(count) + " / 10 coasters";
                p.percent = count * 10;
                p.aside = "Exc. 6.00+";
                return p;
            }
            case Scenario::ObjectiveType::tenRollercoastersLength:
            {
                const int count = Coasters(RideRating::make(7, 0), objective.MinimumLength);
                p.text = std::to_string(count) + " / 10 coasters";
                p.percent = count * 10;
                p.aside = "Exc. 7.00+";
                return p;
            }
            case Scenario::ObjectiveType::finishFiveRollercoasters:
            {
                const int count = FinishedCoasters(objective.MinimumExcitement);
                p.text = std::to_string(count) + " / 5 finished";
                p.percent = count * 20;
                p.aside = "Coasters";
                return p;
            }
            default:
                // Have fun, build the best, or no goal: nothing to track.
                return std::nullopt;
        }
    }
} // namespace OpenRCT2::Ui::Handheld::Goal
