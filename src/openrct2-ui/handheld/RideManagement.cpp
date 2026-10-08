// SPDX-License-Identifier: GPL-3.0-only
#include "RideManagement.h"

#include "Finances.h"

#include <iomanip>
#include <openrct2/GameState.h>
#include <openrct2/actions/ride/RideSetPriceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/interface/Window.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/ShopItem.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/Park.h>
#include <sstream>
#include <unordered_map>

namespace OpenRCT2::Ui::Handheld::RideManagement
{
    static std::unordered_map<uint16_t, uint64_t> removedRides;
    void Invalidate(RideId id)
    {
        if (auto it = removedRides.find(id.ToUnderlying()); it != removedRides.end())
            ++it->second;
    }
    std::optional<Target> Identify(RideId id)
    {
        const auto* ride = GetRide(id);
        if (!ride || ride->type == kRideTypeNull || ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility))
            return {};
        return Target{ id, ride->type, ride->subtype, ride->buildDate, ride->overallView, removedRides[id.ToUnderlying()] };
    }
    const Ride* Resolve(const Target& target)
    {
        const auto now = Identify(target.id);
        return now && *now == target ? GetRide(target.id) : nullptr;
    }
    std::vector<Target> Roster()
    {
        std::vector<Target> result;
        for (const auto& ride : RideManager(getGameState()))
            if (auto target = Identify(ride.id))
                result.push_back(*target);
        return result;
    }
    std::optional<CoordsXYZ> Location(const Target& target)
    {
        const auto* ride = Resolve(target);
        if (!ride || ride->overallView.isNull() || !MapIsLocationValid(ride->overallView))
            return {};
        // Resolve the native overall-view tile; scroll the main viewport without opening desktop windows.
        const auto xy = ride->overallView + CoordsXY{ 16, 16 };
        return CoordsXYZ{ xy, TileElementHeight(xy) };
    }
    bool Locate(const Target& target)
    {
        const auto location = Location(target);
        auto* main = WindowGetMain();
        if (!main || !location)
            return false;
        WindowScrollToLocation(*main, *location);
        return true;
    }
    bool PricePermitted(const Ride& ride)
    {
        const auto* entry = ride.getRideEntry();
        const auto& park = getGameState().park;
        // Mirror desktop admission permission; product/photo prices remain in shop management.
        return entry && entry->shop_item[0] == ShopItem::none && !park.flags.has(ParkFlag::noMoney)
            && Park::RidePricesUnlocked(park);
    }
    std::optional<Draft> Begin(const Target& target, std::optional<RideStatus> status)
    {
        const auto* ride = Resolve(target);
        if (!ride || (status && (*status == RideStatus::simulating || !ride->supportsStatus(*status)))
            || (!status && !PricePermitted(*ride)))
            return {};
        const auto& park = getGameState().park;
        return Draft{ target, ride->status,  ride->price[0], Park::RidePricesUnlocked(park), park.flags.has(ParkFlag::noMoney),
                      status, ride->price[0] };
    }
    void AdjustPrice(Draft& draft, int direction)
    {
        if (!draft.status)
            draft.price = std::clamp<money64>(
                draft.price
                    + (direction > 0       ? 1
                           : direction < 0 ? -1
                                           : 0),
                kRideMinPrice, kRideMaxPrice);
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
        const auto* ride = Resolve(draft.target);
        if (!ride)
            return Invalid("Ride unavailable; return to roster");
        if (ride->status != draft.baselineStatus || ride->price[0] != draft.baselinePrice
            || Park::RidePricesUnlocked(getGameState().park) != draft.pricesUnlocked
            || getGameState().park.flags.has(ParkFlag::noMoney) != draft.noMoney)
            return Invalid("Ride/settings changed; reopen review");
        if (draft.status)
        {
            if (*draft.status != RideStatus::closed && *draft.status != RideStatus::open
                && *draft.status != RideStatus::testing)
                return Invalid("Unsupported ride status");
            if (!ride->supportsStatus(*draft.status))
                return Invalid("Testing unavailable for this ride");
            if (*draft.status == ride->status)
                return Invalid("Ride already has this status");
            GameActions::RideSetStatusAction action(draft.target.id, *draft.status);
            action.SetCallback(std::move(callback));
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        if (!PricePermitted(*ride))
            return Invalid("Admission pricing unavailable");
        if (draft.price < kRideMinPrice || draft.price > kRideMaxPrice)
            return Invalid("Admission price outside native limits");
        if (draft.price == ride->price[0])
            return Invalid("Price unchanged");
        GameActions::RideSetPriceAction action(draft.target.id, draft.price, true);
        action.SetCallback(std::move(callback));
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    static std::string Status(const Ride& ride)
    {
        if (ride.flags.has(RideFlag::crashed))
            return "Crashed";
        if (ride.flags.has(RideFlag::brokenDown))
            return "Broken down";
        return ride.status == RideStatus::open      ? "Open"
            : ride.status == RideStatus::testing    ? "Testing"
            : ride.status == RideStatus::simulating ? "Simulating"
                                                    : "Closed";
    }
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster, const std::optional<Target>& target,
        const std::optional<Draft>& draft, const std::string& feedback)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park dashboard / Rides";
        model.title = "Rides";
        model.selected = selected;
        model.status = feedback;
        if (view == View::roster)
        {
            for (const auto& entry : roster)
            {
                const auto* ride = Resolve(entry);
                model.rows.push_back(ride ? ride->getName() + " / " + Status(*ride) : "Ride unavailable");
            }
            model.rows.push_back("Back to dashboard");
            if (roster.empty())
                model.status = "No non-shop rides in this park";
            return model;
        }
        const auto* ride = target ? Resolve(*target) : nullptr;
        if (!ride)
        {
            model.status = "Ride unavailable; return to roster";
            model.rows = { "Back to roster" };
            return model;
        }
        model.title = ride->getName();
        if (view == View::inspection)
        {
            model.stats = { { "Status", Status(*ride) },
                            { "Admission",
                              getGameState().park.flags.has(ParkFlag::noMoney) ? "Not applicable"
                                                                               : FormatMoney(RideGetPrice(*ride)) } };
            model.rows = { "Locate",
                           "Statistics...",
                           "Open...",
                           "Close...",
                           ride->supportsStatus(RideStatus::testing) ? "Test..." : "Test unavailable",
                           PricePermitted(*ride) ? "Admission price..." : "Price unavailable",
                           "Back to roster" };
        }
        else if (view == View::statistics)
        {
            auto rating = [ride](uint16_t value) {
                if (!ride->flags.has(RideFlag::tested) || !RideHasRatings(*ride))
                    return std::string("Unavailable");
                std::ostringstream text;
                text << value / 100 << '.' << std::setw(2) << std::setfill('0') << value % 100;
                return text.str();
            };
            auto money = [](money64 value) {
                return getGameState().park.flags.has(ParkFlag::noMoney) ? std::string("Not applicable")
                    : value == kMoney64Undefined                        ? std::string("Unavailable")
                                                                        : FormatMoney(value);
            };
            model.stats = { { "Excitement", rating(ride->ratings.excitement) },
                            { "Intensity", rating(ride->ratings.intensity) },
                            { "Nausea", rating(ride->ratings.nausea) },
                            { "Reliability", std::to_string(ride->reliabilityPercentage) + "%" },
                            { "Customers", std::to_string(ride->totalCustomers) },
                            { "Income / hour", money(ride->incomePerHour) },
                            { "Profit / hour", money(ride->profit) } };
            model.rows = { "Back to ride" };
        }
        else if (draft)
        {
            model.editing = view == View::price;
            if (model.editing)
                model.primaryAction = selected < 2 ? "Adjust" : selected == 2 ? "Review" : "Cancel";
            model.confirmation = view == View::confirmation;
            if (model.confirmation)
                model.primaryAction = selected == 1 ? "Cancel" : "Confirm";
            model.breadcrumb = view == View::price ? "Rides / Admission" : "Rides / Review";
            const auto queried = Action(*draft, false);
            model.stats = { { "Current status", Status(*ride) } };
            if (draft->status)
                model.stats.emplace_back(
                    "Requested",
                    *draft->status == RideStatus::open         ? "Open"
                        : *draft->status == RideStatus::closed ? "Closed"
                                                               : "Testing");
            else
            {
                model.stats.emplace_back("Current price", FormatMoney(ride->price[0]));
                model.stats.emplace_back("Draft price", FormatMoney(draft->price));
            }
            model.stats.emplace_back(
                "Action cost", queried.error == GameActions::Status::ok ? FormatMoney(queried.cost) : "Unavailable");
            model.rows = view == View::price ? std::vector<std::string>{ "Decrease", "Increase", "Review...", "Cancel" }
                                             : std::vector<std::string>{ "Confirm", "Cancel" };
            if (model.status.empty() && queried.error != GameActions::Status::ok)
                model.status = Finances::Feedback(queried);
        }
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::RideManagement
