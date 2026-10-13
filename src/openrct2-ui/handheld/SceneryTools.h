// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ObjectPicker.h"

#include <array>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/drawing/Colour.h>
#include <openrct2/interface/Viewport.h>
#include <openrct2/world/Scenery.h>
namespace OpenRCT2::Ui::Handheld::SceneryTools
{
    enum class Category
    {
        small,
        large,
        walls,
        banners,
        additions
    };
    enum class Mode
    {
        place,
        recolour,
        remove,
        pick
    };
    struct Settings
    {
        Category category{};
        ObjectEntryIndex object = kObjectEntryIndexNull;
        Direction direction = 3;
        uint8_t quadrant{};
        int32_t height{}; // zero = native automatic ground/path height
        std::array<Drawing::Colour, 3> colours{ Drawing::Colour::bordeauxRed, Drawing::Colour::yellow,
                                                Drawing::Colour::darkBrown };
        Mode mode{};
        bool scatter{};
        uint16_t scatterSize = 1;
        ScatterToolDensity scatterDensity{};
        bool operator==(const Settings&) const = default;
    };
    struct Target
    {
        Category category;
        CoordsXYZD location;
        ObjectEntryIndex object;
        uint8_t part{}; // native quadrant, sequence, or banner edge
        BannerIndex banner = BannerIndex::GetNull();
        std::array<Drawing::Colour, 3> colours{};
        std::string text{};
        uint8_t bannerFlags{}, textColour{};
        RideId linkedRide = RideId::GetNull();
        std::string descriptor;
        bool operator==(const Target&) const = default;
    };
    struct Group
    {
        ObjectEntryIndex index;
        std::string name;
    };
    std::vector<Group> Groups(Category category);
    bool SetGroup(ObjectEntryIndex group);
    std::string GroupName();
    const std::string& Search();
    bool SetSearch(const std::string& value);
    enum class Style
    {
        noEntry,
        textColour,
        mainColour
    };
    struct StyleDraft
    {
        Target target;
        Style style;
        uint8_t value;
    };
    std::optional<StyleDraft> BeginStyle(const Target& target, Style style);
    void AdjustStyle(StyleDraft& draft, int delta);
    std::string StyleValue(const StyleDraft& draft);
    GameActions::Result ApplyStyle(const StyleDraft& draft, bool execute);
    bool Eligible(Category category, ObjectEntryIndex object);
    std::vector<PickerEntry> Entries(Category category);
    std::string CategoryName(Category category);
    std::vector<std::string> Categories();
    std::string Name(const Settings& settings);
    std::array<bool, 3> Channels(Category category, ObjectEntryIndex object);
    std::vector<std::string> Options();
    void DrawOverlay(Drawing::RenderTarget& rt, ScreenCoordsXY position, const std::optional<Target>& hovered = {});
    void Configure(size_t row, int delta = 1);
    const Settings& Current();
    void SelectCategory(Category category);
    bool Select(ObjectEntryIndex object, const std::string& expectedDescriptor = {});
    bool ScatterActive();
    bool ScatterReviewed();
    std::string ScatterStatus();
    std::pair<CoordsXY, CoordsXY> ScatterBounds(CoordsXY centre);
    void ResetScatter();
    void BeginPick();
    GameActions::Result Pick(const Target& target, bool execute);
    std::optional<Target> Capture(const InteractionInfo& info);
    GameActions::Result Modify(const Target& target, Mode mode, const std::array<Drawing::Colour, 3>& colours, bool execute);
    GameActions::Result SetText(const Target& target, const std::string& text, bool execute);
    GameActions::Result Place(std::optional<CoordsXY> tile, const Settings& settings, bool execute);
    void UpdatePreview(std::optional<CoordsXY> tile);
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
} // namespace OpenRCT2::Ui::Handheld::SceneryTools
