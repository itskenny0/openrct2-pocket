// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"

#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/ride/ShopPlacement.h>
#include <optional>
namespace OpenRCT2::Ui::Handheld::Shops
{
    inline bool Eligible(ObjectEntryIndex index)
    {
        return ShopPlacement::Eligible(index);
    }
    inline std::string Name(ObjectEntryIndex index)
    {
        const auto* object = GetContext()->GetObjectManager().GetLoadedObject<RideObject>(index);
        return object ? object->GetName() : "No shops available";
    }
    inline std::vector<PickerEntry> Entries()
    {
        std::vector<PickerEntry> entries;
        for (ObjectEntryIndex index = 0; index < kMaxRideObjects; ++index)
            if (Eligible(index))
                entries.push_back({ { ObjectType::ride, index }, Name(index), ZoomLevel{ 1 } });
        return entries;
    }
    void ClearPreview();
    void UpdatePreview(std::optional<CoordsXY> tile, ObjectEntryIndex index, Direction direction);
    void RemovePreviewTemporarily();
    void RestorePreview();
    GameActions::Result Action(std::optional<CoordsXY> tile, ObjectEntryIndex index, Direction direction, bool execute);
} // namespace OpenRCT2::Ui::Handheld::Shops
