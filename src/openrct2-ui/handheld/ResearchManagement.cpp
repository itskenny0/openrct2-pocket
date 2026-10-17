// SPDX-License-Identifier: GPL-3.0-only
#include "ResearchManagement.h"

#include "Finances.h"

#include <openrct2/GameState.h>
#include <openrct2/actions/park/ParkSetResearchFundingAction.h>
#include <openrct2/localisation/Localisation.Date.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/management/Finance.h>
#include <openrct2/management/Research.h>
#include <openrct2/world/Park.h>

namespace OpenRCT2::Ui::Handheld::ResearchManagement
{
    static constexpr const char* categories[] = { "Transport",   "Gentle rides",       "Roller coasters",  "Thrill rides",
                                                  "Water rides", "Shops / facilities", "Scenery / theming" };
    bool FundingAvailable()
    {
        const auto& state = getGameState();
        return !state.park.flags.has(ParkFlag::noMoney) && state.researchProgressStage != RESEARCH_STAGE_FINISHED_ALL;
    }
    uint8_t AvailableCategories()
    {
        uint8_t mask = 0;
        for (const auto& item : getGameState().researchItemsUninvented)
            if (static_cast<unsigned>(item.category) < 7)
                mask |= 1u << static_cast<unsigned>(item.category);
        return mask;
    }
    Draft Begin()
    {
        const auto& state = getGameState();
        return { state.researchFundingLevel, state.researchPriorities, state.researchFundingLevel, state.researchPriorities };
    }
    void AdjustFunding(Draft& draft, int direction)
    {
        if (FundingAvailable())
            draft.funding = static_cast<uint8_t>(
                std::clamp(static_cast<int>(draft.funding) + direction, 0, RESEARCH_FUNDING_COUNT - 1));
    }
    void ToggleCategory(Draft& draft, size_t category)
    {
        if (category < 7 && (AvailableCategories() & (1u << category)))
            draft.priorities ^= 1u << category;
    }
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback)
    {
        const auto& state = getGameState();
        GameActions::Result invalid;
        invalid.error = GameActions::Status::disallowed;
        if (draft.baselineFunding != state.researchFundingLevel || draft.baselinePriorities != state.researchPriorities)
            invalid.errorMessage = "Settings changed; reopen draft";
        else if (draft.funding != draft.baselineFunding && !FundingAvailable())
            invalid.errorMessage = "Funding unavailable";
        else if (((draft.priorities ^ draft.baselinePriorities) & ~AvailableCategories()) != 0)
            invalid.errorMessage = "Category changed; reopen draft";
        else
        {
            auto action = GameActions::ParkSetResearchFundingAction(draft.priorities, draft.funding);
            action.SetCallback(std::move(callback));
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        return invalid;
    }
    static std::string Funding(uint8_t level)
    {
        return level < RESEARCH_FUNDING_COUNT ? FormatStringID(kResearchFundingLevelNames[level]) : "Unavailable";
    }
    static std::string Cost(uint8_t level)
    {
        return getGameState().park.flags.has(ParkFlag::noMoney) ? "Not applicable"
            : level < RESEARCH_FUNDING_COUNT                    ? FormatMoney(kResearchCosts[level])
                                                                : "Unavailable";
    }
    ManagementPresentation Presentation(
        View view, size_t selected, const std::optional<Draft>& draft, const std::string& status)
    {
        const auto& state = getGameState();
        ManagementPresentation model;
        model.breadcrumb = "Park dashboard / Research";
        model.title = "Research";
        model.selected = selected;
        model.status = status;
        if (view == View::overview || !draft)
        {
            const bool finished = state.researchProgressStage == RESEARCH_STAGE_FINISHED_ALL;
            static constexpr StringId stages[] = { STR_RESEARCH_STAGE_INITIAL_RESEARCH, STR_RESEARCH_STAGE_DESIGNING,
                                                   STR_RESEARCH_STAGE_COMPLETING_DESIGN, STR_RESEARCH_STAGE_UNKNOWN };
            std::string project = "Unknown";
            if (finished)
                project = "All research completed";
            else if (state.researchNextItem && state.researchNextItem->exists())
            {
                if (state.researchProgressStage == RESEARCH_STAGE_DESIGNING)
                    project = FormatStringID(state.researchNextItem->getCategoryName());
                else if (state.researchProgressStage == RESEARCH_STAGE_COMPLETING_DESIGN)
                    project = FormatStringID(state.researchNextItem->getName());
            }
            const std::string progress = finished                 ? "Completed"
                : state.researchProgressStage < std::size(stages) ? FormatStringID(stages[state.researchProgressStage])
                                                                  : "Unavailable";
            std::string expected = "Unknown";
            if (!finished && state.researchFundingLevel != RESEARCH_FUNDING_NONE
                && state.researchProgressStage != RESEARCH_STAGE_INITIAL_RESEARCH
                && state.researchExpectedDay < std::size(DateDayNames)
                && state.researchExpectedMonth < std::size(DateGameMonthNames))
                expected = FormatStringID(
                    STR_RESEARCH_EXPECTED_FORMAT, DateDayNames[state.researchExpectedDay],
                    DateGameMonthNames[state.researchExpectedMonth]);
            if (finished)
                expected = "Not applicable";
            model.stats = { { "Project", project },
                            { "Progress", progress },
                            { "Expected", expected },
                            { "Funding",
                              state.park.flags.has(ParkFlag::noMoney) ? "No-money park" : Funding(state.researchFundingLevel) },
                            { "Monthly cost", Cost(state.researchFundingLevel) } };
            model.rows = { FundingAvailable() || AvailableCategories() ? "Funding / priorities..." : "Settings unavailable",
                           "Back to dashboard", "Invention catalog..." };
            if (model.status.empty() && finished)
                model.status = "No projects remaining";
            return model;
        }
        const bool editing = view == View::editor;
        model.title = editing ? "Research settings" : "Review research";
        model.editing = editing;
        model.confirmation = !editing;
        model.stats = { { editing ? "Monthly cost" : "Current funding",
                          editing ? Cost(draft->funding) : Funding(draft->baselineFunding) },
                        { editing ? "Upfront charge" : "Monthly cost", editing ? FormatMoney(0) : Cost(draft->funding) } };
        if (!editing)
            model.title = "Review: " + Funding(draft->funding);
        if (editing)
            model.rows.push_back(FundingAvailable() ? "Funding: " + Funding(draft->funding) : "Funding unavailable");
        for (size_t i = 0; i < 7; ++i)
        {
            const bool available = AvailableCategories() & (1u << i);
            model.rows.push_back(
                std::string(categories[i])
                + (available ? (draft->priorities & (1u << i) ? "  On" : "  Off") : "  No projects"));
        }
        model.rows.push_back(editing ? "Review settings..." : "Apply settings");
        model.rows.push_back("Cancel");
        model.primaryAction = editing ? selected == 0 ? "Change"
                : selected < 8                        ? "Toggle"
                                                      : "Select"
            : selected < 7            ? "Review"
                                      : "Select";
        const auto result = Action(*draft, false);
        if (model.status.empty() && result.error != GameActions::Status::ok)
            model.status = Finances::Feedback(result);
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::ResearchManagement
