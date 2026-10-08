// SPDX-License-Identifier: GPL-3.0-only
#include "TrackPreview.h"

#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/actions/track/TrackRemoveAction.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
namespace OpenRCT2::Ui::Handheld
{
    bool TrackPreview::exists() const
    {
        if (!_origin || !_ride)
            return false;
        for (auto* e : TileElementsView<TrackElement>(*_origin))
            if (e->isGhost() && e->getRideIndex() == id())
                return true;
        return false;
    }
    void TrackPreview::clear()
    {
        if (exists())
        {
            auto remove = GameActions::TrackRemoveAction(GetRide(id())->getRideTypeDescriptor().StartTrackPiece, 0, *_origin);
            remove.SetFlags(
                { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
                  GameActions::CommandFlag::allowDuringPaused });
            GameActions::Execute(&remove, getGameState());
        }
        _origin.reset();
        _ride.reset();
    }
    bool TrackPreview::place(ObjectEntryIndex index, ride_type_t type, CoordsXYZD origin)
    {
        clear();
        _ride = std::make_unique<ShopPlacement::PreviewRide>(index, type);
        if (id().IsNull())
        {
            clear();
            return false;
        }
        auto place = GameActions::TrackPlaceAction(
            id(), GetRideTypeDescriptor(type).StartTrackPiece, type, origin, 0, 0, 0, {}, false);
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
