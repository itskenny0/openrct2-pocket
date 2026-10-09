// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/ride/RideConstruction.h>
#include <openrct2/ride/ShopPlacement.h>
#include <openrct2/ride/ted/TrackElemType.h>
#include <optional>
namespace OpenRCT2::Ui::Handheld
{
    class TrackPreview
    {
        std::unique_ptr<ShopPlacement::PreviewRide> _ride;
        std::optional<CoordsXYZD> _origin;
        TrackElemType _type{};

    public:
        RideId id() const
        {
            return _ride ? _ride->id() : RideId::GetNull();
        }
        bool exists() const;
        void clear();
        bool place(
            ObjectEntryIndex index, ride_type_t type, CoordsXYZD origin, TrackElemType piece = TrackElemType::none,
            SelectedLiftAndInverted flags = {}, uint8_t speed = 6, uint8_t seat = 0);
    };
} // namespace OpenRCT2::Ui::Handheld
