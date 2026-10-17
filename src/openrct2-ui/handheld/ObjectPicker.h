// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Panel.h"

#include <algorithm>
#include <functional>
#include <openrct2/Context.h>
#include <openrct2/object/Object.h>
#include <openrct2/object/ObjectManager.h>
#include <optional>
#include <vector>

namespace OpenRCT2::Ui::Handheld
{
    struct PickerIdentity
    {
        ObjectType type;
        ObjectEntryIndex index;
        std::string key{}; // Stable native descriptor or file-backed choice.
        bool operator==(const PickerIdentity& other) const
        {
            if (type != other.type)
                return false;
            return !key.empty() || !other.key.empty() ? key == other.key : index == other.index;
        }
    };
    struct PickerEntry
    {
        PickerIdentity identity;
        std::string name;
        ZoomLevel previewZoom{ 2 };
        std::function<void(Drawing::RenderTarget&)> drawPreview{};
    };
    // Presentation/navigation only. Each caller supplies its eligible objects and activation.
    class ObjectPicker
    {
    public:
        static constexpr size_t columns = 3, pageSize = 6;
        void setEntries(std::vector<PickerEntry> entries, std::optional<PickerIdentity> preferred = {})
        {
            if (!preferred && selected())
                preferred = selected()->identity;
            _entries = std::move(entries);
            if (preferred)
            {
                const auto it = std::find_if(
                    _entries.begin(), _entries.end(), [&](const auto& e) { return e.identity == *preferred; });
                if (it != _entries.end())
                    _focus = static_cast<size_t>(it - _entries.begin());
            }
            _focus = _entries.empty() ? 0 : std::min(_focus, _entries.size() - 1);
        }
        void move(int x, int y)
        {
            if (_entries.empty())
                return;
            if (y != 0)
            {
                const auto next = static_cast<int64_t>(_focus) + static_cast<int64_t>(columns) * (y > 0 ? 1 : -1);
                if (next >= 0 && next < static_cast<int64_t>(_entries.size()))
                    _focus = static_cast<size_t>(next);
                else if (y > 0 && _focus + columns - _focus % columns < _entries.size())
                    _focus = _entries.size() - 1;
            }
            else if (x != 0)
            {
                if (x > 0 && _focus % columns < columns - 1 && _focus + 1 < _entries.size())
                    ++_focus;
                else if (x < 0 && _focus % columns > 0)
                    --_focus;
            }
        }
        const PickerEntry* selected() const
        {
            return _entries.empty() ? nullptr : &_entries[_focus];
        }
        const std::vector<PickerEntry>& entries() const
        {
            return _entries;
        }
        size_t focus() const
        {
            return _focus;
        }
        size_t page() const
        {
            return _focus / pageSize;
        }
        size_t pages() const
        {
            return std::max<size_t>(1, (_entries.size() + pageSize - 1) / pageSize);
        }

    private:
        std::vector<PickerEntry> _entries;
        size_t _focus{};
    };

    inline void DrawObjectPicker(
        Drawing::RenderTarget& rt, ScreenSize screen, const ObjectPicker& picker, const std::string& title,
        const std::string& status = {}, const std::string& emptyLabel = "No objects available", bool searchControl = false,
        const std::string& startAction = "Pause")
    {
        using namespace Concept;
        const auto p = kMenuOrigin;
        const auto& entries = picker.entries();
        const auto start = picker.page() * ObjectPicker::pageSize;
        const auto visible = std::min(ObjectPicker::pageSize, entries.size() - start);
        const int detailTop = 40 + static_cast<int>((visible + ObjectPicker::columns - 1) / ObjectPicker::columns) * 92;
        Box(rt, p, { 244, detailTop + 62 });
        Heading(rt, p + ScreenCoordsXY{ 12, 12 }, title);
        DrawModernText(
            rt, p + ScreenCoordsXY{ 194, 12 },
            "{MUTED}" + std::to_string(picker.page() + 1) + "/" + std::to_string(picker.pages()));
        for (size_t slot = 0; slot < ObjectPicker::pageSize && start + slot < entries.size(); ++slot)
        {
            const auto index = start + slot;
            const bool selected = index == picker.focus();
            const auto cell = p + ScreenCoordsXY{ 12 + static_cast<int>(slot % 3) * 76, 36 + static_cast<int>(slot / 3) * 92 };
            Box(rt, cell, { 68, 84 }, selected);
            // The same loaded Object::DrawPreview path as desktop object selection.
            // Native sprite minification fits tall objects without an asset pipeline.
            if (entries[index].drawPreview)
            {
                Drawing::RenderTarget preview;
                if (ClipRenderTarget(preview, rt, cell + ScreenCoordsXY{ 4, 4 }, 60, 62))
                    entries[index].drawPreview(preview);
            }
            else if (
                const auto* object = GetContext()->GetObjectManager().GetLoadedObject(
                    entries[index].identity.type, entries[index].identity.index))
            {
                Drawing::RenderTarget preview;
                if (ClipRenderTarget(preview, rt, cell + ScreenCoordsXY{ 4, 4 }, 60, 62))
                {
                    preview.zoom_level = entries[index].previewZoom;
                    object->DrawPreview(preview, preview.zoom_level.ApplyTo(60), preview.zoom_level.ApplyTo(62));
                }
            }
            DrawModernEllipsised(
                rt, cell + ScreenCoordsXY{ 4, 70 }, 60, (selected ? "{BLACK}" : "{WHITE}") + entries[index].name);
        }
        Rule(rt, p + ScreenCoordsXY{ 12, detailTop }, 220);
        const auto* selected = picker.selected();
        DrawStrongText(rt, p + ScreenCoordsXY{ 12, detailTop + 12 }, selected ? selected->name : emptyLabel, 220);
        DrawModernText(
            rt, p + ScreenCoordsXY{ 12, detailTop + 36 },
            "{MUTED}" + (status.empty() ? std::to_string(entries.size()) + " available" : status), {}, 220);
        DrawActionStrip(rt, screen);
        const auto y = screen.height - 21;
        DrawCommand(rt, { 12, y }, 38, "D-pad", "Choose");
        DrawCommand(rt, { 140, y }, 17, "A", "Select");
        DrawCommand(rt, { 268, y }, 17, "B", "Back");
        if (searchControl) DrawCommand(rt, {396,y},17,"X","Search");
        else DrawCommand(rt, { 396, y }, 38, "START", startAction.c_str());
    }
} // namespace OpenRCT2::Ui::Handheld
