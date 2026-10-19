// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"
#include "Finances.h"
#include "ManagementPanel.h"

#include <openrct2/OpenRCT2.h>
#include <openrct2/actions/park/LandBuyRightsAction.h>
namespace OpenRCT2::Ui::Handheld::RightsTools
{
    struct Settings
    {
        int width = 1, height = 1;
        bool construction{};
    };
    inline Settings settings;
    struct Counts
    {
        size_t owned{}, available{}, unavailable{};
    };
    inline Counts Count(const MapRange& range, bool construction)
    {
        Counts c;
        for (int y = range.getY1(); y <= range.getY2(); y += 32)
            for (int x = range.getX1(); x <= range.getX2(); x += 32)
            {
                const auto* surface = MapGetSurfaceElementAt(CoordsXY{ x, y });
                if (!surface)
                {
                    ++c.unavailable;
                    continue;
                }
                const auto own = surface->getOwnership();
                if (own.has(OwnershipFlag::landOwned) || (construction && own.has(OwnershipFlag::constructionRightsOwned)))
                    ++c.owned;
                else if (own.has(construction ? OwnershipFlag::constructionRightsForSale : OwnershipFlag::landForSale))
                    ++c.available;
                else
                    ++c.unavailable;
            }
        return c;
    }
    inline std::vector<uint8_t> Snapshot(const MapRange& range)
    {
        std::vector<uint8_t> values;
        for (int y = range.getY1(); y <= range.getY2(); y += 32)
            for (int x = range.getX1(); x <= range.getX2(); x += 32)
            {
                const auto* surface = MapGetSurfaceElementAt(CoordsXY{ x, y });
                values.push_back(surface ? surface->getOwnership().holder : 0);
            }
        return values;
    }
    inline GameActions::Result Query(std::optional<CoordsXY> tile, const Settings& s)
    {
        const auto range = AreaTools::Range(tile, s.width, s.height);
        if (!range || gLegacyScene != LegacyScene::playing)
            return AreaTools::Invalid("Rights selection unavailable");
        const auto c = Count(*range, s.construction);
        if (!c.available)
            return AreaTools::Invalid(
                c.owned              ? "Selection owned or unavailable"
                    : s.construction ? "Construction rights not for sale"
                                     : "Land not for sale");
        GameActions::LandBuyRightsAction action(
            *range,
            s.construction ? GameActions::LandBuyRightSetting::buyConstructionRights
                           : GameActions::LandBuyRightSetting::buyLand);
        return GameActions::Query(&action, getGameState());
    }
    struct Draft
    {
        CoordsXY tile;
        Settings settings;
        std::vector<uint8_t> ownership;
        money64 cost, cash, landPrice, rightsPrice;
        bool noMoney;
    };
    inline std::optional<Draft> Begin(std::optional<CoordsXY> tile, const Settings& s)
    {
        const auto range = AreaTools::Range(tile, s.width, s.height);
        const auto q = Query(tile, s);
        if (!range || q.error != GameActions::Status::ok)
            return {};
        const auto& state = getGameState();
        return Draft{ *tile,
                      s,
                      Snapshot(*range),
                      q.cost,
                      state.park.cash,
                      state.scenarioOptions.landPrice,
                      state.scenarioOptions.constructionRightsPrice,
                      state.park.flags.has(ParkFlag::noMoney) };
    }
    inline GameActions::Result Action(const Draft& d, bool execute)
    {
        const auto range = AreaTools::Range(d.tile, d.settings.width, d.settings.height);
        const auto& state = getGameState();
        if (!range || Snapshot(*range) != d.ownership || state.park.cash != d.cash
            || state.scenarioOptions.landPrice != d.landPrice || state.scenarioOptions.constructionRightsPrice != d.rightsPrice
            || state.park.flags.has(ParkFlag::noMoney) != d.noMoney)
            return AreaTools::Invalid("Ownership or price changed; reopen review");
        const auto q = Query(d.tile, d.settings);
        if (q.error != GameActions::Status::ok || q.cost != d.cost)
            return q.error != GameActions::Status::ok ? q : AreaTools::Invalid("Rights cost changed; reopen review");
        if (!execute)
            return q;
        GameActions::LandBuyRightsAction action(
            *range,
            d.settings.construction ? GameActions::LandBuyRightSetting::buyConstructionRights
                                    : GameActions::LandBuyRightSetting::buyLand);
        return GameActions::Execute(&action, getGameState());
    }
    inline std::string Kind(bool construction)
    {
        return construction ? "Construction rights" : "Land ownership";
    }
    inline std::string Summary(std::optional<CoordsXY> tile, const Settings& s)
    {
        const auto range = AreaTools::Range(tile, s.width, s.height);
        if (!range)
            return "Selection off map";
        const auto c = Count(*range, s.construction);
        return std::to_string(c.owned) + " owned / " + std::to_string(c.available) + " for sale";
    }
    inline std::vector<std::string> Options()
    {
        return { "Resume", Kind(settings.construction), "Width: " + std::to_string(settings.width),
                 "Height: " + std::to_string(settings.height) };
    }
    inline void Configure(size_t row, int delta = 1)
    {
        if (row == 1)
            settings.construction = !settings.construction;
        if (row == 2)
            settings.width = std::clamp(settings.width + delta, 1, 64);
        if (row == 3)
            settings.height = std::clamp(settings.height + delta, 1, 64);
    }
    inline ManagementPresentation Presentation(const Draft& draft, size_t selected, const std::string& status = {})
    {
        ManagementPresentation model;
        model.breadcrumb = "Build / Purchase rights";
        model.title = Kind(draft.settings.construction);
        model.confirmation = true;
        model.selected = selected;
        model.rows = { "Purchase available tiles", "Cancel" };
        const auto range = AreaTools::Range(draft.tile, draft.settings.width, draft.settings.height);
        if (range)
        {
            const auto c = Count(*range, draft.settings.construction);
            model.stats = { { "For sale", std::to_string(c.available) },
                            { "Already owned", std::to_string(c.owned) },
                            { "Unavailable", std::to_string(c.unavailable) },
                            { "Cost", FormatMoney(draft.cost) } };
        }
        const auto q = Action(draft, false);
        model.status = status.empty()
            ? (q.error == GameActions::Status::ok ? "Unavailable tiles are skipped" : q.getErrorMessage())
            : status;
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::RightsTools
