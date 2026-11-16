// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/scenery/SmallSceneryRemoveAction.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/SmallSceneryObject.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/TileElementsView.h>
#include <openrct2/world/tile_element/SmallSceneryElement.h>
#include <optional>
#include <string>

namespace OpenRCT2::Ui::Handheld::SceneryRemoval
{
    struct Target
    {
        CoordsXYZ location;
        uint8_t quadrant;
        ObjectEntryIndex object;
    };

    // Match the native action's first-match identity, without retaining a map pointer.
    inline SmallSceneryElement* Find(const Target& target)
    {
        if (!MapIsLocationValid(target.location))
            return nullptr;
        for (auto* element : TileElementsView<SmallSceneryElement>(target.location))
            if (element->getBaseZ() == target.location.z && element->getSceneryQuadrant() == target.quadrant
                && element->getEntryIndex() == target.object)
                return element;
        return nullptr;
    }

    inline std::optional<Target> FromInfo(const InteractionInfo& info)
    {
        if (info.interactionType != ViewportInteractionItem::scenery || !info.Element
            || info.Element->getType() != TileElementType::smallScenery || info.Element->isGhost())
            return std::nullopt;
        const auto* element = info.Element->asSmallScenery();
        const Target target{ { info.Loc.x & ~31, info.Loc.y & ~31, element->getBaseZ() },
                             element->getSceneryQuadrant(),
                             element->getEntryIndex() };
        if (Find(target) != element)
            return std::nullopt;
        return target;
    }

    inline std::string Name(const std::optional<Target>& target)
    {
        const auto* object = target ? GetContext()->GetObjectManager().GetLoadedObject<SmallSceneryObject>(target->object)
                                    : nullptr;
        return object ? object->GetName() : "No target";
    }

    inline GameActions::Result Action(const std::optional<Target>& target, bool execute)
    {
        const auto* element = target ? Find(*target) : nullptr;
        // Native non-ghost removal can also match ghosts. Never submit that identity.
        if (!element || element->isGhost())
        {
            GameActions::Result result;
            result.error = GameActions::Status::invalidParameters;
            result.errorMessage = "Point at small scenery.";
            return result;
        }
        auto action = GameActions::SmallSceneryRemoveAction(target->location, target->quadrant, target->object);
        return execute ? GameActions::Execute(&action, getGameState()) : GameActions::Query(&action, getGameState());
    }
} // namespace OpenRCT2::Ui::Handheld::SceneryRemoval
