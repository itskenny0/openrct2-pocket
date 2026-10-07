// SPDX-License-Identifier: GPL-3.0-only
#include "Finances.h"

#include <openrct2/GameState.h>
#include <openrct2/actions/park/ParkSetEntranceFeeAction.h>
#include <openrct2/actions/park/ParkSetLoanAction.h>
#include <openrct2/world/Park.h>

namespace OpenRCT2::Ui::Handheld::Finances
{
    bool Available(Field field)
    {
        const auto& park = getGameState().park;
        return !park.flags.has(ParkFlag::noMoney) && (field == Field::loan || Park::EntranceFeeUnlocked(park));
    }
    Draft Begin(Field field)
    {
        const auto& park = getGameState().park;
        const auto value = field == Field::loan ? park.bankLoan : park.entranceFee;
        return { field, value, value };
    }
    void Adjust(Draft& draft, int direction)
    {
        const auto& park = getGameState().park;
        const auto maximum = draft.field == Field::loan ? std::max({ park.bankLoan, park.maxBankLoan, money64{ 0 } })
                                                        : kMaxEntranceFee;
        const money64 step = draft.field == Field::loan ? 1000.00_GBP : 1.00_GBP;
        // Saturate before adding: loan limits are native money64 values.
        if (direction > 0)
            draft.value = draft.value >= maximum - step ? maximum : draft.value + step;
        else if (direction < 0)
            draft.value = draft.value <= step ? 0 : draft.value - step;
    }
    GameActions::Result Action(const Draft& draft, bool execute)
    {
        GameActions::Result invalid;
        invalid.error = GameActions::Status::disallowed;
        if (!Available(draft.field))
        {
            invalid.errorMessage = getGameState().park.flags.has(ParkFlag::noMoney) ? "No-money park"
                                                                                    : "Admission price locked";
            return invalid;
        }
        if (Begin(draft.field).baseline != draft.baseline)
        {
            invalid.errorMessage = "Value changed; reopen editor";
            return invalid;
        }
        if (draft.field == Field::loan)
        {
            auto action = GameActions::ParkSetLoanAction(draft.value);
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        auto action = GameActions::ParkSetEntranceFeeAction(draft.value);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    std::string Feedback(const GameActions::Result& result)
    {
        auto text = result.getErrorMessage();
        return text.empty() ? result.getErrorTitle() : text;
    }
    std::pair<money64, money64> MonthlyTotals()
    {
        money64 income = 0, expense = 0;
        for (const auto value : getGameState().park.expenditureTable[0])
        {
            if (value == kMoney64Undefined)
                return { kMoney64Undefined, kMoney64Undefined };
            if (value > 0)
                income += value;
            else
                expense -= value;
        }
        return { income, expense };
    }
    ManagementPresentation Presentation(
        View view, size_t selected, const std::optional<Draft>& draft, const std::string& status)
    {
        const auto& park = getGameState().park;
        ManagementPresentation model;
        model.breadcrumb = "Park dashboard / Finances";
        model.title = "Finances";
        model.selected = selected;
        model.status = status;
        const bool noMoney = park.flags.has(ParkFlag::noMoney);
        if (view == View::overview || !draft)
        {
            const auto [income, expense] = MonthlyTotals();
            auto money = [&](money64 value) { return noMoney ? std::string("Not applicable") : FormatMoney(value); };
            model.stats = { { "Cash", money(park.cash) },
                            { "Loan", money(park.bankLoan) },
                            { "Month income", money(income) },
                            { "Month spent", money(expense) },
                            { "Current profit", money(park.currentProfit) },
                            { "Admission", noMoney ? "Free" : FormatMoney(Park::GetEntranceFee(park)) } };
            model.rows = { Available(Field::loan) ? "Borrow / repay..." : "Loan unavailable",
                           Available(Field::admission) ? "Admission price..."
                               : noMoney               ? "No-money park"
                                                       : "Admission locked",
                           "Back to dashboard" };
            return model;
        }
        const bool loan = draft->field == Field::loan;
        model.title = loan ? "Loan balance" : "Park admission";
        model.editing = view == View::editor;
        model.confirmation = view == View::confirmation;
        model.stats = { { "Current", FormatMoney(draft->baseline) },
                        { model.editing ? "Draft" : "New value", FormatMoney(draft->value) } };
        if (model.editing)
            model.highlightedStat = 1;
        if (loan)
        {
            const auto change = draft->value - draft->baseline;
            model.stats.push_back({ "Cash change", FormatMoney(change) });
            model.stats.push_back(
                { "Cash after", FormatMoney(park.cash == kMoney64Undefined ? kMoney64Undefined : park.cash + change) });
            model.stats.push_back({ "Loan limit", FormatMoney(park.maxBankLoan) });
            model.stats.push_back({ "Interest", std::to_string(park.bankLoanInterestRate) + "%" });
        }
        else
            model.stats.push_back({ "Cash change", FormatMoney(0) });
        model.rows = model.editing
            ? std::vector<std::string>{ "Review change...", "Cancel" }
            : std::vector<std::string>{ loan ? draft->value >= draft->baseline ? "Borrow" : "Repay" : "Set admission",
                                        "Cancel" };
        const auto result = Action(*draft, false);
        if (model.status.empty())
            model.status = result.error == GameActions::Status::ok
                ? model.editing ? "Step " + FormatMoney(loan ? 1000.00_GBP : 1.00_GBP) : ""
                : Feedback(result);
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::Finances
