// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"
#include "ConstructionPanel.h"
#include "StaffManagement.h"

#include <openrct2/actions/peep/StaffSetPatrolAreaAction.h>
#include <openrct2/drawing/Drawing.Screen.h>
#include <openrct2/entity/PatrolArea.h>
namespace OpenRCT2::Ui::Handheld::StaffPatrol
{
    struct Settings
    {
        int width = 1, height = 1;
        bool remove{};
    };
    inline Settings settings;
    inline bool busy{};
    inline uint64_t revision{};
    inline std::optional<StaffManagement::Target> target;
    inline bool Active()
    {
        return target.has_value();
    }
    inline void HideOverlay()
    {
        const auto owner = GetPatrolAreaToRender();
        const auto* id = std::get_if<EntityId>(&owner);
        if (target && id && *id == target->id)
        {
            ClearPatrolAreaToRender();
            Drawing::GfxInvalidateScreen();
        }
    }
    inline void End()
    {
        HideOverlay();
        target.reset();
        busy = false;
        ++revision;
    }
    inline bool Begin(const StaffManagement::Target& employee)
    {
        End();
        if (!StaffManagement::Resolve(employee))
            return false;
        target = employee;
        settings = {};
        SetPatrolAreaToRender(employee.id);
        Drawing::GfxInvalidateScreen();
        return true;
    }
    inline void ShowOverlay()
    {
        if (!target || !StaffManagement::Resolve(*target))
            return;
        const auto owner = GetPatrolAreaToRender();
        const auto* id = std::get_if<EntityId>(&owner);
        if (!id || *id != target->id)
        {
            SetPatrolAreaToRender(target->id);
            Drawing::GfxInvalidateScreen();
        }
    }
    inline std::optional<MapRange> Range(std::optional<CoordsXY> tile)
    {
        return AreaTools::Range(tile, settings.width, settings.height);
    }
    inline std::vector<TileCoordsXY> Snapshot(const StaffManagement::Target& employee)
    {
        const auto* staff = StaffManagement::Resolve(employee);
        return staff && staff->patrolInfo ? staff->patrolInfo->toVector() : std::vector<TileCoordsXY>{};
    }
    inline GameActions::Result Action(std::optional<CoordsXY> tile, bool execute)
    {
        if (busy)
            return AreaTools::Invalid("Patrol action pending");
        if (!target || !StaffManagement::Resolve(*target))
            return AreaTools::Invalid("Employee unavailable");
        const auto range = Range(tile);
        if (!range)
            return AreaTools::Invalid("Patrol selection outside map");
        GameActions::StaffSetPatrolAreaAction action(
            target->id, *range,
            settings.remove ? GameActions::StaffSetPatrolAreaMode::unset : GameActions::StaffSetPatrolAreaMode::set);
        if (execute)
        {
            busy = true;
            const auto request = revision;
            action.SetCallback([request](const auto*, const auto*) {
                if (request == revision)
                    busy = false;
            });
            const auto result = GameActions::Execute(&action, getGameState());
            if (result.error != GameActions::Status::ok)
                busy = false;
            return result;
        }
        return GameActions::Query(&action, getGameState());
    }
    struct Draft
    {
        StaffManagement::Target employee;
        std::vector<TileCoordsXY> tiles;
    };
    inline std::optional<Draft> ReviewClear()
    {
        if (!target || !StaffManagement::Resolve(*target))
            return {};
        auto tiles = Snapshot(*target);
        if (tiles.empty())
            return {};
        return Draft{ *target, std::move(tiles) };
    }
    inline GameActions::Result Clear(const Draft& draft, bool execute)
    {
        if (busy)
            return AreaTools::Invalid("Patrol action pending");
        if (!StaffManagement::Resolve(draft.employee) || draft.tiles.empty() || Snapshot(draft.employee) != draft.tiles)
            return AreaTools::Invalid("Patrol changed; reopen review");
        GameActions::StaffSetPatrolAreaAction action(
            draft.employee.id, MapRange{ 32, 32, 32, 32 }, GameActions::StaffSetPatrolAreaMode::clearAll);
        if (execute)
        {
            busy = true;
            const auto request = revision;
            action.SetCallback([request](const auto*, const auto*) {
                if (request == revision)
                    busy = false;
            });
            const auto result = GameActions::Execute(&action, getGameState());
            if (result.error != GameActions::Status::ok)
                busy = false;
            return result;
        }
        return GameActions::Query(&action, getGameState());
    }
    inline std::vector<std::string> Options()
    {
        return { "Resume patrol",
                 settings.remove ? "Mode: Remove" : "Mode: Add",
                 "Width: " + std::to_string(settings.width),
                 "Height: " + std::to_string(settings.height),
                 "Clear patrol...",
                 "Return to employee" };
    }
    inline void Configure(size_t row, int delta = 1)
    {
        if (row == 1)
            settings.remove = !settings.remove;
        if (row == 2)
            settings.width = std::clamp(settings.width + delta, 1, 64);
        if (row == 3)
            settings.height = std::clamp(settings.height + delta, 1, 64);
    }
    inline void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY pos)
    {
        const auto* staff = target ? StaffManagement::Resolve(*target) : nullptr;
        DrawConstructionOverlay(
            rt, pos, "Staff patrol", staff ? staff->getName() : "Employee unavailable",
            settings.remove ? "Remove patrol tiles" : "Add patrol tiles",
            std::to_string(settings.width) + " x " + std::to_string(settings.height) + " tiles",
            target ? std::to_string(Snapshot(*target).size()) + " patrol tiles" : "No employee");
    }
    inline ManagementPresentation Presentation(const Draft& draft, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Staff / Clear patrol";
        const auto* staff = StaffManagement::Resolve(draft.employee);
        model.title = staff ? staff->getName() : "Employee unavailable";
        model.stats = { { "Patrol tiles", std::to_string(draft.tiles.size()) }, { "After clearing", "Unrestricted patrol" } };
        model.rows = { "Clear patrol", "Cancel" };
        model.selected = selected;
        model.confirmation = true;
        auto result = Clear(draft, false);
        if (result.error != GameActions::Status::ok)
            model.status = result.getErrorMessage();
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::StaffPatrol
