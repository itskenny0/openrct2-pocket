// SPDX-License-Identifier: GPL-3.0-only
#include "TrackPreview.h"

#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/actions/track/TrackRemoveAction.h>
#include <openrct2/ride/TrackData.h>
#include <openrct2/ride/ted/TrackElementDescriptor.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
namespace OpenRCT2::Ui::Handheld
{
    bool TrackPreview::exists() const
    {
        if (!_origin || !_ride)
            return false;
        const auto& c = TrackMetadata::GetTrackElementDescriptor(_type).sequenceData.sequences[0].clearance;
        const auto xy = CoordsXY(*_origin) + CoordsXY{ c.x, c.y }.rotate(_origin->direction);
        for (auto* e : TileElementsView<TrackElement>(xy))
            if (e->isGhost() && e->getRideIndex() == id())
                return true;
        return false;
    }
    void TrackPreview::clear()
    {
        if (exists())
        {
            const auto& c = TrackMetadata::GetTrackElementDescriptor(_type).sequenceData.sequences[0].clearance;
            const auto xy = CoordsXY(*_origin) + CoordsXY{ c.x, c.y }.rotate(_origin->direction);
            auto remove = GameActions::TrackRemoveAction(_type, 0, { xy, _origin->z + c.z, _origin->direction });
            remove.SetFlags(
                { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
                  GameActions::CommandFlag::allowDuringPaused });
            const auto result = GameActions::Execute(&remove, getGameState());
            if (result.error == GameActions::Status::notOwned)
                remove.Execute(getGameState(), getGameState().park);
        }
        _origin.reset();
        _ride.reset();
    }
    bool TrackPreview::place(
        ObjectEntryIndex index, ride_type_t type, CoordsXYZD origin, TrackElemType piece, SelectedLiftAndInverted flags,
        uint8_t speed)
    {
        clear();
        _ride = std::make_unique<ShopPlacement::PreviewRide>(index, type);
        if (id().IsNull())
        {
            clear();
            return false;
        }
        _type = piece == TrackElemType::none ? GetRideTypeDescriptor(type).StartTrackPiece : piece;
        auto place = GameActions::TrackPlaceAction(id(), _type, type, origin, speed, 0, 0, flags, false);
        place.SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        if (GameActions::Execute(&place, getGameState()).error != GameActions::Status::ok)
        {
            clear();
            return false;
        }
        _origin = origin;
        return true;
    }
} // namespace OpenRCT2::Ui::Handheld
