// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ManagementPanel.h"
#include "ObjectPicker.h"
#include "Shops.h"
#include "FlatRides.h"
#include "Mazes.h"
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
            return "Item changed";
        if (!object)
            return "Unavailable item #" + std::to_string(target.item.entryIndex);
        return ParkInformation::PlainText(FormatStringIDLegacy(target.item.getName(), nullptr), true);
    }
    inline ObjectPicker Picker(const std::vector<Target>& roster, size_t selected)
    {
        std::vector<PickerEntry> entries;
        for (const auto& target : roster)
        {
            PickerEntry entry{{target.item.type == Research::EntryType::ride ? ObjectType::ride : ObjectType::sceneryGroup,
                target.item.entryIndex, std::to_string(target.item.rawValue) + ":" + std::to_string(target.occurrence)}, Name(target), target.item.type == Research::EntryType::ride ? ZoomLevel{1} : ZoomLevel{2}};
            // The picker's own detail pane says what it is and whether you have it yet.
            entry.detail = Category(EnumValue(target.item.category) + 1) + ". "
                + (Invented(target.item) ? "Researched" : Loaded(target.item) ? "Not researched yet" : "Object unavailable");
            entries.push_back(std::move(entry));
        }
        std::optional<PickerIdentity> preferred;
        if (selected < entries.size()) preferred = entries[selected].identity;
        ObjectPicker picker; picker.setEntries(std::move(entries),preferred); return picker;
    }
    enum class BuildKind { none, shop, flat, maze, track };
    inline BuildKind Construction(const Target& target)
    {
        if (!Current(target) || !Invented(target.item) || target.item.type != Research::EntryType::ride) return BuildKind::none;
        const auto i = target.item.entryIndex;
        if (Shops::Eligible(i)) return BuildKind::shop;
        if (FlatRides::Eligible(i)) return BuildKind::flat;
        if (Mazes::Eligible(i)) return BuildKind::maze;
        if (TrackConstructionSupport::Available(i,target.item.baseRideType)) return BuildKind::track;
        return BuildKind::none;
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

        model.status = roster.empty() ? "No items in this category"
                                      : "Left/Right category / A Details";
        return model;
    }
    inline ManagementPresentation Detail(const Target& target, size_t selected)
    {
        ManagementPresentation model;
        model.breadcrumb = "Research / Invention details";
        model.selected = selected;
        model.rows = Construction(target) == BuildKind::none ? std::vector<std::string>{"Back to catalog"}
            : std::vector<std::string>{"Build this item", "Back to catalog"};
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
                        { "Availability",
                          Invented(target.item)     ? "Researched"
                              : Loaded(target.item) ? "Not researched"
                                                    : "Object missing" } };
        model.bodyLines.assign(8, "");
        model.status = Loaded(target.item) ? "Researched item" : "Preview unavailable";
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
