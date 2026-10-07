// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "Trees.h"

#include <openrct2/OpenRCT2.h>
#include <openrct2/drawing/Drawing.Sprite.h>

namespace OpenRCT2::Ui::Handheld
{
    inline std::vector<PickerEntry> TreePickerEntries()
    {
        std::vector<PickerEntry> entries;
        for (ObjectEntryIndex index = 0; index < kMaxSmallSceneryObjects; ++index)
            if (Trees::Eligible(index))
            {
                ZoomLevel zoom{ 2 };
                const auto* entry = ObjectEntryManager::GetObjectEntry<SmallSceneryEntry>(index);
                if (!gOpenRCT2NoGraphics)
                    if (const auto* sprite = GfxGetG1Element(entry->image))
                    {
                        const auto extent = std::max(sprite->width, sprite->height);
                        zoom = ZoomLevel{ static_cast<int8_t>(extent <= 48 ? 0 : extent <= 104 ? 1 : 2) };
                    }
                entries.push_back({ { ObjectType::smallScenery, index }, Trees::Name(index), zoom });
            }
        return entries;
    }
} // namespace OpenRCT2::Ui::Handheld
