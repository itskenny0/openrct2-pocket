// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "AreaTools.h"
#include "ObjectPicker.h"
namespace OpenRCT2::Ui::Handheld::TerrainMaterials
{
    inline std::vector<PickerEntry> Entries(bool edge)
    {
        const auto type = edge ? ObjectType::terrainEdge : ObjectType::terrainSurface;
        std::vector<PickerEntry> result{ { { type, kObjectEntryIndexNull, "keep" }, "Keep unchanged", ZoomLevel{ 0 } } };
        auto& objects = GetContext()->GetObjectManager();
        for (ObjectEntryIndex i = 0; i < 255; ++i)
        {
            const auto* o = objects.GetLoadedObject(type, i);
            if (!o)
                continue;
            result.push_back(
                { { type, i, o->GetDescriptor().ToString() },
                  o->GetName(),
                  ZoomLevel{ 0 },
                  [edge, i](Drawing::RenderTarget& rt) {
                      auto& manager = GetContext()->GetObjectManager();
                      if (edge)
                      {
                          if (const auto* material = manager.GetLoadedObject<TerrainEdgeObject>(i))
                              GfxDrawSprite(
                                  rt, ImageId(material->IconImageId, material->getPreviewColour()), { rt.x + 30, rt.y + 30 });
                      }
                      else
                      {
                          if (const auto* material = manager.GetLoadedObject<TerrainSurfaceObject>(i))
                              GfxDrawSprite(
                                  rt, ImageId(material->IconImageId, material->getPreviewColour()), { rt.x + 30, rt.y + 30 });
                      }
                  } });
        }
        return result;
    }
    inline bool Select(bool edge, const PickerIdentity& choice)
    {
        const auto entries = Entries(edge);
        if (std::none_of(entries.begin(), entries.end(), [&](const auto& e) {
                return e.identity.type == choice.type && e.identity.index == choice.index && e.identity.key == choice.key;
            }))
            return false;
        if (edge)
            AreaTools::settings.edge = choice.index;
        else
            AreaTools::settings.surface = choice.index;
        AreaTools::settings.operation = AreaTools::Operation::paint;
        bool enabled;
        if (edge)
            LandTool::resetEdgeColourSelection(AreaTools::settings.edge, AreaTools::settings.edgeColour, enabled);
        else
            LandTool::resetSurfaceColourSelection(AreaTools::settings.surface, AreaTools::settings.surfaceColour, enabled);
        return true;
    }
} // namespace OpenRCT2::Ui::Handheld::TerrainMaterials
