// SPDX-License-Identifier: GPL-3.0-only
#include "RideManagement.h"

#include "CustomTracks.h"
#include "Finances.h"
#include "FlatRides.h"
#include "RideAccess.h"

#include <iomanip>
#include <openrct2/GameState.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideSetPriceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/interface/Window.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/ShopItem.h>
#include <openrct2/ride/ted/TrackGroup.h>
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
    void InvalidateAll()
    {
        for (auto& [id, generation] : removedRides)
            ++generation;
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
        return Draft{ target,
                      ride->status,
                      ride->price[0],
                      Park::RidePricesUnlocked(park),
                      park.flags.has(ParkFlag::noMoney),
                      status,
                      ride->price[0],
                      {},
                      0,
                      {},
                      {},
                      false,
                      0 };
    }
    static std::array<uint8_t, 13> Snapshot(const Ride& ride)
    {
        return { static_cast<uint8_t>(ride.mode),
                 ride.minWaitingTime,
                 ride.maxWaitingTime,
                 ride.departFlags,
                 ride.liftHillSpeed,
                 static_cast<uint8_t>(ride.inspectionInterval),
                 ride.proposedNumTrains,
                 ride.proposedNumCarsPerTrain,
                 ride.numTrains,
                 ride.numCarsPerTrain,
                 ride.numStations,
                 ride.numBlockBrakes,
                 ride.numCircuits };
    }
    std::vector<Setting> Settings(const Ride& ride, bool maintenance)
    {
        using GameActions::RideSetSetting;
        std::vector<Setting> result;
        const auto& type = ride.getRideTypeDescriptor();
        if (!ride.getRideEntry())
            return result;
        auto add = [&](RideSetSetting key, const char* name, uint8_t current, int min, int max) {
            Setting setting{ key, name, {}, {}, current };
            for (int value = min; value <= max; ++value)
            {
                setting.values.push_back(value);
                setting.labels.push_back(std::to_string(value));
            }
            result.push_back(std::move(setting));
        };
        if (maintenance)
        {
            if (!type.availableBreakdowns.isEmpty())
            {
                add(RideSetSetting::inspectionInterval, "Inspection interval", static_cast<uint8_t>(ride.inspectionInterval), 0,
                    static_cast<uint8_t>(RideInspection::never));
                auto& setting = result.back();
                for (size_t i = 0; i < setting.labels.size(); ++i)
                    setting.labels[i] = FormatStringID(kRideInspectionIntervalNames[i]);
            }
            return result;
        }
        Setting mode{ RideSetSetting::mode, "Operating mode", {}, {}, static_cast<uint8_t>(ride.mode) };
        const auto modes = ride.getAvailableModes();
        for (int i = 0; i < static_cast<int>(RideMode::count); ++i)
            if (modes.has(static_cast<RideMode>(i)))
            {
                mode.values.push_back(i);
                mode.labels.push_back(FormatStringID(kRideModeNames[i]));
            }
        if (mode.values.size() > 1)
            result.push_back(std::move(mode));
        if (type.flags.has(RtdFlag::hasLoadOptions))
        {
            add(RideSetSetting::minWaitingTime, "Minimum wait / seconds", ride.minWaitingTime, 0, Limits::kMaxWaitingTime);
            add(RideSetSetting::maxWaitingTime, "Maximum wait / seconds", ride.maxWaitingTime, 0, Limits::kMaxWaitingTime);
            for (const auto bit : { RIDE_DEPART_WAIT_FOR_MINIMUM_LENGTH, RIDE_DEPART_WAIT_FOR_MAXIMUM_LENGTH })
            {
                result.push_back(
                    { RideSetSetting::departure,
                      bit == RIDE_DEPART_WAIT_FOR_MINIMUM_LENGTH ? "Use minimum wait" : "Use maximum wait",
                      { static_cast<uint8_t>(ride.departFlags & ~bit), static_cast<uint8_t>(ride.departFlags | bit) },
                      { "Off", "On" },
                      ride.departFlags });
            }
        }
        const bool lift = type.TrackPaintFunctions.Regular.SupportsTrackGroup(TrackGroup::liftHill)
            || (type.flags.has(RtdFlag::hasInvertedVariant)
                && type.InvertedTrackPaintFunctions.Regular.SupportsTrackGroup(TrackGroup::liftHill));
        if (lift)
        {
            add(RideSetSetting::liftHillSpeed, "Lift speed", ride.liftHillSpeed, type.LiftData.minimum_speed,
                type.LiftData.maximum_speed);
            auto& setting = result.back();
            for (size_t i = 0; i < setting.values.size(); ++i)
                setting.labels[i] = FormatStringID(STR_VELOCITY, static_cast<uint16_t>(setting.values[i]));
        }
        return result;
    }
    static Ride VehicleCalculation(const Ride& ride, const Setting* setting = nullptr, uint8_t value = 0)
    {
        // Use the native calculation on a detached copy, never the live ride as query scratch storage.
        Ride preview{};
        preview.id = ride.id;
        preview.type = ride.type;
        preview.subtype = ride.subtype;
        preview.mode = ride.mode;
        std::copy(ride.getStations().begin(), ride.getStations().end(), preview.getStations().begin());
        preview.numStations = ride.numStations;
        preview.numBlockBrakes = ride.numBlockBrakes;
        preview.numTrains = ride.numTrains;
        preview.numCarsPerTrain = ride.numCarsPerTrain;
        preview.proposedNumTrains = ride.proposedNumTrains;
        preview.proposedNumCarsPerTrain = ride.proposedNumCarsPerTrain;
        preview.maxTrains = ride.maxTrains;
        preview.minCarsPerTrain = ride.minCarsPerTrain;
        preview.maxCarsPerTrain = ride.maxCarsPerTrain;
        if (setting && setting->vehicle)
        {
            if (*setting->vehicle == GameActions::RideSetVehicleType::numTrains)
                preview.proposedNumTrains = value;
            else
                preview.proposedNumCarsPerTrain = value;
        }
        preview.updateMaxVehicles();
        return preview;
    }
    std::vector<Setting> VehicleSettings(const Ride& ride)
    {
        std::vector<Setting> result;
        const auto* entry = ride.getRideEntry();
        if (!entry || ride.getRideTypeDescriptor().flags.has(RtdFlag::noVehicles) || ride.numStations == 0)
            return result;
        const auto calculated = VehicleCalculation(ride);
        auto add = [&](GameActions::RideSetVehicleType key, const char* name, uint8_t current, int min, int max, int zero) {
            Setting setting{ GameActions::RideSetSetting::departure, name, {}, {}, current };
            setting.vehicle = key;
            setting.section = View::vehicles;
            for (int value = min; value <= max; ++value)
            {
                setting.values.push_back(value);
                setting.labels.push_back(std::to_string(std::max(0, value - zero)));
            }
            if (setting.values.size() > 1)
                result.push_back(std::move(setting));
        };
        // Mirror native vehicle widget visibility and native station/object limits.
        if (entry->cars_per_flat_ride > 1 || getGameState().cheats.disableTrainLengthLimit)
            add(GameActions::RideSetVehicleType::numTrains, "Trains / vehicles", ride.proposedNumTrains, 1,
                getGameState().cheats.disableTrainLengthLimit ? Limits::kMaxTrainsPerRide : calculated.maxTrains, 0);
        if (entry->zero_cars + 1 < entry->max_cars_in_train || getGameState().cheats.disableTrainLengthLimit)
            add(GameActions::RideSetVehicleType::numCarsPerTrain, "Cars per train", ride.proposedNumCarsPerTrain,
                getGameState().cheats.disableTrainLengthLimit ? 1 : entry->min_cars_in_train,
                getGameState().cheats.disableTrainLengthLimit ? Limits::kMaxCarsPerTrain : entry->max_cars_in_train,
                entry->zero_cars);
        return result;
    }
    std::vector<Setting> DispatchSettings(const Ride& ride)
    {
        std::vector<Setting> result;
        if (!ride.getRideEntry())
            return result;
        const auto& type = ride.getRideTypeDescriptor();
        auto toggle = [&](const char* name, int bit) {
            Setting setting{ GameActions::RideSetSetting::departure,
                             name,
                             { static_cast<uint8_t>(ride.departFlags & ~bit), static_cast<uint8_t>(ride.departFlags | bit) },
                             { "Off", "On" },
                             ride.departFlags };
            setting.section = View::dispatch;
            result.push_back(std::move(setting));
        };
        if (type.flags.has(RtdFlag::hasLoadOptions))
        {
            toggle("Wait for load", RIDE_DEPART_WAIT_FOR_LOAD);
            Setting load{ GameActions::RideSetSetting::departure, "Required load", {}, {}, ride.departFlags };
            constexpr StringId names[] = { STR_QUARTER_LOAD, STR_HALF_LOAD, STR_THREE_QUARTER_LOAD, STR_FULL_LOAD,
                                           STR_ANY_LOAD };
            static_assert(std::size(names) == WAIT_FOR_LOAD_COUNT);
            for (int i = 0; i < WAIT_FOR_LOAD_COUNT; ++i)
            {
                load.values.push_back((ride.departFlags & ~RIDE_DEPART_WAIT_FOR_LOAD_MASK) | i);
                load.labels.push_back(FormatStringID(names[i]));
            }
            load.section = View::dispatch;
            result.push_back(std::move(load));
        }
        if (type.flags.has(RtdFlag::hasLeaveWhenAnotherVehicleArrivesAtStation) && ride.numTrains > 1
            && !ride.isBlockSectioned())
            toggle("Leave when another arrives", RIDE_DEPART_LEAVE_WHEN_ANOTHER_ARRIVES);
        if (type.flags.has(RtdFlag::canSynchroniseWithAdjacentStations))
            toggle("Synchronise stations", RIDE_DEPART_SYNCHRONISE_WITH_ADJACENT_STATIONS);
        return result;
    }
    static std::vector<Setting> SectionSettings(const Ride& ride, View section)
    {
        return section == View::vehicles ? VehicleSettings(ride)
            : section == View::dispatch  ? DispatchSettings(ride)
                                         : Settings(ride, section == View::maintenance);
    }
    std::optional<Draft> BeginSetting(const Target& target, const Setting& selected)
    {
        const auto* ride = Resolve(target);
        if (!ride)
            return {};
        const auto section = selected.vehicle                                 ? View::vehicles
            : selected.section == View::dispatch                              ? View::dispatch
            : selected.key == GameActions::RideSetSetting::inspectionInterval ? View::maintenance
                                                                              : View::operations;
        auto settings = SectionSettings(*ride, section);
        const auto found = std::find_if(settings.begin(), settings.end(), [&](const auto& setting) {
            return setting.key == selected.key && setting.vehicle == selected.vehicle && setting.name == selected.name;
        });
        if (found == settings.end())
            return {};
        const auto& park = getGameState().park;
        Draft draft{ target,
                     ride->status,
                     ride->price[0],
                     Park::RidePricesUnlocked(park),
                     park.flags.has(ParkFlag::noMoney),
                     {},
                     ride->price[0],
                     {},
                     0,
                     {},
                     {},
                     false,
                     0 };
        draft.setting = *found;
        draft.setting->section = section;
        draft.value = found->current;
        if (!found->values.empty() && std::find(found->values.begin(), found->values.end(), draft.value) == found->values.end())
            draft.value = found->values.front(); // Honest unavailable current value; draft starts at a native legal value.
        draft.baselineSettings = Snapshot(*ride);
        if (found->vehicle)
        {
            const auto preview = VehicleCalculation(*ride, &*found, draft.value);
            draft.effectiveVehicles = { preview.numTrains, preview.numCarsPerTrain };
        }
        return draft;
    }
    std::optional<Draft> BeginDemolition(const Target& target)
    {
        const auto* ride = Resolve(target);
        if (!ride)
            return {};
        const auto& park = getGameState().park;
        Draft draft{ target,
                     ride->status,
                     ride->price[0],
                     Park::RidePricesUnlocked(park),
                     park.flags.has(ParkFlag::noMoney),
                     {},
                     ride->price[0],
                     {},
                     0,
                     {},
                     {},
                     false,
                     0 };
        draft.demolition = true;
        draft.refund = RideGetRefundPrice(*ride);
        return draft;
    }
    static std::string SettingValue(const Setting& setting, uint8_t value)
    {
        auto found = std::find(setting.values.begin(), setting.values.end(), value);
        return found == setting.values.end() ? "Unavailable" : setting.labels[found - setting.values.begin()];
    }
    void AdjustPrice(Draft& draft, int direction)
    {
        if (draft.setting)
        {
            const auto& values = draft.setting->values;
            const auto found = std::find(values.begin(), values.end(), draft.value);
            if (found != values.end() && !values.empty())
                draft.value = values[std::clamp<int>(
                    found - values.begin()
                        + (direction > 0       ? 1
                               : direction < 0 ? -1
                                               : 0),
                    0, values.size() - 1)];
            if (draft.setting->vehicle)
                if (const auto* ride = Resolve(draft.target))
                {
                    const auto preview = VehicleCalculation(*ride, &*draft.setting, draft.value);
                    draft.effectiveVehicles = { preview.numTrains, preview.numCarsPerTrain };
                }
            return;
        }
        if (!draft.status && !draft.demolition)
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
        if (draft.demolition)
        {
            // Native demolition Query validates rules but its cost is populated only by Execute.
            // Match the desktop demolition prompt's native TrackRemove-query refund calculator.
            if (RideGetRefundPrice(*ride) != draft.refund)
                return Invalid("Ride layout/refund changed; reopen review");
            GameActions::RideDemolishAction action(draft.target.id, GameActions::RideModifyType::demolish);
            action.SetCallback(std::move(callback));
            if (execute)
                return GameActions::Execute(&action, getGameState());
            auto result = GameActions::Query(&action, getGameState());
            if (result.error == GameActions::Status::ok)
                result.cost = getGameState().park.flags.has(ParkFlag::noMoney) ? 0 : draft.refund;
            return result;
        }
        if (draft.setting)
        {
            if (Snapshot(*ride) != draft.baselineSettings)
                return Invalid("Operations changed; reopen review");
            const auto current = SectionSettings(*ride, draft.setting->section);
            const auto found = std::find_if(current.begin(), current.end(), [&](const auto& setting) {
                return setting.key == draft.setting->key && setting.vehicle == draft.setting->vehicle
                    && setting.name == draft.setting->name;
            });
            if (found == current.end() || found->values != draft.setting->values
                || std::find(found->values.begin(), found->values.end(), draft.value) == found->values.end())
                return Invalid("Setting unavailable or outside native limits");
            if (draft.value == found->current)
                return Invalid("Setting unchanged");
            if (found->vehicle)
            {
                const auto preview = VehicleCalculation(*ride, &*found, draft.value);
                if (draft.effectiveVehicles != std::array<uint8_t, 2>{ preview.numTrains, preview.numCarsPerTrain })
                    return Invalid("Track/vehicle limits changed; reopen draft");
                GameActions::RideSetVehicleAction action(draft.target.id, *found->vehicle, draft.value);
                action.SetCallback(std::move(callback));
                return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
            }
            GameActions::RideSetSettingAction action(draft.target.id, found->key, draft.value);
            action.SetCallback(std::move(callback));
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
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
            model.stats.emplace_back("Queue", std::to_string(ride->getTotalQueueLength()));
            model.rows = { "Locate",
                           "Statistics...",
                           "Open...",
                           "Close...",
                           ride->supportsStatus(RideStatus::testing) ? "Test..." : "Test unavailable",
                           PricePermitted(*ride) ? "Admission price..." : "Price unavailable",
                           "Back to roster" };
            if (FlatRides::CanResume(*target))
                model.rows.push_back("Finish entrances/exits...");
            if (RideAccess::Supported(*target))
                model.rows.push_back("Access / paths...");
            if (CustomTracks::CanResume(*target))
                model.rows.push_back("Continue track...");
            model.rows.push_back("Vehicles...");
            model.rows.push_back("Dispatch...");
            model.rows.push_back("Operations...");
            model.rows.push_back("Maintenance...");
            model.rows.push_back("Demolish...");
        }
        else if (view == View::operations || view == View::maintenance || view == View::vehicles || view == View::dispatch)
        {
            model.breadcrumb = view == View::maintenance ? "Rides / Maintenance" : "Rides / Operations";
            if (view == View::maintenance)
            {
                model.stats = { { "Reliability", std::to_string(ride->reliabilityPercentage) + "%" },
                                { "Downtime", std::to_string(ride->downtime) + "%" },
                                { "Last inspection",
                                  ride->lastInspection <= 240 ? std::to_string(ride->lastInspection) + " min ago"
                                                              : "Over 4 hours" },
                                { "Inspection", ride->flags.has(RideFlag::dueInspection) ? "Due" : "Not due" } };
                model.stats.emplace_back("Age / months", std::to_string(ride->getAge()));
                if (ride->getRideTypeDescriptor().availableBreakdowns.isEmpty())
                    model.stats = { { "Maintenance", "Not applicable" }, { "Age / months", std::to_string(ride->getAge()) } };
            }
            else
                model.stats = { { "Mode", FormatStringID(kRideModeNames[static_cast<uint8_t>(ride->mode)]) },
                                { "Trains / cars",
                                  std::to_string(ride->numTrains) + " / " + std::to_string(ride->numCarsPerTrain) } };
            if (view == View::vehicles)
            {
                model.breadcrumb = "Rides / Vehicles";
                const auto* entry = ride->getRideEntry();
                const int zero = entry ? entry->zero_cars : 0;
                model.stats = { { "Current vehicles", std::to_string(ride->numTrains) },
                                { "Current cars / train", std::to_string(std::max(0, ride->numCarsPerTrain - zero)) },
                                { "Requested vehicles", std::to_string(ride->proposedNumTrains) },
                                { "Requested cars / train",
                                  std::to_string(std::max(0, ride->proposedNumCarsPerTrain - zero)) } };
                model.status = "Native limits / close before changing";
            }
            if (view == View::dispatch)
            {
                model.breadcrumb = "Rides / Dispatch";
                model.stats = { { "Mode", FormatStringID(kRideModeNames[static_cast<uint8_t>(ride->mode)]) } };
            }
            for (const auto& setting : SectionSettings(*ride, view))
                model.rows.push_back(setting.name + ": " + SettingValue(setting, setting.current));
            model.rows.push_back("Back to ride");
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
            if (draft->demolition)
            {
                model.breadcrumb = "Rides / Demolish permanently";
                model.stats.emplace_back("Removal", "Ride, track, entrances/exits");
                model.stats.emplace_back(
                    "Refund",
                    draft->noMoney                                 ? "Not applicable"
                        : queried.error == GameActions::Status::ok ? FormatMoney(-queried.cost)
                                                                   : "Unavailable");
            }
            else if (draft->setting)
            {
                model.breadcrumb = "Rides / " + draft->setting->name;
                model.stats.emplace_back("Current", SettingValue(*draft->setting, draft->setting->current));
                model.stats.emplace_back("Draft", SettingValue(*draft->setting, draft->value));
                if (draft->setting->vehicle)
                {
                    const auto* entry = ride->getRideEntry();
                    model.stats.emplace_back(
                        "Effective trains / cars",
                        std::to_string(draft->effectiveVehicles[0]) + " / "
                            + std::to_string(std::max(0, draft->effectiveVehicles[1] - (entry ? entry->zero_cars : 0))));
                    model.status = "Native limits may clamp requested counts";
                }
                if (queried.error == GameActions::Status::ok
                    && draft->setting->key == GameActions::RideSetSetting::minWaitingTime)
                    model.status = "Native maximum wait increases if needed";
                if (queried.error == GameActions::Status::ok
                    && draft->setting->key == GameActions::RideSetSetting::maxWaitingTime)
                    model.status = "Native minimum wait decreases if needed";
            }
            else if (draft->status)
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
            if (!draft->demolition)
                model.stats.emplace_back(
                    "Action cost", queried.error == GameActions::Status::ok ? FormatMoney(queried.cost) : "Unavailable");
            model.rows = view == View::price ? std::vector<std::string>{ "Decrease", "Increase", "Review...", "Cancel" }
                                             : std::vector<std::string>{ "Confirm", "Cancel" };
            if (queried.error != GameActions::Status::ok && feedback.empty())
                model.status = Finances::Feedback(queried);
        }
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::RideManagement
