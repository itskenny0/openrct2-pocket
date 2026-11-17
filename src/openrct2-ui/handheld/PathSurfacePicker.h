// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"
#include "Paths.h"

#include <openrct2/object/FootpathObject.h>
#include <openrct2/object/FootpathSurfaceObject.h>
#include <openrct2/object/ObjectLimits.h>
namespace OpenRCT2::Ui::Handheld::PathSurfaces
{
    inline std::string Key(const Object& object)
    {
        return std::string(Paths::IsQueue() ? "queue:" : "path:") + object.GetDescriptor().ToString();
    }
    inline std::vector<PickerEntry> Entries()
    {
        std::vector<PickerEntry> entries;
        if (!Paths::IsActive())
            return entries;
        auto& objects = GetContext()->GetObjectManager();
        const bool legacy = gFootpathSelection.legacyPath != kObjectEntryIndexNull;
        const auto limit = legacy ? kMaxPathObjects : kMaxFootpathSurfaceObjects;
        for (ObjectEntryIndex i = 0; i < limit; ++i)
        {
            const Object* object = legacy ? static_cast<const Object*>(objects.GetLoadedObject<FootpathObject>(i))
                                          : static_cast<const Object*>(objects.GetLoadedObject<FootpathSurfaceObject>(i));
            if (!object)
                continue;
            const auto flags = legacy ? static_cast<const FootpathObject*>(object)->GetPathSurfaceDescriptor().flags
                                      : static_cast<const FootpathSurfaceObject*>(object)->Flags;
            if (flags & FOOTPATH_ENTRY_FLAG_SHOW_ONLY_IN_SCENARIO_EDITOR)
                continue;
            if (!legacy && Paths::IsQueue() != ((flags & FOOTPATH_ENTRY_FLAG_IS_QUEUE) != 0))
                continue;
            entries.push_back(
                { { legacy ? ObjectType::paths : ObjectType::footpathSurface, i, Key(*object) },
                  object->GetName(),
                  ZoomLevel{ 1 } });
        }
        return entries;
    }
    inline std::optional<PickerIdentity> Current()
    {
        const auto index = gFootpathSelection.legacyPath != kObjectEntryIndexNull ? gFootpathSelection.legacyPath
                                                                                  : gFootpathSelection.getSelectedSurface();
        for (const auto& e : Entries())
            if (e.identity.index == index)
                return e.identity;
        return {};
    }
    inline bool Select(const PickerIdentity& choice)
    {
        const auto entries = Entries();
        const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& e) {
            return e.identity.type == choice.type && e.identity.index == choice.index && e.identity.key == choice.key;
        });
        if (found == entries.end())
            return false;
        Paths::ClearPreview();
        if (choice.type == ObjectType::paths)
            gFootpathSelection.legacyPath = choice.index;
        else if (Paths::IsQueue())
            gFootpathSelection.queueSurface = choice.index;
        else
            gFootpathSelection.normalSurface = choice.index;
        return true;
    }
} // namespace OpenRCT2::Ui::Handheld::PathSurfaces
