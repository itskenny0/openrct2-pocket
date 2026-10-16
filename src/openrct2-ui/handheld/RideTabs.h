// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "TabbedActions.h"
#include "ManagementPanel.h"
#include <algorithm>
#include <string_view>

namespace OpenRCT2::Ui::Handheld::RideTabs
{
    enum Tab : size_t { overview, operations, statistics, settings, count };
    using State = TabFocus<count>;
    inline const std::vector<std::string> labels{"Overview", "Operations", "Statistics", "Settings"};
    inline Tab Group(std::string_view label)
    {
        if (label == "Rename..." || label == "Appearance..." || label == "Music..."
            || label == "Save design..." || label == "Demolish..." || label == "Back to roster")
            return settings;
        if (label == "Statistics...") return statistics;
        if (label == "Operations..." || label == "Maintenance..." || label == "Vehicles..."
            || label == "Dispatch..." || label == "Continue track..." || label == "Edit maze..."
            || label == "Edit track blocks..." || label == "Access / paths...") return operations;
        return overview;
    }
    inline std::vector<size_t> Indices(const ManagementPresentation& source, size_t tab)
    {
        std::vector<size_t> result;
        // Status controls form one immediate group, rather than being interspersed
        // with inspection/navigation. Everything maps back to its native row.
        const std::vector<std::string_view> order = tab == overview
            ? std::vector<std::string_view>{"Open...", "Close...", "Test...", "Test unavailable",
                "Admission price...", "Price unavailable", "Locate", "Entrances / exits...", "Place entrance / exit..."}
            : tab == operations ? std::vector<std::string_view>{"Operations...", "Maintenance...", "Vehicles...", "Dispatch..."}
            : tab == settings ? std::vector<std::string_view>{"Rename...", "Appearance...", "Music...", "Save design...", "Demolish...", "Back to roster"}
            : std::vector<std::string_view>{};
        for (auto label : order)
            for (size_t i=0;i<source.rows.size();++i)
                if (source.rows[i] == label) result.push_back(i);
        for (size_t i=0;i<source.rows.size();++i)
            if (Group(source.rows[i]) == tab && std::find(result.begin(),result.end(),i)==result.end()) result.push_back(i);
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
        model.rows.clear(); model.rowEnabled.clear(); model.rowDetails.clear();
        model.metricHeight = 40;
        if (state.tab == statistics)
        {
            model.stats = stats.stats;
            model.rows = {"Customers...", "Finances...", "Graphs..."};
        }
        else
        {
            for (const auto index : Indices(source, state.tab))
            {
                model.rows.push_back(source.rows[index]);
                model.rowDetails.push_back(index < source.rowDetails.size() ? source.rowDetails[index] : "");
                model.rowEnabled.push_back(index < source.rowEnabled.size() ? source.rowEnabled[index]
                    : source.rows[index].find("unavailable") == std::string::npos);
            }
        }
        model.actionGrid = true;
        model.summaryMetrics = true;
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::RideTabs
