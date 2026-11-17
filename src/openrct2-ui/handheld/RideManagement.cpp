// SPDX-License-Identifier: GPL-3.0-only
#include "RideManagement.h"
#include "RideFollow.h"

#include "../interface/Dropdown.h"
#include "CustomTracks.h"
#include "Finances.h"
#include "FlatRides.h"
#include "Mazes.h"
#include "RideAccess.h"
#include "TextEntry.h"
#include "TrackEditing.h"

#include <iomanip>
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/ride/RideDemolishAction.h>
#include <openrct2/actions/ride/RideSetColourSchemeAction.h>
#include <openrct2/actions/ride/RideSetNameAction.h>
#include <openrct2/actions/ride/RideSetPriceAction.h>
#include <openrct2/actions/ride/RideSetStatusAction.h>
#include <openrct2/core/UnitConversion.h>
#include <openrct2/entity/Staff.h>
#include <openrct2/interface/Window.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/MusicObject.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/StationObject.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/RideManager.hpp>
#include <openrct2/ride/ShopItem.h>
#include <openrct2/ride/ted/TrackGroup.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/tile_element/TrackElement.h>
#include <sstream>
#include <unordered_map>

namespace OpenRCT2::Ui::Handheld::RideManagement
{
    static std::unordered_map<uint16_t, uint64_t> removedRides;
    static GameActions::Result Invalid(const char* text);
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
    std::optional<Target> Identify(RideId id, bool includeShops)
    {
        const auto* ride = GetRide(id);
        if (!ride || ride->type == kRideTypeNull
            || (!includeShops && ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility)))
            return {};
        return Target{ id,
                       ride->type,
                       ride->subtype,
                       ride->buildDate,
                       ride->overallView,
                       removedRides[id.ToUnderlying()],
                       ride->getRideTypeDescriptor().flags.has(RtdFlag::isShopOrFacility) };
    }
    const Ride* Resolve(const Target& target)
    {
        const auto now = Identify(target.id, target.shop);
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
    static std::array<uint16_t, 17> Snapshot(const Ride& ride)
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
                 ride.numCircuits,
                 ride.operationOption,
                 static_cast<uint16_t>(ride.flags.has(RideFlag::reversedTrains)),
                 ride.music,
                 static_cast<uint16_t>(ride.flags.has(RideFlag::music)) };
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
        // Mirror the desktop operating-page semantics; never expose the union byte as a generic option.
        if (ride.getAvailableModes().has(ride.mode))
        {
            StringId caption = kStringIdEmpty;
            StringId format = STR_COMMA16;
            int multiplier = type.OperatingSettings.OperatingSettingMultiplier;
            bool velocity = false;
            switch (ride.mode)
            {
                case RideMode::poweredLaunchPassthrough:
                case RideMode::poweredLaunch:
                case RideMode::upwardLaunch:
                case RideMode::poweredLaunchBlockSectioned:
                    caption = STR_LAUNCH_SPEED;
                    velocity = true;
                    break;
                case RideMode::stationToStation:
                    caption = STR_SPEED;
                    velocity = true;
                    break;
                case RideMode::race:
                    caption = STR_NUMBER_OF_LAPS;
                    multiplier = 1;
                    break;
                case RideMode::dodgems:
                    caption = STR_TIME_LIMIT;
                    format = STR_RIDE_TIME_ENTRY;
                    break;
                case RideMode::swing:
                    caption = STR_NUMBER_OF_SWINGS;
                    break;
                case RideMode::rotation:
                case RideMode::forwardRotation:
                case RideMode::backwardRotation:
                    caption = STR_NUMBER_OF_ROTATIONS;
                    break;
                case RideMode::shopStall:
                    break;
                default:
                    if (type.flags.has(RtdFlag::noVehicles))
                        caption = STR_MAX_PEOPLE_ON_RIDE;
                    break;
            }
            const auto& options = type.OperatingSettings;
            const bool unlimited = getGameState().cheats.unlockOperatingLimits;
            const int minimum = unlimited ? 0 : options.MinValue;
            const int maximum = unlimited ? Limits::kCheatsMaxOperatingLimit : options.MaxValue;
            if (caption != kStringIdEmpty && maximum > minimum)
            {
                auto name = FmtString(FormatStringID(caption)).WithoutFormatTokens();
                while (!name.empty() && (name.back() == ':' || name.back() == ' '))
                    name.pop_back();
                add(RideSetSetting::operation, name.c_str(), ride.operationOption, minimum, maximum);
                auto& setting = result.back();
                setting.step = ride.mode == RideMode::dodgems ? 10 : 1;
                for (size_t i = 0; i < setting.values.size(); ++i)
                    setting.labels[i] = FmtString(
                                            FormatStringID(
                                                velocity ? static_cast<StringId>(STR_VELOCITY) : format,
                                                static_cast<uint16_t>(
                                                    velocity ? (setting.values[i] * 9) / 4 : setting.values[i] * multiplier)))
                                            .WithoutFormatTokens();
            }
            if (ride.canHaveMultipleCircuits() && !ride.flags.has(RideFlag::cableLift))
                add(RideSetSetting::numCircuits, "Number of circuits", ride.numCircuits, 1,
                    unlimited ? 255 : Limits::kMaxCircuitsPerRide);
        }
        return result;
    }
    static Ride VehicleCalculation(const Ride& ride, const Setting* setting = nullptr, uint16_t value = 0)
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
            else if (*setting->vehicle == GameActions::RideSetVehicleType::numCarsPerTrain)
                preview.proposedNumCarsPerTrain = value;
            else if (*setting->vehicle == GameActions::RideSetVehicleType::rideEntry)
            {
                preview.subtype = value;
                if (const auto* entry = GetRideEntryByIndex(value); entry && !getGameState().cheats.disableTrainLengthLimit)
                    preview.proposedNumCarsPerTrain = std::clamp(
                        preview.proposedNumCarsPerTrain, entry->min_cars_in_train, entry->max_cars_in_train);
            }
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
        Setting objects{ GameActions::RideSetSetting::departure, "Vehicle type", {}, {}, ride.subtype };
        objects.vehicle = GameActions::RideSetVehicleType::rideEntry;
        objects.section = View::vehicles;
        const auto& state = getGameState();
        const auto& descriptor = ride.getRideTypeDescriptor();
        const bool expanded = state.cheats.showVehiclesFromOtherTrackTypes && !descriptor.flags.has(RtdFlag::isFlatRide)
            && descriptor.specialType != RtdSpecialType::maze && descriptor.specialType != RtdSpecialType::miniGolf;
        std::vector<std::pair<std::string, ObjectEntryIndex>> entries;
        for (ride_type_t type = 0; type < RIDE_TYPE_COUNT; ++type)
        {
            if (!expanded && type != ride.type)
                continue;
            const auto& candidate = GetRideTypeDescriptor(type);
            if (expanded
                && (candidate.flags.has(RtdFlag::isFlatRide) || candidate.specialType == RtdSpecialType::maze
                    || candidate.specialType == RtdSpecialType::miniGolf))
                continue;
            for (auto index : GetContext()->GetObjectManager().GetAllRideEntries(type))
                if (const auto* object = GetRideEntryByIndex(index);
                    object && (RideEntryIsInvented(index) || state.cheats.ignoreResearchStatus))
                    entries.emplace_back(FmtString(FormatStringID(object->naming.Name)).WithoutFormatTokens(), index);
        }
        std::sort(entries.begin(), entries.end());
        for (const auto& [name, index] : entries)
        {
            if (std::find(objects.values.begin(), objects.values.end(), index) != objects.values.end())
                continue;
            objects.values.push_back(index);
            objects.labels.push_back(name);
        }
        if (objects.values.size() > 1)
            result.push_back(std::move(objects));
        if ((descriptor.flags.has(RtdFlag::allowReversedTrains) && !entry->flags.has(RideEntryFlag::noReverseOption))
            || (state.cheats.disableTrainLengthLimit && !descriptor.flags.has(RtdFlag::isFlatRide)))
        {
            Setting reversed{ GameActions::RideSetSetting::departure,
                              "Reversed trains",
                              { 0, 1 },
                              { "Off", "On" },
                              static_cast<uint16_t>(ride.flags.has(RideFlag::reversedTrains)) };
            reversed.vehicle = GameActions::RideSetVehicleType::trainsReversed;
            reversed.section = View::vehicles;
            result.push_back(std::move(reversed));
        }
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
    std::vector<Setting> MusicSettings(const Ride& ride)
    {
        using GameActions::RideSetSetting;
        const auto& type = ride.getRideTypeDescriptor();
        if (!ride.getRideEntry() || !type.flags.has(RtdFlag::allowMusic))
            return {};
        Setting enabled{ RideSetSetting::music,
                         "Play music",
                         { 0, 1 },
                         { "Off", "On" },
                         static_cast<uint16_t>(ride.flags.has(RideFlag::music)) };
        enabled.section = View::music;
        std::vector<Setting> result{ enabled };
        Setting style{ RideSetSetting::musicType, "Music style", {}, {}, ride.music };
        style.section = View::music;
        auto& manager = GetContext()->GetObjectManager();
        for (ObjectEntryIndex i = 0; i < kMaxMusicObjects; ++i)
        {
            auto* object = manager.GetLoadedObject<MusicObject>(i);
            if (!object)
                continue;
            const auto original = object->GetOriginalStyleId();
            if (original && (*original == MUSIC_STYLE_CUSTOM_MUSIC_1 || *original == MUSIC_STYLE_CUSTOM_MUSIC_2)
                && (object->GetTrackCount() == 0 || !object->GetTrack(0)->Asset.IsAvailable()))
                continue;
            if (!getGameState().cheats.unlockOperatingLimits
                && !object->SupportsRideType(ride.type, type.flags.has(RtdFlag::requireExplicitListingInMusicObjects)))
                continue;
            style.values.push_back(i);
            style.labels.push_back(FmtString(FormatStringID(object->NameStringId)).WithoutFormatTokens());
        }
        if (!style.values.empty())
            result.push_back(std::move(style));
        return result;
    }
    static std::vector<uint16_t> AppearanceSnapshot(const Ride& ride)
    {
        std::vector<uint16_t> snapshot{ ride.entranceStyle, static_cast<uint16_t>(ride.vehicleColourSettings) };
        for (const auto& colour : ride.trackColours)
        {
            snapshot.push_back(static_cast<uint16_t>(colour.main));
            snapshot.push_back(static_cast<uint16_t>(colour.additional));
            snapshot.push_back(static_cast<uint16_t>(colour.supports));
        }
        for (const auto& colour : ride.vehicleColours)
        {
            snapshot.push_back(static_cast<uint16_t>(colour.Body));
            snapshot.push_back(static_cast<uint16_t>(colour.Trim));
            snapshot.push_back(static_cast<uint16_t>(colour.Tertiary));
        }
        return snapshot;
    }
    std::vector<Setting> AppearanceSettings(const Ride& ride)
    {
        using A = GameActions::RideSetAppearanceType;
        const auto* entry = ride.getRideEntry();
        const auto& type = ride.getRideTypeDescriptor();
        if (!entry || entry->flags.has(RideEntryFlag::disableColourTab))
            return {};
        std::vector<Setting> result;
        auto add = [&](A key, std::string name, uint16_t current, uint32_t index) -> Setting& {
            Setting setting{ GameActions::RideSetSetting::mode, std::move(name), {}, {}, current };
            setting.section = View::appearance;
            setting.appearance = key;
            setting.index = index;
            result.push_back(std::move(setting));
            return result.back();
        };
        auto colour = [&](A key, std::string name, Drawing::Colour current, uint32_t index) {
            auto& setting = add(key, std::move(name), static_cast<uint16_t>(current), index);
            const auto colourCount = getGameState().cheats.allowSpecialColourSchemes ? Drawing::kColourNumTotal
                                                                                     : Drawing::kColourNumNormal;
            for (uint8_t i = 0; i < colourCount; ++i)
            {
                setting.values.push_back(static_cast<uint16_t>(Windows::ColourDropDownIndexToColour(i)));
                setting.labels.push_back(
                    FmtString(FormatStringID(Windows::ColourDropDownIndexToName(i))).WithoutFormatTokens());
            }
        };
        const auto* station = ride.getStationObject();
        const bool main = type.flags.has(RtdFlag::hasTrackColourMain)
            || (station && station->Flags.has(StationObjectFlag::hasPrimaryColour));
        const bool additional = type.flags.has(RtdFlag::hasTrackColourAdditional)
            || (station && station->Flags.has(StationObjectFlag::hasSecondaryColour));
        const bool supports = type.flags.has(RtdFlag::hasTrackColourSupports);
        const int schemes = type.flags.has(RtdFlag::supportsMultipleColourSchemes) ? kNumRideColourSchemes : 1;
        for (int i = 0; i < schemes; ++i)
        {
            const auto prefix = schemes > 1 ? "Track " + std::to_string(i + 1) + " / " : "Entrance / ";
            if (main)
                colour(A::trackColourMain, prefix + "Main", ride.trackColours[i].main, i);
            if (additional)
                colour(A::trackColourAdditional, prefix + "Additional", ride.trackColours[i].additional, i);
            if (supports)
            {
                if (type.specialType == RtdSpecialType::maze)
                {
                    auto& style = add(A::mazeStyle, "Maze style", static_cast<uint16_t>(ride.trackColours[i].supports), i);
                    style.values = { 0, 1, 2, 3 };
                    style.labels = { "Brick walls", "Hedges", "Ice blocks", "Wooden fences" };
                }
                else
                    colour(A::trackColourSupports, prefix + "Supports", ride.trackColours[i].supports, i);
            }
        }
        if (type.flags.has(RtdFlag::hasEntranceAndExit))
        {
            auto& style = add(A::entranceStyle, "Entrance style", ride.entranceStyle, 0);
            auto& manager = GetContext()->GetObjectManager();
            for (ObjectEntryIndex i = 0; i < kMaxStationObjects; ++i)
                if (const auto* object = manager.GetLoadedObject<StationObject>(i))
                {
                    style.values.push_back(i);
                    style.labels.push_back(FmtString(FormatStringID(object->NameStringId)).WithoutFormatTokens());
                }
            if (style.values.empty())
                result.pop_back();
        }
        if (!type.flags.has(RtdFlag::noVehicles) && type.flags.has(RtdFlag::hasVehicleColours))
        {
            bool body = false, trim = false, tertiary = false;
            std::vector<uint32_t> visible;
            for (int i = 0; i < ride.numCarsPerTrain; ++i)
            {
                const auto& car = entry->Cars[RideEntryGetVehicleAtPosition(ride.subtype, ride.numCarsPerTrain, i)];
                body |= car.flags.has(CarEntryFlag::enableBodyColour);
                trim |= car.flags.has(CarEntryFlag::enableTrimColour);
                tertiary |= car.flags.has(CarEntryFlag::enableTertiaryColour);
                if (car.isVisible())
                    visible.push_back(i);
            }
            if ((body || trim || tertiary) && !type.flags.has(RtdFlag::vehicleIsIntegral)
                && (ride.numCarsPerTrain | ride.numTrains) > 1)
            {
                auto& scheme = add(
                    A::vehicleColourScheme, "Vehicle colour scheme", static_cast<uint16_t>(ride.vehicleColourSettings), 0);
                scheme.values = { 0, 1 };
                scheme.labels = { "Same colours", "Per train" };
                if (visible.size() > 1)
                {
                    scheme.values.push_back(2);
                    scheme.labels.push_back("Per car");
                }
            }
            std::vector<uint32_t> indices{ 0 };
            if (ride.vehicleColourSettings == VehicleColourSettings::perTrain)
            {
                indices.clear();
                for (uint32_t i = 0; i < ride.numTrains && i < std::size(ride.vehicleColours); ++i)
                    indices.push_back(i);
            }
            else if (ride.vehicleColourSettings == VehicleColourSettings::perCar)
                indices = visible;
            for (size_t n = 0; n < indices.size(); ++n)
            {
                const auto i = indices[n];
                if (i >= std::size(ride.vehicleColours))
                    continue;
                const auto prefix = ride.vehicleColourSettings == VehicleColourSettings::same
                    ? std::string("Vehicle / ")
                    : (ride.vehicleColourSettings == VehicleColourSettings::perTrain ? "Train " : "Car ")
                        + std::to_string(n + 1) + " / ";
                if (body)
                    colour(A::vehicleColourBody, prefix + "Body", ride.vehicleColours[i].Body, i);
                if (trim)
                    colour(A::vehicleColourTrim, prefix + "Trim", ride.vehicleColours[i].Trim, i);
                if (tertiary)
                    colour(A::vehicleColourTertiary, prefix + "Tertiary", ride.vehicleColours[i].Tertiary, i);
            }
        }
        return result;
    }
    static std::vector<Setting> SectionSettings(const Ride& ride, View section)
    {
        return section == View::appearance ? AppearanceSettings(ride)
            : section == View::music       ? MusicSettings(ride)
            : section == View::vehicles    ? VehicleSettings(ride)
            : section == View::dispatch    ? DispatchSettings(ride)
                                           : Settings(ride, section == View::maintenance);
    }
    std::optional<Draft> BeginSetting(const Target& target, const Setting& selected)
    {
        const auto* ride = Resolve(target);
        if (!ride)
            return {};
        const auto section = selected.vehicle                                 ? View::vehicles
            : selected.section == View::appearance                            ? View::appearance
            : selected.section == View::music                                 ? View::music
            : selected.section == View::dispatch                              ? View::dispatch
            : selected.key == GameActions::RideSetSetting::inspectionInterval ? View::maintenance
                                                                              : View::operations;
        auto settings = SectionSettings(*ride, section);
        const auto found = std::find_if(settings.begin(), settings.end(), [&](const auto& setting) {
            return setting.key == selected.key && setting.vehicle == selected.vehicle
                && setting.appearance == selected.appearance && setting.index == selected.index
                && setting.name == selected.name;
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
        if (found->appearance)
            draft.baselineAppearance = AppearanceSnapshot(*ride);
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
    std::optional<Draft> BeginName(const Target& target)
    {
        auto draft = BeginDemolition(target);
        const auto* ride = Resolve(target);
        if (!draft || !ride)
            return {};
        draft->demolition = false;
        draft->renaming = true;
        draft->baselineName = ride->getName();
        draft->name = draft->baselineName;
        return draft;
    }
    std::optional<Draft> BeginRefurbishment(const Target& target)
    {
        auto draft = BeginDemolition(target);
        if (!draft)
            return {};
        draft->demolition = false;
        draft->refurbishment = true;
        GameActions::RideDemolishAction action(target.id, GameActions::RideModifyType::renew);
        const auto query = GameActions::Query(&action, getGameState());
        draft->refund = query.cost;
        return draft;
    }
    std::optional<PaintTarget> CapturePaint(const Target& target, const InteractionInfo& info)
    {
        const auto* ride = Resolve(target);
        if (!ride || !ride->getRideTypeDescriptor().flags.has(RtdFlag::supportsMultipleColourSchemes) || !info.Element
            || info.Element->isGhost() || info.Element->getType() != TileElementType::track)
            return {};
        const auto* track = info.Element->asTrack();
        if (track->getRideIndex() != target.id)
            return {};
        return PaintTarget{ target, CoordsXYZD{ info.Loc.x & ~31, info.Loc.y & ~31, track->getBaseZ(), track->getDirection() },
                            track->getTrackType(), track->getSequenceIndex(), track->getColourScheme() };
    }
    GameActions::Result Paint(const PaintTarget& target, uint8_t scheme, bool execute)
    {
        const auto* ride = Resolve(target.ride);
        const auto* track = MapGetTrackElementAtOfTypeSeq(target.location, target.type, target.sequence);
        if (!ride || !ride->getRideTypeDescriptor().flags.has(RtdFlag::supportsMultipleColourSchemes) || !track
            || track->isGhost() || track->getRideIndex() != target.ride.id || track->getDirection() != target.location.direction
            || track->getColourScheme() != target.baseline || scheme >= kNumRideColourSchemes)
            return Invalid("Track changed or paint target unavailable");
        if (target.baseline == scheme)
            return Invalid("Track already uses this colour scheme");
        GameActions::RideSetColourSchemeAction action(target.location, target.type, scheme);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
    std::optional<Draft> BeginRandomColours(const Target& target)
    {
        const auto* ride = Resolve(target);
        if (!ride || ride->vehicleColourSettings == VehicleColourSettings::same)
            return {};
        auto draft = BeginDemolition(target);
        if (!draft)
            return {};
        draft->demolition = false;
        draft->baselineSettings = Snapshot(*ride);
        draft->baselineAppearance = AppearanceSnapshot(*ride);
        for (const auto& setting : AppearanceSettings(*ride))
            if (setting.appearance == GameActions::RideSetAppearanceType::vehicleColourBody
                || setting.appearance == GameActions::RideSetAppearanceType::vehicleColourTrim
                || setting.appearance == GameActions::RideSetAppearanceType::vehicleColourTertiary)
            {
                const auto value = static_cast<uint16_t>(Drawing::getRandomColour());
                draft->randomColours.emplace_back(setting, value);
            }
        if (!draft->randomColours.empty()
            && std::all_of(draft->randomColours.begin(), draft->randomColours.end(), [](const auto& item) {
                   return item.first.current == item.second;
               }))
        {
            auto& [setting, value] = draft->randomColours.front();
            const auto different = std::find_if(
                setting.values.begin(), setting.values.end(), [&](auto c) { return c != setting.current; });
            if (different != setting.values.end())
                value = *different;
        }
        return draft->randomColours.empty() ? std::optional<Draft>{} : draft;
    }
    bool BeginGraphs(const Target& target)
    {
        if (!Resolve(target))
            return false;
        auto* ride = GetRide(target.id);
        // Native registration schedules measurement at the next departure and owns eviction.
        ride->getMeasurement();
        return true;
    }
    bool LocateMechanic(const Target& target)
    {
        const auto* ride = Resolve(target);
        auto* main = WindowGetMain();
        if (!ride || !main)
            return false;
        auto* mechanic = RideGetAssignedMechanic(*ride);
        if (!mechanic)
            mechanic = RideFindClosestMechanic(*ride, 1);
        if (!mechanic || mechanic->getLocation().isNull())
            return false;
        WindowScrollToLocation(*main, mechanic->getLocation());
        return true;
    }
    static std::string SettingValue(const Setting& setting, uint16_t value)
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
                        + (direction > 0       ? draft.setting->step
                               : direction < 0 ? -draft.setting->step
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
        if (!draft.status && !draft.demolition && !draft.refurbishment && !draft.renaming)
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
        if (!draft.randomColours.empty())
        {
            if (Snapshot(*ride) != draft.baselineSettings || AppearanceSnapshot(*ride) != draft.baselineAppearance)
                return Invalid("Vehicle colours/configuration changed; reopen review");
            const auto current = AppearanceSettings(*ride);
            GameActions::Result result;
            for (const auto& [setting, value] : draft.randomColours)
            {
                const auto found = std::find_if(current.begin(), current.end(), [&](const auto& s) {
                    return s.appearance == setting.appearance && s.index == setting.index;
                });
                if (found == current.end() || found->values != setting.values
                    || std::find(found->values.begin(), found->values.end(), value) == found->values.end())
                    return Invalid("Vehicle colour channel unavailable");
                GameActions::RideSetAppearanceAction action(draft.target.id, *setting.appearance, value, setting.index);
                result = GameActions::Query(&action, getGameState());
                if (result.error != GameActions::Status::ok)
                    return result;
            }
            if (!execute)
                return result;
            // Desktop randomization also submits one native action per channel. Query
            // all first, then retain the single UI completion callback on the final action.
            for (size_t i = 0; i < draft.randomColours.size(); ++i)
            {
                const auto& [setting, value] = draft.randomColours[i];
                GameActions::RideSetAppearanceAction action(draft.target.id, *setting.appearance, value, setting.index);
                if (i + 1 == draft.randomColours.size())
                    action.SetCallback(std::move(callback));
                result = GameActions::Execute(&action, getGameState());
                if (result.error != GameActions::Status::ok)
                    return result;
            }
            return result;
        }
        if (draft.renaming)
        {
            if (ride->getName() != draft.baselineName)
                return Invalid("Name changed; reopen editor");
            if (TextEntry{ draft.name }.Length() > TextEntry::limit)
                return Invalid("Name exceeds 32 characters");
            if (draft.name == draft.baselineName)
                return Invalid("Name unchanged");
            GameActions::RideSetNameAction action(draft.target.id, draft.name);
            action.SetCallback(std::move(callback));
            return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
        }
        if (draft.refurbishment)
        {
            GameActions::RideDemolishAction action(draft.target.id, GameActions::RideModifyType::renew);
            const auto query = GameActions::Query(&action, getGameState());
            if (query.error != GameActions::Status::ok)
                return query;
            if (query.cost != draft.refund)
                return Invalid("Refurbishment cost changed; reopen review");
            action.SetCallback(std::move(callback));
            return execute ? GameActions::Execute(&action, getGameState()) : query;
        }
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
                    && setting.appearance == draft.setting->appearance && setting.index == draft.setting->index
                    && setting.name == draft.setting->name;
            });
            if (found == current.end() || found->values != draft.setting->values || found->labels != draft.setting->labels
                || std::find(found->values.begin(), found->values.end(), draft.value) == found->values.end())
                return Invalid("Setting unavailable or outside native limits");
            if (draft.value == found->current)
                return Invalid("Setting unchanged");
            if (found->appearance)
            {
                if (AppearanceSnapshot(*ride) != draft.baselineAppearance)
                    return Invalid("Appearance changed; reopen review");
                GameActions::RideSetAppearanceAction action(draft.target.id, *found->appearance, draft.value, found->index);
                action.SetCallback(std::move(callback));
                return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
            }
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
            // Native RideSetStatusAction uses Close on an already-closed crashed ride
            // to clear the crash flag. Do not block that recovery action here.
            if (*draft.status == ride->status
                && !(*draft.status == RideStatus::closed && ride->flags.has(RideFlag::crashed)))
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
        const std::optional<Draft>& draft, const std::string& feedback, size_t graphMetric, size_t graphOffset)
    {
        ManagementPresentation model;
        model.breadcrumb = "Park / Rides";
        model.title = "Rides";
        model.selected = selected;
        model.status = feedback;
        if (view == View::roster)
        {
            model.rosterCards = true;
            for (const auto& entry : roster)
            {
                const auto* ride = Resolve(entry);
                model.rows.push_back(ride ? ride->getName() : "Ride unavailable");
                model.rowDetails.push_back(ride ? Status(*ride) + "  |  Queue " + std::to_string(ride->getTotalQueueLength()) : "No longer in park");
                model.rowEnabled.push_back(ride != nullptr);
            }
            model.rows.push_back("Back to dashboard");
            if (roster.empty())
                model.status = "No rides built yet";
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
            model.summaryMetrics = true;
            model.actionGrid = true;
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
                model.rows.insert(model.rows.begin(), "Place entrance / exit...");
            if (RideAccess::Supported(*target))
            {
                model.rows.insert(model.rows.begin() + (FlatRides::CanResume(*target) ? 1 : 0), "Entrances / exits...");
                model.rows.push_back("Access / paths...");
            }
            // Ride along sits beside Locate; the dispatcher takes it out by name before
            // resolving the remaining rows by position.
            if (RideFollow::Available(target->id))
                model.rows.insert(model.rows.begin() + 1, "Ride along");
            if (CustomTracks::CanResume(*target))
                model.rows.push_back("Continue track...");
            if (Mazes::CanResume(*target))
                model.rows.push_back("Edit maze...");
            if (TrackEditing::Eligible(*target))
                model.rows.push_back("Edit track...");
            model.rows.push_back("Save design...");
            model.rows.push_back("Rename...");
            model.rows.push_back("Appearance...");
            model.rows.push_back("Music...");
            model.rows.push_back("Vehicles...");
            model.rows.push_back("Dispatch...");
            model.rows.push_back("Operations...");
            model.rows.push_back("Maintenance...");
            model.rows.push_back("Demolish...");
        }
        else if (
            view == View::operations || view == View::maintenance || view == View::vehicles || view == View::dispatch
            || view == View::music || view == View::appearance)
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
                model.status = "Close the ride before changing vehicles";
            }
            if (view == View::dispatch)
            {
                model.breadcrumb = "Rides / Dispatch";
                model.stats = { { "Mode", FormatStringID(kRideModeNames[static_cast<uint8_t>(ride->mode)]) } };
            }
            if (view == View::appearance)
            {
                model.breadcrumb = "Rides / Appearance";
                model.stats = { { "Changes", "Colours and entrance styles" } };
            }
            if (view == View::music)
            {
                model.breadcrumb = "Rides / Music";
                model.stats = { { "Playback", ride->flags.has(RideFlag::music) ? "On" : "Off" } };
                if (MusicSettings(*ride).empty())
                    model.status = "Music unavailable for this ride";
            }
            for (const auto& setting : SectionSettings(*ride, view))
                model.rows.push_back(setting.name + ": " + SettingValue(setting, setting.current));
            if (view == View::appearance && ride->vehicleColourSettings != VehicleColourSettings::same)
            {
                const auto appearance = AppearanceSettings(*ride);
                if (std::any_of(appearance.begin(), appearance.end(), [](const auto& setting) {
                        return setting.appearance == GameActions::RideSetAppearanceType::vehicleColourBody
                            || setting.appearance == GameActions::RideSetAppearanceType::vehicleColourTrim
                            || setting.appearance == GameActions::RideSetAppearanceType::vehicleColourTertiary;
                    }))
                    model.rows.push_back("Randomize vehicle colours...");
            }
            if (view == View::appearance && ride->getRideTypeDescriptor().flags.has(RtdFlag::supportsMultipleColourSchemes))
                model.rows.push_back("Paint track scheme...");
            if (view == View::maintenance && !ride->getRideTypeDescriptor().availableBreakdowns.isEmpty())
            {
                model.rows.push_back("Locate mechanic");
                model.rows.push_back("Refurbish...");
            }
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
            model.rows = { "Customers...", "Finances...", "Graphs...", "Back to ride" };
        }
        else if (view == View::customers)
        {
            model.breadcrumb = "Rides / Customers";
            model.stats = {
                { "Total customers", std::to_string(ride->totalCustomers) },
                { "Customers / hour", std::to_string(RideCustomersPerHour(*ride)) },
                { "Riders now", std::to_string(ride->numRiders) },
                { "In queue", std::to_string(ride->getTotalQueueLength()) },
                { "Longest wait", std::to_string(ride->getMaxQueueTime()) + " minutes" },
                { "Popularity", ride->popularity == 255 ? "Unavailable" : std::to_string(ride->popularity * 4) + "%" },
                { "Satisfaction", ride->satisfaction == 255 ? "Unavailable" : std::to_string(ride->satisfaction * 5) + "%" }
            };
            model.rows = { "Stations...", "Back to statistics" };
        }
        else if (view == View::stations)
        {
            model.breadcrumb = "Rides / Stations";
            size_t index = 0;
            for (const auto& station : ride->getStations())
            {
                ++index;
                if (station.start.isNull())
                    continue;
                model.bodyLines.push_back(
                    "Station " + std::to_string(index) + " / queue " + std::to_string(station.queueLength) + " / "
                    + std::to_string(station.queueTime) + " min");
                model.bodyLines.push_back(
                    std::string("Entrance ") + (station.entrance.isNull() ? "missing" : "placed") + " / exit "
                    + (station.exit.isNull() ? "missing" : "placed"));
            }
            if (model.bodyLines.empty())
                model.bodyLines.push_back("No placed stations");
            model.rows = { "Back to customers" };
        }
        else if (view == View::earnings)
        {
            model.breadcrumb = "Rides / Finances";
            auto money = [](money64 value) {
                return getGameState().park.flags.has(ParkFlag::noMoney) ? std::string("Not applicable")
                    : value == kMoney64Undefined                        ? std::string("Unavailable")
                                                                        : FormatMoney(value);
            };
            model.stats = { { "Admission", money(RideGetPrice(*ride)) },
                            { "Income / hour", money(ride->incomePerHour) },
                            { "Running cost / hour",
                              money(ride->upkeepCost == kMoney64Undefined ? kMoney64Undefined : ride->upkeepCost * 16) },
                            { "Profit / hour", money(ride->profit) },
                            { "Total profit", money(ride->totalProfit) } };
            model.rows = { "Back to statistics" };
        }
        else if (view == View::graphs)
        {
            model.breadcrumb = "Rides / Graphs";
            model.rows = { "Speed", "Altitude", "Vertical G", "Lateral G", "Back to statistics" };
            const auto* measurement = ride->measurement.get();
            const bool supports = ride->getRideTypeDescriptor().flags.has(RtdFlag::hasDataLogging);
            if (!supports)
                model.status = "Data logging unavailable for this ride";
            else if (!measurement || !measurement->flags.has(RideMeasurementFlag::running) || measurement->num_items == 0)
                model.status = "Waiting for a vehicle to depart";
            else if (graphMetric > 3 || (graphMetric >= 2 && !measurement->flags.has(RideMeasurementFlag::gForces)))
                model.status = "G-force logging unavailable";
            else
            {
                const size_t count = std::min<size_t>(measurement->num_items, RideMeasurement::kMaxItems);
                const size_t first = std::min(graphOffset, count - 1);
                const size_t end = std::min(first + 240, count);
                for (size_t i = first; i < end; ++i)
                {
                    int value = graphMetric == 0 ? ToHumanReadableSpeed(int(measurement->velocity[i]) * 65536 / 5)
                        : graphMetric == 1       ? BaseZToMetres(measurement->altitude[i])
                        : graphMetric == 2       ? int(measurement->vertical[i]) * 8
                                                 : int(measurement->lateral[i]) * 8;
                    model.graph.push_back(value);
                }
                auto label = [&](int value) {
                    if (graphMetric < 2)
                        return FmtString(FormatStringID(
                                             graphMetric == 0 ? static_cast<StringId>(STR_VELOCITY)
                                                              : static_cast<StringId>(STR_RIDE_STATS_ALTITUDE_FORMAT),
                                             static_cast<int16_t>(value)))
                            .WithoutFormatTokens();
                    std::ostringstream text;
                    text << (value < 0 ? "-" : "") << std::abs(value) / 100 << '.' << std::setw(2) << std::setfill('0')
                         << std::abs(value) % 100 << "g";
                    return text.str();
                };
                const auto [low, high] = std::minmax_element(model.graph.begin(), model.graph.end());
                model.graphRange = label(*low) + " to " + label(*high);
                model.stats = { { model.rows[graphMetric],
                                  std::to_string(first / 16) + "-" + std::to_string((end + 15) / 16) + " seconds" } };
                model.status = "Left / Right: scroll time";
            }
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
                model.stats.emplace_back("Removes", "Ride, track and exits");
                model.stats.emplace_back(
                    "Refund",
                    draft->noMoney                                 ? "Not applicable"
                        : queried.error == GameActions::Status::ok ? FormatMoney(-queried.cost)
                                                                   : "Unavailable");
            }
            else if (!draft->randomColours.empty())
            {
                model.breadcrumb = "Rides / Randomize vehicle colours";
                model.stats.emplace_back("Channels", std::to_string(draft->randomColours.size()));
                model.stats.emplace_back("Cost", "No charge");
                model.status = "Applies the colours to this scheme";
            }
            else if (draft->renaming)
            {
                model.breadcrumb = "Rides / Rename";
                model.stats.emplace_back("Current", draft->baselineName);
                model.stats.emplace_back("New name", draft->name.empty() ? "Reset to default" : draft->name);
            }
            else if (draft->refurbishment)
            {
                model.breadcrumb = "Rides / Refurbish";
                model.stats.emplace_back("Work", "Renew age and reliability");
                model.stats.emplace_back(
                    "Cost",
                    draft->noMoney                                 ? "Not applicable"
                        : queried.error == GameActions::Status::ok ? FormatMoney(queried.cost)
                                                                   : "Unavailable");
            }
            else if (draft->setting)
            {
                model.breadcrumb = "Rides / " + draft->setting->name;
                model.stats.emplace_back("Current", SettingValue(*draft->setting, draft->setting->current));
                model.stats.emplace_back("Draft", SettingValue(*draft->setting, draft->value));
                if (draft->setting->vehicle)
                {
                    const auto* entry = draft->setting->vehicle == GameActions::RideSetVehicleType::rideEntry
                        ? GetRideEntryByIndex(draft->value)
                        : ride->getRideEntry();
                    model.stats.emplace_back(
                        "Effective trains / cars",
                        std::to_string(draft->effectiveVehicles[0]) + " / "
                            + std::to_string(std::max(0, draft->effectiveVehicles[1] - (entry ? entry->zero_cars : 0))));
                    model.status = "Vehicle counts follow the ride limits";
                }
                if (queried.error == GameActions::Status::ok
                    && draft->setting->key == GameActions::RideSetSetting::minWaitingTime)
                    model.status = "Maximum wait also increases if needed";
                if (queried.error == GameActions::Status::ok
                    && draft->setting->key == GameActions::RideSetSetting::maxWaitingTime)
                    model.status = "Minimum wait also decreases if needed";
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
            if (!draft->demolition && queried.cost!=0)
                model.stats.emplace_back(
                    "Cost", queried.error == GameActions::Status::ok ? FormatMoney(queried.cost) : "Unavailable");
            if (view == View::price && draft->setting && draft->setting->appearance)
            {
                model.title = draft->setting->name;
                if (IsColourSetting(*draft->setting)) model.colourValues = draft->setting->values;
                model.currentColour = draft->setting->current;
                model.rows = draft->setting->labels;
                model.primaryAction = "Review";
                model.editing = false;
                model.navigationHint = "Choose";
                return model;
            }
            const bool vehicleReview=draft->setting && draft->setting->vehicle.has_value();
            if(view==View::price) {
                model.title=draft->setting ? draft->setting->name : "Admission price";
                model.primaryAction=vehicleReview ? "Review" : "Apply";
                model.navigationHint="Adjust";
                model.summaryMetrics=true;model.metricColumns=1;
                if(feedback.empty())model.status=queried.error==GameActions::Status::ok ? "Left/Right: Adjust" : Finances::Feedback(queried);
            }
            model.rows = view == View::price ? std::vector<std::string>{vehicleReview ? "Review configuration" : "Apply change", "Cancel"}
                : std::vector<std::string>{draft->demolition ? "Demolish ride" : draft->refurbishment ? "Refurbish ride" : "Apply change", "Cancel"};
            if (queried.error != GameActions::Status::ok && feedback.empty())
                model.status = Finances::Feedback(queried);
        }
        return model;
    }
} // namespace OpenRCT2::Ui::Handheld::RideManagement
