// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Currency.h"
#include "ManagementPanel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/ride/RideSetAppearanceAction.h>
#include <openrct2/actions/ride/RideSetSettingAction.h>
#include <openrct2/actions/ride/RideSetVehicleAction.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ride/Ride.h>

namespace OpenRCT2::Ui::Handheld::RideManagement
{
    struct Target
    {
        RideId id;
        ride_type_t type;
        ObjectEntryIndex object;
        int32_t buildDate;
        CoordsXY location;
        uint64_t generation;
        bool shop{};
        bool operator==(const Target&) const = default;
    };
    enum class View
    {
        roster,
        inspection,
        statistics,
        customers,
        stations,
        earnings,
        graphs,
        operations,
        maintenance,
        vehicles,
        dispatch,
        music,
        appearance,
        price,
        confirmation
    };
    struct Setting
    {
        GameActions::RideSetSetting key;
        std::string name;
        std::vector<uint16_t> values;
        std::vector<std::string> labels;
        uint16_t current;
        std::optional<GameActions::RideSetVehicleType> vehicle{};
        View section = View::operations;
        uint8_t step = 1;
        std::optional<GameActions::RideSetAppearanceType> appearance{};
        uint32_t index{};
    };
    std::vector<Setting> Settings(const Ride& ride, bool maintenance = false);
    std::vector<Setting> VehicleSettings(const Ride& ride);
    std::vector<Setting> DispatchSettings(const Ride& ride);
    std::vector<Setting> MusicSettings(const Ride& ride);
    std::vector<Setting> AppearanceSettings(const Ride& ride);
    struct Draft
    {
        Target target;
        RideStatus baselineStatus;
        money64 baselinePrice;
        bool pricesUnlocked;
        bool noMoney;
        std::optional<RideStatus> status;
        money64 price;
        std::optional<Setting> setting;
        uint16_t value{};
        std::array<uint16_t, 17> baselineSettings{};
        std::array<uint8_t, 2> effectiveVehicles{};
        bool demolition{};
        money64 refund{};
        bool refurbishment{};
        std::vector<uint16_t> baselineAppearance{};
        bool renaming{};
        std::string baselineName{};
        std::string name{};
        std::vector<std::pair<Setting, uint16_t>> randomColours{};
    };
    void Invalidate(RideId id);
    void InvalidateAll();
    std::optional<Target> Identify(RideId id, bool includeShops = false);
    const Ride* Resolve(const Target& target);
    std::vector<Target> Roster();
    std::optional<CoordsXYZ> Location(const Target& target);
    bool Locate(const Target& target);
    bool PricePermitted(const Ride& ride);
    std::optional<Draft> Begin(const Target& target, std::optional<RideStatus> status = {});
    std::optional<Draft> BeginSetting(const Target& target, const Setting& setting);
    std::optional<Draft> BeginDemolition(const Target& target);
    std::optional<Draft> BeginName(const Target& target);
    std::optional<Draft> BeginRefurbishment(const Target& target);
    struct PaintTarget
    {
        Target ride;
        CoordsXYZD location;
        TrackElemType type;
        uint8_t sequence;
        uint8_t baseline;
    };
    std::optional<PaintTarget> CapturePaint(const Target& target, const InteractionInfo& info);
    GameActions::Result Paint(const PaintTarget& target, uint8_t scheme, bool execute);
    std::optional<Draft> BeginRandomColours(const Target& target);
    bool BeginGraphs(const Target& target);
    bool LocateMechanic(const Target& target);
    void AdjustPrice(Draft& draft, int direction);
    GameActions::Result Action(const Draft& draft, bool execute, GameActions::GameAction::Callback_t callback = {});
    ManagementPresentation Presentation(
        View view, size_t selected, const std::vector<Target>& roster = {}, const std::optional<Target>& target = {},
        const std::optional<Draft>& draft = {}, const std::string& feedback = {}, size_t graphMetric = 0,
        size_t graphOffset = 0);
} // namespace OpenRCT2::Ui::Handheld::RideManagement
