// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ManagementPanel.h"
#include "ParkInformation.h"

#include <openrct2/Context.h>
#include <openrct2/GameState.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/Object.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/ride/RideData.h>
namespace OpenRCT2::Ui::Handheld::ResearchCatalog
{
    struct Target
    {
        ResearchItem item{};
        size_t occurrence{};
        std::optional<ObjectEntryDescriptor> descriptor;
        bool operator==(const Target& other) const
        {
            return item.rawValue == other.item.rawValue && item.flags == other.item.flags
                && item.category == other.item.category && occurrence == other.occurrence && descriptor == other.descriptor;
        }
    };
    inline Object* Loaded(const ResearchItem& item)
    {
        if (item.isNull() || EnumValue(item.category) > 6)
            return nullptr;
        if (item.type == Research::EntryType::ride && item.baseRideType >= RIDE_TYPE_COUNT)
            return nullptr;
        if (item.type != Research::EntryType::ride && item.type != Research::EntryType::scenery)
            return nullptr;
        return GetContext()->GetObjectManager().GetLoadedObject(
            item.type == Research::EntryType::ride ? ObjectType::ride : ObjectType::sceneryGroup, item.entryIndex);
    }
    inline bool Invented(const ResearchItem& item)
    {
        if (!Loaded(item))
            return false;
        return item.type == Research::EntryType::ride
            ? RideTypeIsInvented(item.baseRideType) && RideEntryIsInvented(item.entryIndex)
            : SceneryGroupIsInvented(item.entryIndex);
    }
    inline std::string Category(uint8_t filter)
    {
        if (!filter)
            return "All categories";
        ResearchItem item{};
        item.category = static_cast<ResearchCategory>((filter - 1) % 7);
        return ParkInformation::PlainText(FormatStringIDLegacy(item.getCategoryName(), nullptr), true);
    }
    inline std::vector<Target> Roster(uint8_t filter)
    {
        std::vector<Target> result;
        const auto& list = getGameState().researchItemsInvented;
        for (size_t i = 0; i < list.size(); ++i)
        {
            const auto& item = list[i];
            if (item.isNull() || (filter && EnumValue(item.category) != filter - 1))
                continue;
            size_t occurrence = 0;
            for (size_t j = 0; j < i; ++j)
                if (list[j].rawValue == item.rawValue && list[j].category == item.category && list[j].flags == item.flags)
                    ++occurrence;
            const auto* object = Loaded(item);
            Target target{};
            target.item = item;
            target.occurrence = occurrence;
            if (object)
                target.descriptor = object->GetDescriptor();
            result.push_back(std::move(target));
        }
        return result;
    }
    inline bool Current(const Target& target)
    {
        const auto list = Roster(0);
        return std::find(list.begin(), list.end(), target) != list.end();
    }
    inline std::string Name(const Target& target)
    {
        const auto* object = Loaded(target.item);
        if (object && (!target.descriptor || object->GetDescriptor() != *target.descriptor))
            return "Changed native object";
        if (!object)
            return "Unavailable native object #" + std::to_string(target.item.entryIndex);
        return ParkInformation::PlainText(FormatStringIDLegacy(target.item.getName(), nullptr), true);
    }
    inline ManagementPresentation Presentation(const std::vector<Target>& roster, uint8_t filter, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Research / Invention catalog";
        model.title = Category(filter);
        model.selected = selected;
        model.stats = { { "Items", std::to_string(roster.size()) } };
        for (const auto& target : roster)
            model.rows.push_back(Name(target));
        model.rows.push_back("Change category");
        model.rows.push_back("Back to Research");
        model.status = roster.empty() ? "No native records in this category"
                                      : "Native list order; Left/Right filters, A Details";
        return model;
    }
    inline ManagementPresentation Detail(const Target& target, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Research / Invention details";
        model.selected = selected;
        model.rows = { "Back to catalog" };
        if (!Current(target))
        {
            model.title = "Research record changed";
            model.status = "Refresh catalog and choose again";
            return model;
        }
        model.title = Name(target);
        model.stats = { { "Category",
                          EnumValue(target.item.category) <= 6 ? Category(EnumValue(target.item.category) + 1)
                                                               : "Unavailable" },
                        { "Native availability",
                          Invented(target.item)     ? "Invented and loaded"
                              : Loaded(target.item) ? "Not yet invented"
                                                    : "Object missing" } };
        model.bodyLines.assign(8, "");
        model.status = Loaded(target.item) ? "Read-only native object preview" : "Native record retained; preview unavailable";
        return model;
    }
    inline void DrawPreview(Drawing::RenderTarget& rt, const Target& target)
    {
        if (!Current(target))
            return;
        const auto* object = Loaded(target.item);
        Drawing::RenderTarget preview;
        if (object && ClipRenderTarget(preview, rt, { 130, 139 }, 256, 104))
            object->DrawPreview(preview, 256, 104);
    }
} // namespace OpenRCT2::Ui::Handheld::ResearchCatalog
