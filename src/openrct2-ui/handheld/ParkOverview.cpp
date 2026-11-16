// SPDX-License-Identifier: GPL-3.0-only
#include "ParkOverview.h"

#include "Alerts.h"
#include "Finances.h"

#include <openrct2-ui/interface/Objective.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/park/ParkSetParameterAction.h>
#include <openrct2/drawing/Drawing.String.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/scenario/Scenario.h>

namespace OpenRCT2::Ui::Handheld::ParkOverview
{
    bool StatusPermitted()
    {
        // Match the native Park entrance UI; the generic parameter action does not enforce this UI rule.
        return getGameState().scenarioOptions.objective.Type != Scenario::ObjectiveType::guestsAndRating;
    }
    std::optional<Draft> Begin()
    {
        if (!StatusPermitted())
            return {};
        const auto& state = getGameState();
        const auto open = state.park.flags.has(ParkFlag::parkOpen);
        return Draft{ open, !open, state.park.flags.has(ParkFlag::noMoney), state.scenarioOptions.objective,
                      state.scenarioCompletedCompanyValue };
    }
    static GameActions::Result Invalid(const char* text)
    {
        GameActions::Result result;
        result.error = GameActions::Status::disallowed;
        result.errorMessage = text;
        return result;
    }
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback)
    {
        const auto& state = getGameState();
        const auto& objective = state.scenarioOptions.objective;
        if (!StatusPermitted())
            return Invalid("Park status locked by objective");
        if (state.park.flags.has(ParkFlag::parkOpen) != draft.baselineOpen
            || state.park.flags.has(ParkFlag::noMoney) != draft.noMoney || objective.Type != draft.objective.Type
            || objective.Year != draft.objective.Year || objective.NumGuests != draft.objective.NumGuests
            || objective.Currency != draft.objective.Currency || state.scenarioCompletedCompanyValue != draft.outcome)
            return Invalid("Park/settings changed; reopen review");
        if (draft.open == draft.baselineOpen)
            return Invalid("Park status unchanged");
        GameActions::ParkSetParameterAction action(
            draft.open ? GameActions::ParkParameter::open : GameActions::ParkParameter::close);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    std::string ObjectiveText()
    {
        const auto& objective = getGameState().scenarioOptions.objective;
        const auto index = static_cast<size_t>(objective.Type);
        if (index >= static_cast<size_t>(Scenario::ObjectiveType::count))
            return "Objective unavailable";
        Formatter ft;
        Ui::formatObjective(ft, objective);
        return FmtString(FormatStringIDLegacy(Ui::kObjectiveNames[index], ft.Data())).WithoutFormatTokens();
    }
    std::string Outcome()
    {
        const auto& state = getGameState();
        if (state.scenarioCompletedCompanyValue == kCompanyValueOnFailedObjective)
            return "Failed";
        if (state.scenarioCompletedCompanyValue != kMoney64Undefined)
            return "Completed";
        const auto type = state.scenarioOptions.objective.Type;
        if (type == Scenario::ObjectiveType::none || type == Scenario::ObjectiveType::haveFun)
            return "No completion target";
        if (type >= Scenario::ObjectiveType::count)
            return "Unavailable";
        return "In progress";
    }
    ManagementPresentation Presentation(size_t selected, const std::optional<Draft>& draft, const std::string& feedback)
    {
        const auto& state = getGameState();
        const auto& park = state.park;
        ManagementPresentation model;
        model.breadcrumb = "Park / Overview";
        model.title = park.name;
        model.selected = selected;
        model.status = feedback;
        const auto status = park.flags.has(ParkFlag::parkOpen) ? "Open" : "Closed";
        if (draft)
        {
            model.breadcrumb = "Overview / Review park status";
            model.confirmation = true;
            model.primaryAction = selected == 1 ? "Cancel" : "Confirm";
            const auto query = Action(*draft, false);
            model.stats = { { "Current", status },
                            { "Requested", draft->open ? "Open" : "Closed" },
                            { "Action cost",
                              query.error == GameActions::Status::ok ? FormatMoney(query.cost) : "Unavailable" } };
            model.rows = { "Confirm", "Cancel" };
            if (feedback.empty() && query.error != GameActions::Status::ok)
                model.status = Finances::Feedback(query);
            return model;
        }
        model.summaryMetrics = true;
        model.actionGrid = true;
        model.stats = { { "Park status", status },
                        { "Guests in park", std::to_string(park.numGuestsInPark) },
                        { "Park rating", std::to_string(park.rating) + " / 999" },
                        { "Park value",
                          park.flags.has(ParkFlag::noMoney)     ? "Not applicable"
                              : park.value == kMoney64Undefined ? "Unavailable"
                                                                : FormatMoney(park.value) } };
        // The home screen leads with what needs the player. When something does, it replaces the long
        // objective text (which has its own page) so the action grid still fits on a small screen.
        if (const auto& alerts = Alerts::Current(); !alerts.empty())
        {
            model.bodyLines.push_back("{CAUTION}Needs attention  -  Y in the park visits each one");
            for (size_t i = 0; i < std::min<size_t>(alerts.size(), 3); ++i)
                model.bodyLines.push_back(
                    std::string(alerts[i].kind <= Alerts::Kind::breakdown ? "{DANGER}" : "{WHITE}") + alerts[i].title
                    + "  -  " + alerts[i].fix);
            if (alerts.size() > 3)
                model.bodyLines.push_back("{MUTED}and " + std::to_string(alerts.size() - 3) + " more");
            model.bodyLines.push_back("{MUTED}Objective: " + Outcome());
            model.rows = { StatusPermitted() ? (park.flags.has(ParkFlag::parkOpen) ? "Close park..." : "Open park...")
                                             : "Park status locked",
                           Finances::Available(Finances::Field::admission) ? "Admission..." : "Admission unavailable",
                           "Park map...", "Park news...", "Park awards...", "Park history..." };
            return model;
        }
        model.bodyLines.push_back("{POSITIVE}All clear - nothing needs you right now");
        std::string wrapped;
        int32_t count{};
        // Reuse the desktop objective string/arguments and native UTF-8/format-aware wrapping.
        Drawing::wrapString(ObjectiveText(), 320, FontStyle::medium, &wrapped, &count);
        size_t start = 0;
        while (start < wrapped.size())
        {
            const auto end = wrapped.find('\0', start);
            model.bodyLines.push_back(wrapped.substr(start, end == std::string::npos ? end : end - start));
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        model.bodyLines.push_back("Objective: " + Outcome());
        model.rows = { StatusPermitted() ? (park.flags.has(ParkFlag::parkOpen) ? "Close park..." : "Open park...")
                                         : "Park status locked",
                       Finances::Available(Finances::Field::admission) ? "Admission..." : "Admission unavailable", "Park map...", "Park news...", "Park awards...", "Park history..." };
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::ParkOverview
