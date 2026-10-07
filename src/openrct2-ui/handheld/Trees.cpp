// SPDX-License-Identifier: GPL-3.0-only
#include "Trees.h"

#include <openrct2/actions/scenery/SmallSceneryRemoveAction.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>

namespace OpenRCT2::Ui::Handheld::Trees
{
    struct Request
    {
        CoordsXY tile;
        ObjectEntryIndex index;
        bool operator==(const Request&) const = default;
    };
    struct Ghost
    {
        Request request;
        int32_t height;
        uint8_t quadrant;
    };
    static std::optional<Request> requested;
    static std::optional<Ghost> ghost;
    static uint32_t suspensionDepth{};

    static GameActions::SmallSceneryPlaceAction Placement(const Request& request)
    {
        // Match the existing native ground-height/default direction/colour configuration.
        return { { request.tile.x, request.tile.y, 0, 3 },
                 0,
                 request.index,
                 Drawing::Colour::bordeauxRed,
                 Drawing::Colour::yellow,
                 Drawing::Colour::darkBrown };
    }

    static bool GhostExists(const Ghost& preview)
    {
        for (const auto* element : TileElementsView<SmallSceneryElement>(preview.request.tile))
            if (element->isGhost() && element->getEntryIndex() == preview.request.index && element->getBaseZ() == preview.height
                && element->getSceneryQuadrant() == preview.quadrant)
                return true;
        return false;
    }

    static void RemoveGhost()
    {
        if (!ghost)
            return;
        const auto previous = *ghost;
        ghost.reset();
        // Native ghost-only lookup cannot delete a real tree, even after ownership changes.
        auto action = GameActions::SmallSceneryRemoveAction(
            { previous.request.tile, previous.height }, previous.quadrant, previous.request.index);
        action.SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        GameActions::Execute(&action, getGameState());
    }

    void ClearPreview()
    {
        requested.reset();
        RemoveGhost();
    }

    void RemovePreviewTemporarily()
    {
        ++suspensionDepth;
        RemoveGhost();
    }

    void RestorePreview()
    {
        if (suspensionDepth == 0 || --suspensionDepth != 0 || !requested)
            return;
        const auto request = *requested;
        UpdatePreview(request.tile, request.index);
    }

    static GameActions::Result Query(std::optional<CoordsXY> tile, ObjectEntryIndex index)
    {
        const auto max = GetMapSizeMaxXY();
        const bool valid = tile && tile->x >= 32 && tile->y >= 32 && tile->x <= max.x && tile->y <= max.y && tile->x % 32 == 0
            && tile->y % 32 == 0;
        if (!valid || !MapGetSurfaceElementAt(*tile) || !Eligible(index))
        {
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = !valid ? "Move onto land." : "Tree unavailable.";
            return result;
        }
        auto action = Placement({ *tile, index });
        return GameActions::Query(&action, getGameState());
    }

    void UpdatePreview(std::optional<CoordsXY> tile, ObjectEntryIndex index)
    {
        // Ordinary queries ignore ghosts and enforce real funds, pause and clearance.
        const auto result = Query(tile, index);
        if (result.error != GameActions::Status::ok)
        {
            ClearPreview();
            return;
        }
        const Request request{ *tile, index };
        requested = request;
        if (suspensionDepth != 0)
            return;
        if (ghost && ghost->request == request && ghost->height == result.position.z && GhostExists(*ghost))
            return; // No duplicate/replacement action for an unchanged valid tree.
        RemoveGhost();
        auto action = Placement(request);
        action.SetFlags(
            { GameActions::CommandFlag::ghost, GameActions::CommandFlag::noSpend,
              GameActions::CommandFlag::allowDuringPaused });
        const auto placed = GameActions::Execute(&action, getGameState());
        if (placed.error == GameActions::Status::ok)
        {
            const auto data = placed.getData<GameActions::SmallSceneryPlaceActionResult>();
            ghost = Ghost{ request, data.BaseHeight, data.SceneryQuadrant };
        }
    }

    GameActions::Result Action(std::optional<CoordsXY> tile, ObjectEntryIndex index, bool execute)
    {
        if (execute)
            ClearPreview();
        const auto query = Query(tile, index);
        if (query.error != GameActions::Status::ok)
            ClearPreview();
        if (!execute || query.error != GameActions::Status::ok)
            return query;
        auto action = Placement({ *tile, index });
        return GameActions::Execute(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::Trees
