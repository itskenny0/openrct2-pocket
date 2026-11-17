// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <functional>
#include <openrct2-ui/handheld/ParkManagementTabs.h>
#include <openrct2-ui/handheld/PeopleTabs.h>

// Layout and native semantic boundaries; call from the initialized runtime driver.
static void CheckVisualPolishLayout()
{
    const std::vector<std::vector<H::ControlHint>> cases{
        { { "D-pad", "Section" },
          { "A", "Enter" },
          { "B", "Park" },
          { "L1/R1", "Section" },
          { "SELECT", "Enter", H::HintPriority::guide } },
        { { "D-pad", "Choose" },
          { "A", "Select" },
          { "B", "Back" },
          { "L1/R1", "Tab" },
          { "X", "Access" },
          { "SELECT", "Sections", H::HintPriority::guide } },
        { { "A", "Build" }, { "B", "Undo" }, { "L1/R1", "Piece" }, { "X", "Speed +" }, { "Y", "Speed -" } },
        { { "D-pad", "Move" },
          { "A", "Review" },
          { "B", "Cancel" },
          { "Y", "Adjust" },
          { "SELECT", "Surface", H::HintPriority::guide } },
        { { "D-pad", "Move" },
          { "A", "Place" },
          { "B", "Back" },
          { "X", "Remove" },
          { "Y", "Adjust" },
          { "SELECT", "Surface", H::HintPriority::guide } },
        { { "D-pad", "Adjust" }, { "A", "Build" }, { "B", "Build" }, { "L1/R1", "Group" } },
        { { "D-pad", "Move" }, { "A", "Apply" }, { "B", "Back" }, { "Y", "Add/Remove" } },
        { { "D-pad", "Choose" }, { "A", "Confirm" }, { "B", "Cancel" } },
        { { "D-pad", "Sample" },
          { "A", "Graph" },
          { "B", "Back" },
          { "L1/R1", "Tab" },
          { "SELECT", "Sections", H::HintPriority::guide } }
    };
    for (const auto& hints : cases)
    {
        auto layout = H::MeasureFooter({ 512, 384 }, hints);
        Check(layout.size() == H::FooterHints(hints, 496).size(), "Footer lost a visible action");
        Check(H::FooterHeight({ 512, 384 }, hints) == 28, "Ordinary footer expanded beyond one row");
        std::sort(
            layout.begin(), layout.end(), [](const auto& a, const auto& b) { return a.y == b.y ? a.x < b.x : a.y < b.y; });
        std::array<int, 2> right{ 0, 0 };
        for (const auto& item : layout)
        {
            const auto lane = item.y < 365 ? 0 : 1;
            Check(item.x >= right[lane] && item.x + item.width <= 504, "Footer overlaps or clips at logical resolution");
            Check(item.y - 3 >= 340 && item.y + 14 <= 384, "Footer glyph is outside its reserved area");
            right[lane] = item.x + item.width;
        }
    }
    Check(H::FooterHeight({ 512, 384 }, cases[7]) == 28, "Simple dialog should use one footer row");
    H::ObjectPicker picker;
    std::vector<H::PickerEntry> entries;
    for (int i = 0; i < 21; ++i)
        entries.push_back({ { ObjectType::ride, static_cast<ObjectEntryIndex>(i) }, "Item" });
    picker.setEntries(entries);
    picker.move(0, 2);
    Check(picker.focus() == 6, "Shoulder did not advance one catalog page");
    picker.move(0, -2);
    Check(picker.focus() == 0, "Catalog reverse page lost focus");
    picker.move(0, -2);
    Check(picker.focus() == 0, "Catalog paging wrapped at top");
    Check(H::ControlKey("L/R") == "L1/R1" && H::ControlKey("L2/R2") == "L2/R2", "Shoulder generations ambiguous");
    Check(
        H::PeopleTabs::ThoughtLabel(
            "\xE2\x80\x9C"
            "I am hungry"
            "\xE2\x80\x9D")
            == "\"I am hungry\"",
        "Native thought punctuation forced the small fallback font");
    H::ManagementPresentation model;
    model.actionGrid = true;
    model.summaryMetrics = true;
    model.metricColumns = 2;
    model.metricHeight = 36;
    model.rowHeight = 24;
    model.tabs = { "Project", "Funding & priorities" };
    model.stats = { { "Funding", "Normal" }, { "Cost / month", "100" } };
    model.bodyLines = { "Draft" };
    model.rows.resize(9);
    Check(
        H::MeasureManagement({ 496, H::Layout::ContentHeight({ 512, 384 }) }, model).capacity >= 9,
        "Research discard/review moved off the visible grid");
    model.metricColumns = 3;
    model.metricHeight = 38;
    model.rowHeight = 26;
    model.stats.resize(6);
    model.bodyLines.clear();
    model.rows.resize(7);
    Check(
        H::MeasureManagement({ 496, H::Layout::ContentHeight({ 512, 384 }) }, model).capacity >= 7,
        "Ride Overview unnecessarily paged its final action");
}
static void CheckGuestNeedBoundaries(Guest& guest, const std::function<void(const char*)>& capture)
{
    const auto savedHunger = guest.hunger, savedThirst = guest.thirst, savedToilet = guest.toilet, savedNausea = guest.nausea;
    const auto savedEnergy = guest.energy, savedHappiness = guest.happiness;
    const auto target = H::GuestManagement::Identify(guest.id);
    Check(target.has_value(), "Guest semantic fixture missing");
    H::PeopleTabs::State tabs;
    tabs.tab = 1;
    guest.hunger = 190;
    guest.thirst = 190;
    guest.toilet = 0;
    guest.nausea = 0;
    guest.energy = 128;
    guest.happiness = 255;
    auto m = H::PeopleTabs::Guest(*target, tabs, 0);
    Check(
        m.meters[0] == 100 && m.meters[2] == 0 && !m.meterAttention[2] && !m.meterAttention[5],
        "Good native need values interpreted backwards");
    Check(m.meterDirections == std::vector<int>({ 1, 1, -1, -1, -1, -1 }), "Need direction semantics lost");
    capture("10a-guest-needs-good.png");
    guest.toilet = 159;
    guest.nausea = 139;
    m = H::PeopleTabs::Guest(*target, tabs, 0);
    Check(!m.meterAttention[4] && !m.meterAttention[5], "Attention threshold invented below native trigger");
    guest.toilet = 160;
    guest.nausea = 140;
    m = H::PeopleTabs::Guest(*target, tabs, 0);
    Check(m.meterAttention[4] && m.meterAttention[5], "Native toilet/nausea attention trigger missing");
    guest.hunger = 0;
    guest.thirst = 0;
    guest.toilet = 242;
    guest.nausea = 255;
    guest.energy = 32;
    guest.happiness = 0;
    capture("10b-guest-needs-attention.png");
    guest.hunger = savedHunger;
    guest.thirst = savedThirst;
    guest.toilet = savedToilet;
    guest.nausea = savedNausea;
    guest.energy = savedEnergy;
    guest.happiness = savedHappiness;
}
