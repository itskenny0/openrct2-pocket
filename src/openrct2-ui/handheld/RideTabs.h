// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ManagementPanel.h"
#include "TabbedActions.h"

#include <algorithm>
#include <string_view>

namespace OpenRCT2::Ui::Handheld::RideTabs
{
    enum Tab : size_t
    {
        overview,
        operations,
        statistics,
        settings,
        count
    };
    using State = TabFocus<count>;
    inline const std::vector<std::string> labels{ "Overview", "Operations", "Statistics", "Settings" };
    inline Tab Group(std::string_view label)
    {
        if (label == "Rename..." || label == "Appearance..." || label == "Music..." || label == "Save design..."
            || label == "Demolish..." || label == "Back to roster")
            return settings;
        if (label == "Statistics...")
            return statistics;
        if (label == "Operations..." || label == "Maintenance..." || label == "Vehicles..." || label == "Dispatch..."
            || label == "Continue track..." || label == "Edit maze..." || label == "Edit track..."
            || label == "Access / paths...")
            return operations;
        return overview;
    }
    // Open / Close / Test are one state switch in the overview's hero, changed in place; their
    // native rows no longer appear as separate buttons.
    inline bool StatusRow(std::string_view label)
    {
        return label == "Open..." || label == "Close..." || label == "Test..." || label == "Test unavailable";
    }
    inline constexpr std::string_view kStatusRow = "Status";
    inline std::vector<size_t> Indices(const ManagementPresentation& source, size_t tab)
    {
        std::vector<size_t> result;
        // Status controls form one immediate group, rather than being interspersed
        // with inspection/navigation. Everything maps back to its native row.
        const std::vector<std::string_view> order = tab == overview
            ? std::vector<std::string_view>{ "Admission price...", "Price unavailable",   "Place entrance / exit...", "Locate",
                                             "Ride along",         "Entrances / exits..." }
            : tab == operations
            ? std::vector<std::string_view>{ "Operations...", "Vehicles...", "Dispatch...", "Maintenance..." }
            : tab == settings ? std::vector<std::string_view>{ "Rename...",      "Appearance...", "Music...",
                                                               "Save design...", "Demolish...",   "Back to roster" }
                              : std::vector<std::string_view>{};
        for (auto label : order)
            for (size_t i = 0; i < source.rows.size(); ++i)
                if (source.rows[i] == label)
                    result.push_back(i);
        for (size_t i = 0; i < source.rows.size(); ++i)
            if (Group(source.rows[i]) == tab && !(tab == overview && StatusRow(source.rows[i]))
                && std::find(result.begin(), result.end(), i) == result.end())
                result.push_back(i);
        return result;
    }
    inline ManagementPresentation Compose(
        const ManagementPresentation& source, const ManagementPresentation& stats, const State& state, size_t selected)
    {
        auto model = source;
        model.tabs = labels;
        model.selectedTab = state.tab;
        model.headerStatus = source.stats.empty() ? "" : source.stats.front().second;
        model.selected = selected;
        model.headerInk = model.headerStatus == "Crashed" || model.headerStatus == "Broken down" ? "{DANGER}"
            : model.headerStatus == "Open"                                                       ? "{POSITIVE}"
            : model.headerStatus == "Testing"                                                    ? "{CAUTION}"
                                                                                                 : "{WHITE}";
        if (state.tab == overview)
            for (const auto& stat : stats.stats)
                if (stat.first == "Reliability" || stat.first == "Customers" || stat.first == "Profit / hour")
                    model.stats.push_back(stat);
        model.rows.clear();
        model.rowEnabled.clear();
        model.rowDetails.clear();
        model.metricHeight = state.tab == overview ? 38 : 40;
        if (state.tab == overview)
            model.rowHeight = 26;
        if (state.tab == statistics)
        {
            model.stats = stats.stats;
            model.rows = { "Customers...", "Finances...", "Graphs..." };
        }
        else
        {
            for (const auto index : Indices(source, state.tab))
            {
                model.rows.push_back(source.rows[index]);
                model.rowDetails.push_back(index < source.rowDetails.size() ? source.rowDetails[index] : "");
                model.rowEnabled.push_back(
                    index < source.rowEnabled.size() ? source.rowEnabled[index]
                                                     : source.rows[index].find("unavailable") == std::string::npos);
            }
        }
        model.actionGrid = true;
        model.summaryMetrics = true;
        if (state.tab == overview)
        {
            // The header already shows the status; the stat card would say it twice.
            std::erase_if(model.stats, [](const auto& stat) { return stat.first == "Status"; });
            // One row of stats under the hero keeps every action on screen.
            model.metricColumns = static_cast<int>(std::min<size_t>(5, std::max<size_t>(1, model.stats.size())));
            // Row 0 is the hero's state switch (hidden as a button, focusable as the switch).
            model.rows.insert(model.rows.begin(), std::string(kStatusRow));
            model.rowDetails.insert(model.rowDetails.begin(), "");
            model.rowEnabled.insert(model.rowEnabled.begin(), true);
            model.heroRow = 0;
            // The price is a value you change in place, right under the switch (row 1).
            if (model.rows.size() > 1 && (model.rows[1] == "Admission price..." || model.rows[1] == "Price unavailable"))
            {
                model.settingRows = { 1 };
                model.rows[1] = "Admission";
                std::erase_if(model.stats, [](const auto& stat) { return stat.first == "Admission"; });
                model.metricColumns = static_cast<int>(std::min<size_t>(5, std::max<size_t>(1, model.stats.size())));
            }
            // The one obvious next step for an unfinished ride.
            const auto found = std::find(model.rows.begin(), model.rows.end(), "Place entrance / exit...");
            if (found != model.rows.end())
                model.primaryRow = static_cast<size_t>(found - model.rows.begin());
        }
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::RideTabs
