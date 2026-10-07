// SPDX-License-Identifier: GPL-3.0-only
#include "MarketingManagement.h"

#include "Finances.h"

#include <openrct2/GameState.h>
#include <openrct2/actions/park/ParkMarketingAction.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/ShopItem.h>
#include <openrct2/world/Park.h>
#include <unordered_map>

namespace OpenRCT2::Ui::Handheld::MarketingManagement
{
    static std::unordered_map<uint16_t, uint64_t> removedRides;
    void RideRemoved(RideId id)
    {
        if (auto it = removedRides.find(id.ToUnderlying()); it != removedRides.end())
            ++it->second;
    }
    bool NeedsTarget(int type)
    {
        return type == ADVERTISING_CAMPAIGN_RIDE_FREE || type == ADVERTISING_CAMPAIGN_RIDE
            || type == ADVERTISING_CAMPAIGN_FOOD_OR_DRINK_FREE;
    }
    std::string Name(int type)
    {
        // Compact labels for the six native semantic campaign types, not new campaigns.
        static constexpr const char* names[] = { "Free park entry",   "Free ride vouchers", "Half-price park entry",
                                                 "Free food / drink", "Advertise park",     "Advertise ride" };
        return type >= 0 && type < ADVERTISING_CAMPAIGN_COUNT ? names[type] : "Unavailable campaign";
    }
    std::vector<Target> Targets(int type)
    {
        std::vector<Target> result;
        if (type == ADVERTISING_CAMPAIGN_FOOD_OR_DRINK_FREE)
        {
            std::array<bool, static_cast<size_t>(ShopItem::count)> items{};
            for (const auto& ride : RideManager(getGameState()))
                if (ride.status == RideStatus::open)
                    if (const auto* entry = ride.getRideEntry())
                        for (auto item : entry->shop_item)
                            if (item != ShopItem::none && GetShopItemDescriptor(item).IsFoodOrDrink())
                                items[static_cast<size_t>(item)] = true;
            for (size_t i = 0; i < items.size(); ++i)
                if (items[i])
                    result.push_back({ static_cast<int>(i), false });
        }
        else if (type == ADVERTISING_CAMPAIGN_RIDE_FREE || type == ADVERTISING_CAMPAIGN_RIDE)
        {
            for (const auto& ride : RideManager(getGameState()))
            {
                const auto& rtd = ride.getRideTypeDescriptor();
                if (ride.status != RideStatus::open || !ride.isRide() || rtd.flags.has(RtdFlag::isShopOrFacility)
                    || rtd.flags.has(RtdFlag::sellsFood) || rtd.flags.has(RtdFlag::sellsDrinks)
                    || rtd.specialType == RtdSpecialType::toilet)
                    continue;
                result.push_back({ ride.id.ToUnderlying(), true, ride.type, ride.subtype, ride.buildDate, ride.overallView,
                                   removedRides[ride.id.ToUnderlying()] });
            }
            std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                const auto left = TargetName(a), right = TargetName(b);
                return left == right ? a.item < b.item : left < right;
            });
        }
        return result;
    }
    std::string TargetName(const Target& target)
    {
        if (!target.ride)
            return target.item >= 0 && target.item < static_cast<int>(ShopItem::count)
                ? FormatStringID(GetShopItemDescriptor(static_cast<ShopItem>(target.item)).Naming.Plural)
                : "Unavailable";
        const auto* ride = GetRide(RideId::FromUnderlying(target.item));
        return ride && ride->type == target.type && ride->subtype == target.object && ride->buildDate == target.buildDate
                && ride->overallView == target.location && removedRides[ride->id.ToUnderlying()] == target.generation
            ? ride->getName()
            : "Target unavailable";
    }
    bool Eligible(int type)
    {
        return type >= 0 && type < ADVERTISING_CAMPAIGN_COUNT
            && !getGameState().park.flags.has(ParkFlag::forbidMarketingCampaigns) && !MarketingGetCampaign(type)
            && MarketingIsCampaignTypeApplicable(type) && (!NeedsTarget(type) || !Targets(type).empty());
    }
    std::vector<int> Types()
    {
        std::vector<int> result;
        for (int type = 0; type < ADVERTISING_CAMPAIGN_COUNT; ++type)
            if (Eligible(type))
                result.push_back(type);
        return result;
    }
    std::vector<int> ActiveTypes()
    {
        std::vector<int> result;
        for (int type = 0; type < ADVERTISING_CAMPAIGN_COUNT; ++type)
            if (MarketingGetCampaign(type))
                result.push_back(type);
        return result;
    }
    bool SelectTarget(Draft& draft, const Target& target)
    {
        const auto targets = Targets(draft.type);
        if (std::find(targets.begin(), targets.end(), target) == targets.end())
            return false;
        draft.target = target;
        return true;
    }
    void AdjustWeeks(Draft& draft, int direction)
    {
        draft.weeks = static_cast<int>(std::clamp(static_cast<int64_t>(draft.weeks) + direction, int64_t{ 2 }, int64_t{ 12 }));
    }
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback)
    {
        GameActions::Result invalid;
        invalid.error = GameActions::Status::disallowed;
        if (!Eligible(draft.type))
            invalid.errorMessage = "Campaign unavailable or already active";
        else if (draft.weeks < 2 || draft.weeks > 12)
            invalid.errorMessage = "Choose 2 to 12 weeks";
        else if (NeedsTarget(draft.type))
        {
            const auto targets = Targets(draft.type);
            if (!draft.target || std::find(targets.begin(), targets.end(), *draft.target) == targets.end())
                invalid.errorMessage = "Choose an available target";
        }
        if (!invalid.getErrorMessage().empty())
            return invalid;
        auto action = GameActions::ParkMarketingAction(draft.type, draft.target ? draft.target->item : 0, draft.weeks);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    static std::string ActiveTarget(const MarketingCampaign& campaign)
    {
        if (campaign.type == ADVERTISING_CAMPAIGN_FOOD_OR_DRINK_FREE)
            return TargetName({ static_cast<int>(campaign.shopItemType), false });
        if (campaign.type == ADVERTISING_CAMPAIGN_RIDE || campaign.type == ADVERTISING_CAMPAIGN_RIDE_FREE)
        {
            auto* ride = GetRide(campaign.rideId);
            return ride ? ride->getName() : "Unavailable";
        }
        return getGameState().park.name;
    }
    ManagementPresentation Presentation(
        View view, size_t selected, const std::optional<Draft>& draft, int activeType, const std::vector<int>& types,
        const std::vector<Target>& targets, const std::string& status)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park dashboard / Marketing";
        model.title = "Marketing";
        model.selected = selected;
        model.status = status;
        const auto& park = getGameState().park;
        const bool noMoney = park.flags.has(ParkFlag::noMoney);
        if (view == View::overview)
        {
            model.stats = { { "Active", std::to_string(ActiveTypes().size()) },
                            { "Cash", noMoney ? "Not applicable" : FormatMoney(park.cash) } };
            model.rows = { Types().empty() ? "No campaigns available" : "New campaign...", "Back to dashboard" };
            for (auto type : types)
                if (auto* campaign = MarketingGetCampaign(type))
                    model.rows.push_back(Name(type) + "  " + std::to_string(campaign->weeksLeft) + "w");
                else
                    model.rows.push_back(Name(type) + "  Ended");
            if (model.status.empty() && park.flags.has(ParkFlag::forbidMarketingCampaigns))
                model.status = "Marketing forbidden by authority";
            return model;
        }
        if (view == View::active)
        {
            model.title = Name(activeType);
            if (const auto* campaign = MarketingGetCampaign(activeType))
                model.stats = { { "Target", ActiveTarget(*campaign) },
                                { "Weeks left", std::to_string(campaign->weeksLeft) },
                                { "Status", "Active" } };
            else
                model.stats = { { "Status", "Campaign ended" } };
            model.rows = { "Back to campaigns" };
            return model;
        }
        if (view == View::types)
        {
            model.title = "New campaign";
            for (auto type : types)
                model.rows.push_back(Name(type));
            model.rows.push_back("Cancel");
            return model;
        }
        if (!draft)
        {
            model.status = "Draft unavailable";
            model.rows = { "Back" };
            return model;
        }
        if (view == View::targets)
        {
            model.title = "Choose target";
            for (const auto& target : targets)
                model.rows.push_back(TargetName(target));
            model.rows.push_back("Cancel");
            return model;
        }
        model.title = Name(draft->type);
        const auto query = Action(*draft, false);
        // No-money skips native payment; retain the query result instead of inventing a charge.
        const bool validType = draft->type >= 0 && draft->type < ADVERTISING_CAMPAIGN_COUNT;
        const auto nominal = validType ? AdvertisingCampaignPricePerWeek[draft->type] : kMoney64Undefined;
        const auto cost = query.error == GameActions::Status::ok ? query.cost : kMoney64Undefined;
        if (view == View::editor)
        {
            model.stats = { { "Per week", noMoney ? "Not applicable" : FormatMoney(nominal) },
                            { "Total cost", noMoney ? "Not applicable" : FormatMoney(cost) } };
            if (NeedsTarget(draft->type))
                model.rows.push_back(draft->target ? "Target: " + TargetName(*draft->target) : "Choose target...");
            model.rows.push_back("Duration: " + std::to_string(draft->weeks) + " weeks");
            model.rows.push_back("Review campaign...");
            model.rows.push_back("Cancel");
            model.editing = true;
            model.primaryAction = "Select";
        }
        else
        {
            model.breadcrumb = "Marketing / Confirm campaign";
            model.stats = { { "Target", draft->target ? TargetName(*draft->target) : park.name },
                            { "Duration", std::to_string(draft->weeks) + " weeks" },
                            { "Total cost", noMoney ? "Not applicable" : FormatMoney(cost) },
                            { "Cash after",
                              noMoney ? "Not applicable"
                                  : cost == kMoney64Undefined || park.cash == kMoney64Undefined
                                  ? "Unavailable"
                                  : FormatMoney(park.cash - cost) } };
            model.rows = { "Start campaign", "Cancel" };
            model.confirmation = true;
        }
        if (model.status.empty() && query.error != GameActions::Status::ok)
            model.status = Finances::Feedback(query);
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::MarketingManagement
