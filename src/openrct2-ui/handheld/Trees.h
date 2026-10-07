// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/scenery/SmallSceneryPlaceAction.h>
#include <openrct2/object/ObjectEntryManager.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/SmallSceneryObject.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/Scenery.h>
#include <optional>
#include <string>

namespace OpenRCT2::Ui::Handheld::Trees
{
    inline bool Eligible(ObjectEntryIndex index)
    {
        const auto* object = GetContext()->GetObjectManager().GetLoadedObject<SmallSceneryObject>(index);
        if (!object)
            return false;
        const auto* entry = ObjectEntryManager::GetObjectEntry<SmallSceneryEntry>(index);
        return entry->flags.hasAll(SmallSceneryFlag::isTree, SmallSceneryFlag::occupiesFullTile)
            && !entry->flags.hasAny(SmallSceneryFlag::occupiesHalfTile, SmallSceneryFlag::occupiesThreeQuarters)
            && IsSceneryAvailableToBuild({ SCENERY_TYPE_SMALL, index });
    }

    inline ObjectEntryIndex Next(ObjectEntryIndex current)
    {
        const int start = current == kObjectEntryIndexNull ? -1 : current;
        for (int step = 1; step <= kMaxSmallSceneryObjects; ++step)
        {
            const auto index = static_cast<ObjectEntryIndex>((start + step) % kMaxSmallSceneryObjects);
            if (Eligible(index))
                return index;
        }
        return kObjectEntryIndexNull;
    }

    inline std::string Name(ObjectEntryIndex index)
    {
        const auto* object = Eligible(index) ? GetContext()->GetObjectManager().GetLoadedObject<SmallSceneryObject>(index)
                                             : nullptr;
        return object ? object->GetName() : "No trees available";
    }

    // Selection remains independent of preview ownership for a future controller picker.
    void UpdatePreview(std::optional<CoordsXY> tile, ObjectEntryIndex index);
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
    GameActions::Result Action(std::optional<CoordsXY> tile, ObjectEntryIndex index, bool execute);
} // namespace OpenRCT2::Ui::Handheld::Trees
