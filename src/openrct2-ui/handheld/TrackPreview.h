// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/ride/ShopPlacement.h>
#include <optional>
namespace OpenRCT2::Ui::Handheld
{
    class TrackPreview
    {
        std::unique_ptr<ShopPlacement::PreviewRide> _ride;
        std::optional<CoordsXYZD> _origin;

    public:
        RideId id() const
        {
            return _ride ? _ride->id() : RideId::GetNull();
        }
        bool exists() const;
        void clear();
        bool place(ObjectEntryIndex index, ride_type_t type, CoordsXYZD origin);
    };
} // namespace OpenRCT2::Ui::Handheld
