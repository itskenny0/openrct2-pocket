// SPDX-License-Identifier: GPL-3.0-only
#include "Shops.h"

#include "TrackPreview.h"

#include <openrct2/actions/ride/ShopPlaceAction.h>
#include <openrct2/actions/track/TrackPlaceAction.h>
#include <openrct2/actions/track/TrackRemoveAction.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/TrackElement.h>
namespace OpenRCT2::Ui::Handheld::Shops
{
    struct Request
    {
        CoordsXYZD origin;
        ObjectEntryIndex index;
        bool operator==(const Request&) const = default;
    };
    static std::optional<Request> requested, ghost;
    static TrackPreview preview;
    static uint32_t suspensionDepth{};
    static bool Exists()
    {
        return preview.exists();
    }
    static void RemoveGhost()
    {
        preview.clear();
        ghost.reset();
    }
    void ClearPreview()
    {
        requested.reset();
        RemoveGhost();
    }
    static std::optional<Request> Resolve(std::optional<CoordsXY> tile, ObjectEntryIndex index, Direction direction)
    {
        const auto max = GetMapSizeMaxXY();
        if (!tile || tile->x < 32 || tile->y < 32 || tile->x > max.x || tile->y > max.y || tile->x % 32 || tile->y % 32
            || !MapGetSurfaceElementAt(*tile) || !Eligible(index))
            return {};
        // Same native highest ground/water height as desktop ground construction.
        return Request{ { *tile, MapGetHighestZ(*tile), direction }, index };
    }
    static GameActions::Result Query(const Request& request)
    {
        if (!preview.id().IsNull() && ghost && ghost->index == request.index)
        {
            const auto type = ShopPlacement::Type(request.index);
            auto action = GameActions::TrackPlaceAction(
                preview.id(), GetRideTypeDescriptor(type).StartTrackPiece, type, request.origin, 0, 0, 0, {}, false);
            return GameActions::Query(&action, getGameState());
        }
        auto action = GameActions::ShopPlaceAction(request.index, request.origin);
        return GameActions::Query(&action, getGameState());
    }
    GameActions::Result Action(std::optional<CoordsXY> tile, ObjectEntryIndex index, Direction direction, bool execute)
    {
        if (execute)
            ClearPreview();
        const auto request = Resolve(tile, index, direction);
        if (!request)
        {
            ClearPreview();
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = Eligible(index) ? "Move onto land" : "Shop unavailable";
            return result;
        }
        auto result = Query(*request);
        if (result.error != GameActions::Status::ok)
            ClearPreview();
        if (!execute || result.error != GameActions::Status::ok)
            return result;
        auto action = GameActions::ShopPlaceAction(index, request->origin);
        return GameActions::Execute(&action, getGameState());
    }
    void UpdatePreview(std::optional<CoordsXY> tile, ObjectEntryIndex index, Direction direction)
    {
        const auto request = Resolve(tile, index, direction);
        if (ghost && ghost->index != index)
            RemoveGhost();
        if (!request || Query(*request).error != GameActions::Status::ok)
        {
            ClearPreview();
            return;
        }
        requested = request;
        if (suspensionDepth)
            return;
        if (ghost == request && Exists())
            return;
        RemoveGhost();
        if (preview.place(index, ShopPlacement::Type(index), request->origin))
            ghost = request;
    }
    void RemovePreviewTemporarily()
    {
        ++suspensionDepth;
        RemoveGhost();
    }
    void RestorePreview()
    {
        if (!suspensionDepth || --suspensionDepth || !requested)
            return;
        const auto request = *requested;
        UpdatePreview(request.origin, request.index, request.origin.direction);
    }
} // namespace OpenRCT2::Ui::Handheld::Shops
