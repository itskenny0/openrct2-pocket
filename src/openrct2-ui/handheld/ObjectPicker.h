// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "MenuFeel.h"
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
        std::string label{};  // Short name for the grid cell; the full name shows in the detail pane.
        std::string detail{}; // Optional description shown under the full name.
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
        void pageBy(int direction)
        {
            if (_entries.empty())
                return;
            _focus = static_cast<size_t>(std::clamp<int64_t>(
                static_cast<int64_t>(_focus) + direction * static_cast<int64_t>(pageSize), 0, _entries.size() - 1));
        }
        void move(int x, int y)
        {
            if (y > 1 || y < -1)
            {
                pageBy(y > 0 ? 1 : -1);
                return;
            }
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

    // Marquees restart when focus moves, so each name begins from its first letter.
    inline uint64_t MarqueeTime()
    {
        return activeMenuFeel != nullptr ? activeMenuFeel->FocusElapsed() : MenuFeel::Clock();
    }
    inline void DrawObjectPicker(
        Drawing::RenderTarget& rt, ScreenSize screen, const ObjectPicker& picker, const std::string& title,
        const std::string& status = {}, const std::string& emptyLabel = "No objects available", bool searchControl = false,
        const std::string& startAction = "Pause", const std::string& extraAction = "", const std::string& groupAction = "")
    {
        using namespace Concept;
        const auto p = kMenuOrigin;
        const auto& entries = picker.entries();
        const int viewportHeight=184;
        const int offset=ListOffset(picker.focus()/3,(entries.size()+2)/3,viewportHeight,92);
        const auto start=static_cast<size_t>(offset/92)*3;
        const int detailTop=224;
        const auto* focused = picker.selected();
        const bool described = focused != nullptr && !focused->detail.empty();
        Box(rt, p, { 320, detailTop + (described ? 92 : 78) });
        Heading(rt, p + ScreenCoordsXY{ 12, 12 }, title);
        DrawModernText(
            rt, p + ScreenCoordsXY{ 194, 12 },
            "{MUTED}" + std::to_string(entries.empty()?0:picker.focus()+1) + " / " + std::to_string(entries.size()));
        Drawing::RenderTarget gridRt;
        if(ClipRenderTarget(gridRt,rt,p+ScreenCoordsXY{0,36},320,viewportHeight))
        {
        for (size_t slot = 0; slot < ObjectPicker::pageSize+ObjectPicker::columns && start + slot < entries.size(); ++slot)
        {
            const auto index = start + slot;
            const bool selected = index == picker.focus();
            const auto cell=ScreenCoordsXY{12+static_cast<int>(slot%3)*100,static_cast<int>((start+slot)/3)*92-offset};
            Box(gridRt, cell, { 94, 84 }, selected);
            // The same loaded Object::DrawPreview path as desktop object selection.
            // Native sprite minification fits tall objects without an asset pipeline.
            if (entries[index].drawPreview)
            {
                Drawing::RenderTarget preview;
                if (ClipRenderTarget(preview, gridRt, cell + ScreenCoordsXY{ 4, 4 }, 86, 62))
                    entries[index].drawPreview(preview);
            }
            else if (
                const auto* object = GetContext()->GetObjectManager().GetLoadedObject(
                    entries[index].identity.type, entries[index].identity.index))
            {
                Drawing::RenderTarget preview;
                if (ClipRenderTarget(preview, gridRt, cell + ScreenCoordsXY{ 4, 4 }, 86, 62))
                {
                    preview.zoom_level = entries[index].previewZoom;
                    object->DrawPreview(preview, preview.zoom_level.ApplyTo(86), preview.zoom_level.ApplyTo(62));
                }
            }
            const auto& cellName = entries[index].label.empty() ? entries[index].name : entries[index].label;
            if (selected)
                DrawMarquee(gridRt, cell + ScreenCoordsXY{ 5, 70 }, 84, "{BLACK}", cellName, MarqueeTime());
            else
                DrawModernEllipsised(gridRt, cell + ScreenCoordsXY{ 5, 70 }, 84, "{WHITE}" + cellName);
        }
        }
        Rule(rt, p + ScreenCoordsXY{ 12, detailTop }, 296);
        const auto* selected = focused;
        const auto statusLine = "{MUTED}" + (status.empty() ? std::to_string(entries.size()) + " available" : status);
        if (described)
        {
            DrawMarquee(rt, p + ScreenCoordsXY{ 12, detailTop + 10 }, 296, "{WHITE}", selected->name, MarqueeTime(), true);
            DrawWrappedLabel(rt, p + ScreenCoordsXY{ 12, detailTop + 30 }, 296, "{MUTED}" + selected->detail, 2);
            DrawModernText(rt, p + ScreenCoordsXY{ 12, detailTop + 68 }, statusLine, {}, 296);
        }
        else
        {
            DrawWrappedLabel(rt, p + ScreenCoordsXY{ 12, detailTop + 12 }, 296, selected ? selected->name : emptyLabel, 2);
            DrawModernText(rt, p + ScreenCoordsXY{ 12, detailTop + 54 }, statusLine, {}, 296);
        }
        std::vector<ControlHint> hints{ { "D-pad", "Choose" },
                                        { "A", "Select" },
                                        { "B", "Back" },
                                        { "L1/R1", "Browse" },
                                        { searchControl ? "X" : "START", searchControl ? "Search" : startAction,
                                          searchControl ? HintPriority::normal : HintPriority::guide } };
        if (!extraAction.empty())
            hints.push_back({ "X", extraAction });
        if (!groupAction.empty())
            hints.push_back({ "Y", groupAction });
        DrawFooter(rt, screen, hints);
    }
} // namespace OpenRCT2::Ui::Handheld
