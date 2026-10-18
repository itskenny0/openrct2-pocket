// SPDX-License-Identifier: GPL-3.0-only
#include "ShopManagement.h"

#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2-ui/UiStringIds.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideSetPriceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/localisation/StringIds.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>

namespace OpenRCT2::Ui::Handheld::ShopManagement
{
    static GameActions::Result Missing()
    {
        GameActions::Result result;
        result.error = GameActions::Status::invalidParameters;
        result.errorMessage = "Shop no longer available";
        return result;
    }
    const Ride* Find(const Target& target)
    {
        const auto* ride = GetRide(target.id);
        if (!ride || ride->type != target.type || ride->subtype != target.object || ride->buildDate != target.buildDate
            || !ride->getRideEntry() || !ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility)
            || !MapIsLocationValid(target.location))
            return nullptr;
        for (const auto* e : TileElementsView<TrackElement>(target.location))
            if (!e->isGhost() && e->getRideIndex() == target.id && e->getBaseZ() == target.location.z
                && e->getDirection() == target.direction)
                return ride;
        return nullptr;
    }
    std::optional<Target> Capture(RideId id)
    {
        const auto* ride = GetRide(id);
        if (!ride || !ride->getRideEntry() || !ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility)
            || ride->overallView.isNull() || !MapIsLocationValid(ride->overallView))
            return {};
        for (const auto* e : TileElementsView<TrackElement>(ride->overallView))
            if (!e->isGhost() && e->getRideIndex() == id)
                return Target{
                    id, ride->type, ride->subtype, ride->buildDate, { ride->overallView, e->getBaseZ() }, e->getDirection()
                };
        return {};
    }
    std::optional<Target> FromInfo(const InteractionInfo& info)
    {
        if (info.interactionType != ViewportInteractionItem::ride || !info.Element || info.Element->isGhost()
            || info.Element->getType() != TileElementType::track)
            return {};
        const auto target = Capture(info.Element->getRideIndex());
        return target && target->location.x == (info.Loc.x & ~31) && target->location.y == (info.Loc.y & ~31) ? target
                                                                                                              : std::nullopt;
    }
    std::vector<Price> Prices(const Target& target)
    {
        std::vector<Price> result;
        const auto* ride = Find(target);
        if (!ride)
            return result;
        for (size_t slot = 0; slot < 2; ++slot)
        {
            auto item = ride->getRideEntry()->shop_item[slot];
            if (slot == 0 && ride->getRideTypeDescriptor().specialType == RtdSpecialType::toilet)
                item = ShopItem::admission;
            if (item != ShopItem::none)
                result.push_back(
                    { slot == 0, item, FormatStringID(GetShopItemDescriptor(item).Naming.Singular), ride->price[slot],
                      ShopItemHasCommonPrice(item) });
        }
        return result;
    }
    GameActions::Result SetStatus(const Target& target, bool open)
    {
        if (!Find(target))
            return Missing();
        auto action = GameActions::RideSetStatusAction(target.id, open ? RideStatus::open : RideStatus::closed);
        return GameActions::Execute(&action, getGameState());
    }
    GameActions::Result SetPrice(const Target& target, bool primary, money64 value)
    {
        const auto prices = Prices(target);
        if (std::none_of(prices.begin(), prices.end(), [primary](const auto& price) { return price.primary == primary; }))
            return Missing();
        auto action = GameActions::RideSetPriceAction(target.id, value, primary);
        return GameActions::Execute(&action, getGameState());
    }
    GameActions::Result Demolish(const Target& target, bool execute)
    {
        if (!Find(target))
            return Missing();
        auto action = GameActions::RideDemolishAction(target.id, GameActions::RideModifyType::demolish);
        if (execute)
            return GameActions::Execute(&action, getGameState());
        auto result = GameActions::Query(&action, getGameState());
        // The native demolition prompt uses this TrackRemove-query refund calculation.
        // RideDemolishAction::Query validates demolition but does not populate its cost.
        if (result.error == GameActions::Status::ok)
            result.cost = RideGetRefundPrice(*Find(target));
        return result;
    }
    std::string Money(money64 value)
    {
        return FormatMoney(value);
    }
    void Draw(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::optional<Target>& target, View view, size_t focus,
        std::optional<money64> draft, const std::string& feedback)
    {
        using namespace Concept;
        constexpr int width = 224;
        const ScreenCoordsXY p{ 8, 8 };
        const auto* ride = target ? Find(*target) : nullptr;
        const auto prices = target ? Prices(*target) : std::vector<Price>{};
        const bool noMoney = getGameState().park.flags.has(ParkFlag::noMoney);
        if (ride && view == View::inspection)
        {
            ManagementPresentation model;
            model.breadcrumb = "Shops / Facilities";
            model.title = ride->getName();
            auto status = Formatter();
            ride->formatStatusTo(status);
            model.headerStatus = FormatStringIDLegacy(STR_STRINGID, status.Data());
            model.headerInk = ride->status == RideStatus::open ? "{POSITIVE}" : "{WHITE}";
            auto earnings = [noMoney](money64 value) {
                return noMoney ? std::string("Not applicable") : value == kMoney64Undefined ? std::string("Unavailable") : Money(value);
            };
            model.stats = {{"Customers", std::to_string(ride->totalCustomers)},
                           {"Customers / hr", std::to_string(RideCustomersPerHour(*ride))},
                           {"Income / hr", earnings(ride->incomePerHour)}, {"Profit / hr", earnings(ride->profit)}};
            model.summaryMetrics = true;
            model.metricColumns = 2;
            model.metricHeight = 34;
            model.rowHeight = 26;
            for (const auto& price : prices)
            {
                const auto sold = price.primary ? ride->numPrimaryItemsSold : ride->numSecondaryItemsSold;
                model.bodyLines.push_back(price.name + "  " + (noMoney ? "Free" : Money(price.value))
                    + (price.item == ShopItem::admission ? "" : " / " + std::to_string(sold) + " sold"));
            }
            model.rows = {ride->status == RideStatus::open ? "Close shop" : "Open shop", "Prices...", "Demolish...", "Rename...", "Appearance..."};
            model.rowEnabled = {true, !noMoney && !prices.empty(), true, true, true};
            model.selected = focus;
            model.status = feedback;
            DrawManagementContent(rt, p, {320, Layout::ContentHeight(screen)}, model);
            DrawFooter(rt, screen, {{"D-pad", "Choose"}, {"A", "Select"}, {"B", "Back"}});
            return;
        }
        Box(rt, p, { width, view == View::inspection ? 348 : 252 });
        auto text = [&](int y, const std::string& s, bool strong = false) {
            if (strong)
                DrawStrongText(rt, p + ScreenCoordsXY{ 10, y }, s, width - 20);
            else
                DrawModernEllipsised(rt, p + ScreenCoordsXY{ 10, y }, width - 20, s);
        };
        auto row = [&](int y, const std::string& s, bool selected) {
            if (selected)
                Box(rt, p + ScreenCoordsXY{ 8, y }, { width - 16, view==View::inspection?23:28 }, true);
            text(y + 9, (selected ? "{BLACK}" : "{WHITE}") + s, selected);
        };
        text(12, "{MUTED}Shops / Facilities");
        text(32, "{WHITE}" + (ride ? ride->getName() : "Shop unavailable"), true);
        Rule(rt, p + ScreenCoordsXY{ 10, 54 }, width - 20);
        if (ride && view == View::prices)
        {
            text(66, "{MUTED}Product / admission prices");
            for (size_t i = 0; i < prices.size(); ++i)
            {
                const auto& price = prices[i];
                row(90 + i * 60, price.name + "  " + Money(i == focus && draft ? *draft : price.value), i == focus);
                text(123 + i * 60, std::string("{MUTED}") + (price.shared ? "Shared throughout park" : "This location only"));
            }
            text(219, "{MUTED}A applies  /  B discards draft");
        }
        else if (ride)
        {
            text(69, "{WHITE}Demolish this location?", true);
            const auto query = Demolish(*target, false);
            text(
                94,
                "{MUTED}"
                    + (query.error == GameActions::Status::ok
                           ? (noMoney ? "No refund / no-money park" : "Refund  " + Money(-query.cost))
                           : query.getErrorMessage()));
            text(117, "{MUTED}This cannot be undone");
            row(150, "Demolish", focus == 0);
            row(182, "Cancel", focus == 1);
        }
        if (!feedback.empty())
            text(view == View::inspection ? 330 : 237, "{MUTED}" + feedback);
        DrawFooter(
            rt, screen,
            { { "D-pad", view == View::prices ? "Price / item" : "Choose" },
              { "A", view == View::prices ? "Apply" : "Select" },
              { "B", "Back" } });
    }
} // namespace OpenRCT2::Ui::Handheld::ShopManagement
