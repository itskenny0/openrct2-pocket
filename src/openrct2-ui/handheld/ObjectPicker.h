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
    // Full-screen catalog: a three-column grid of previews on the left (the same navigation as
    // before), and on the right a large preview of the focused object with its name, description
    // and status. Optional tabs across the top for categories (the ride picker's types).
    inline void DrawObjectPicker(
        Drawing::RenderTarget& rt, ScreenSize screen, const ObjectPicker& picker, const std::string& title,
        const std::string& status = {}, const std::string& emptyLabel = "No objects available", bool searchControl = false,
        const std::string& startAction = "Pause", const std::string& extraAction = "", const std::string& groupAction = "",
        const std::vector<std::string>& tabs = {}, size_t selectedTab = 0)
    {
        const ScreenCoordsXY p{ 8, 8 };
        const int width = screen.width - 16;
        const int height = Layout::ContentHeight(screen);
        const auto& entries = picker.entries();
        const auto* focused = picker.selected();
        DrawSurface(rt, p, { width, height });
        DrawHeadingText(rt, p + ScreenCoordsXY{ 12, 11 }, "{WHITE}" + title, width - 120);
        const std::string count = std::to_string(entries.empty() ? 0 : picker.focus() + 1) + " of "
            + std::to_string(entries.size());
        DrawModernText(rt, p + ScreenCoordsXY{ width - 12 - TextWidth(count), 15 }, "{MUTED}" + count);
        Fill(rt, p + ScreenCoordsXY{ 12, 37 }, { width - 24, 1 }, Theme::edge);
        int top = 44;
        if (!tabs.empty())
        {
            DrawTabs(rt, p + ScreenCoordsXY{ 10, top }, width - 20, tabs, selectedTab);
            top += 32;
        }
        // Grid.
        constexpr int cellW = 94, cellH = 84, pitchX = 100, pitchY = 90;
        const int gridHeight = height - top - 8;
        const int offset = ListOffset(picker.focus() / 3, (entries.size() + 2) / 3, gridHeight, pitchY);
        const auto start = static_cast<size_t>(offset / pitchY) * 3;
        Drawing::RenderTarget gridRt;
        if (ClipRenderTarget(gridRt, rt, p + ScreenCoordsXY{ 0, top }, 12 + 3 * pitchX, gridHeight))
        {
            for (size_t slot = 0; start + slot < entries.size() && static_cast<int>(slot / 3) * pitchY < gridHeight + pitchY;
                 ++slot)
            {
                const auto index = start + slot;
                const bool selected = index == picker.focus();
                const auto cell = ScreenCoordsXY{ 12 + static_cast<int>(slot % 3) * pitchX,
                                                  static_cast<int>((start + slot) / 3) * pitchY - offset + 2 };
                FillRound(gridRt, cell, { cellW, cellH }, Theme::raised);
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
                DrawModernEllipsised(gridRt, cell + ScreenCoordsXY{ 5, 69 }, 84, "{WHITE}" + cellName);
                if (selected)
                    DrawFocusRing(gridRt, cell, { cellW, cellH });
            }
        }
        // Detail pane.
        const int paneX = 12 + 3 * pitchX + 4;
        const int paneW = width - paneX - 10;
        const auto pane = p + ScreenCoordsXY{ paneX, top + 2 };
        FillRound(rt, pane, { paneW, gridHeight - 4 }, Theme::raised);
        if (focused)
        {
            const int artH = std::min(120, (gridHeight - 4) / 2);
            Drawing::RenderTarget art;
            if (ClipRenderTarget(art, rt, pane + ScreenCoordsXY{ 6, 6 }, paneW - 12, artH))
            {
                FillRound(art, { 0, 0 }, { art.width, art.height }, Theme::ink);
                if (focused->drawPreview)
                {
                    Drawing::RenderTarget inner;
                    if (ClipRenderTarget(inner, art, { (art.width - 86) / 2, (art.height - 62) / 2 }, 86, 62))
                        focused->drawPreview(inner);
                }
                else if (
                    const auto* object = GetContext()->GetObjectManager().GetLoadedObject(
                        focused->identity.type, focused->identity.index))
                {
                    // Full-size preview where it fits (ride previews are 112 px squares).
                    Drawing::RenderTarget inner;
                    const int side = std::min({ 112, art.width, art.height });
                    if (focused->identity.type == ObjectType::ride
                        && ClipRenderTarget(inner, art, { (art.width - side) / 2, (art.height - side) / 2 }, side, side))
                        object->DrawPreview(inner, side, side);
                    else
                        object->DrawPreview(art, art.width, art.height);
                }
            }
            int y = artH + 12;
            // With reduced motion a long name wraps instead of freezing mid-scroll.
            const bool wrapName = MenuFeel::reduceMotion && TextWidth(focused->name, true) > paneW - 16;
            if (wrapName)
                DrawWrappedLabel(rt, pane + ScreenCoordsXY{ 8, y }, paneW - 16, "{WHITE}" + focused->name, 2);
            else
                DrawMarquee(rt, pane + ScreenCoordsXY{ 8, y }, paneW - 16, "{WHITE}", focused->name, MarqueeTime(), true);
            y += wrapName ? 36 : 20;
            if (!focused->detail.empty())
            {
                const int lines = std::max(1, (gridHeight - 4 - y - 30) / 16);
                DrawWrappedLabel(rt, pane + ScreenCoordsXY{ 8, y }, paneW - 16, "{MUTED}" + focused->detail, lines);
            }
        }
        else
            DrawWrappedLabel(rt, pane + ScreenCoordsXY{ 8, 12 }, paneW - 16, "{MUTED}" + emptyLabel, 3);
        const auto statusLine = status.empty() ? std::to_string(entries.size()) + " available" : status;
        DrawModernEllipsised(rt, pane + ScreenCoordsXY{ 8, gridHeight - 24 }, paneW - 16, "{MUTED}" + statusLine);
        std::vector<ControlHint> hints{ { "D-pad", "Choose" }, { "A", "Select" }, { "B", "Back" } };
        if (!tabs.empty())
            hints.push_back({ "L1/R1", "Type" });
        else
            hints.push_back({ "L1/R1", "Page", HintPriority::secondary });
        hints.push_back(
            { searchControl ? "X" : "START", searchControl ? "Search" : startAction,
              searchControl ? HintPriority::normal : HintPriority::guide });
        if (!extraAction.empty())
            hints.push_back({ "X", extraAction });
        if (!groupAction.empty() && tabs.empty())
            hints.push_back({ "Y", groupAction });
        DrawFooter(rt, screen, hints);
    }
} // namespace OpenRCT2::Ui::Handheld
