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
        banners
    };
    enum class Mode
    {
        place,
        recolour,
        remove
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
    };
    bool Eligible(Category category, ObjectEntryIndex object);
    std::vector<PickerEntry> Entries(Category category);
    std::string CategoryName(Category category);
    std::string Name(const Settings& settings);
    std::array<bool, 3> Channels(Category category, ObjectEntryIndex object);
    std::vector<std::string> Options();
    void Configure(size_t row, int delta = 1);
    const Settings& Current();
    void SelectCategory(Category category);
    bool Select(ObjectEntryIndex object);
    std::optional<Target> Capture(const InteractionInfo& info);
    GameActions::Result Modify(const Target& target, Mode mode, const std::array<Drawing::Colour, 3>& colours, bool execute);
    GameActions::Result SetText(const Target& target, const std::string& text, bool execute);
    GameActions::Result Place(std::optional<CoordsXY> tile, const Settings& settings, bool execute);
    void UpdatePreview(std::optional<CoordsXY> tile);
    void ClearPreview();
    void RemovePreviewTemporarily();
    void RestorePreview();
} // namespace OpenRCT2::Ui::Handheld::SceneryTools
