// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/ride/Ride.h>
#include <openrct2/ride/ShopItem.h>
#include <optional>

namespace OpenRCT2::Ui::Handheld::ShopManagement
{
    struct Target
    {
        RideId id;
        ride_type_t type;
        ObjectEntryIndex object;
        int32_t buildDate;
        CoordsXYZ location;
        Direction direction;
    };
    struct Price
    {
        bool primary;
        ShopItem item;
        std::string name;
        money64 value;
        bool shared;
    };
    enum class View
    {
        inspection,
        prices,
        demolition
    };
    std::optional<Target> Capture(RideId id);
    std::optional<Target> FromInfo(const InteractionInfo& info);
    const Ride* Find(const Target& target);
    std::vector<Price> Prices(const Target& target);
    GameActions::Result SetStatus(const Target& target, bool open);
    GameActions::Result SetPrice(const Target& target, bool primary, money64 value);
    GameActions::Result Demolish(const Target& target, bool execute);
    std::string Money(money64 value);
    void Draw(
        Drawing::RenderTarget& rt, ScreenSize screen, const std::optional<Target>& target, View view, size_t focus,
        std::optional<money64> draft = {}, const std::string& feedback = {});
} // namespace OpenRCT2::Ui::Handheld::ShopManagement
