// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ConceptStyle.h"
#include "Panel.h"

#include <filesystem>
#include <openrct2/Context.h>
#include <openrct2/ParkImporter.h>
#include <openrct2/core/FileStream.h>
#include <openrct2/localisation/Formatting.h>
#include <openrct2/localisation/Localisation.Date.h>
#include <openrct2/park/ParkPreview.h>
#include <optional>

namespace OpenRCT2::Ui::Handheld::SavedParkPresentation
{
    inline std::optional<ParkPreview> Read(const std::filesystem::path& path)
    {
        if (!ParkImporter::ExtensionIsOpenRCT2ParkFile(path.extension().u8string()))
            return {};
        try
        {
            auto importer = ParkImporter::CreateParkFile(GetContext()->GetObjectRepository());
            auto stream = FileStream(path.u8string(), FileMode::open);
            importer->LoadFromStream(&stream, false, true, path.u8string());
            return importer->GetParkPreview();
        }
        catch (const std::exception&)
        {
            return {};
        }
    }
    inline std::string Date(const ParkPreview& park)
    {
        const auto month = FormatStringIDLegacy(DateGameMonthNames[std::clamp(park.month, 0, 7)], nullptr);
        return std::to_string(park.day + 1) + " " + month + ", year " + std::to_string(park.year + 1);
    }
    // The most useful picture in a save: a screenshot if it has one, else the minimap, skipping
    // blank (single-colour) images.
    inline const PreviewImage* BestImage(const ParkPreview& park)
    {
        const PreviewImage* image = nullptr;
        for (const auto& candidate : park.images)
        {
            if (!candidate.width || !candidate.height)
                continue;
            const auto begin = std::begin(candidate.pixels), end = begin + candidate.width * candidate.height;
            const bool useful = std::any_of(begin, end, [&](auto colour) { return colour != *begin; });
            if (useful && (!image || candidate.type == PreviewImageType::screenshot))
                image = &candidate;
        }
        return image;
    }
    // Scales an image to fit a box, keeping its shape, centred in the box. Returns false if none.
    inline bool DrawFitted(Drawing::RenderTarget& rt, const PreviewImage* image, ScreenCoordsXY box, ScreenSize size)
    {
        if (!image)
            return false;
        const int scale = std::min(size.width * 1000 / image->width, size.height * 1000 / image->height);
        const int width = std::max(1, image->width * scale / 1000), height = std::max(1, image->height * scale / 1000);
        const ScreenCoordsXY origin{ box.x + (size.width - width) / 2, box.y + (size.height - height) / 2 };
        Drawing::RenderTarget target;
        if (ClipRenderTarget(target, rt, origin, width, height))
            for (int y = target.y; y < target.y + target.height; ++y)
                for (int x = target.x; x < target.x + target.width; ++x)
                    target.bits[(y - target.y) * target.LineStride() + x - target.x] = image->pixels
                                                                                           [y * image->height / height
                                                                                                * image->width
                                                                                            + x * image->width / width];
        return true;
    }
    // A console save slot: the park's picture, its name, the in-game date and how it is doing.
    // Empty slots are an outline that says what A will do there.
    inline void DrawSlot(
        Drawing::RenderTarget& rt, ScreenCoordsXY pos, ScreenSize size, size_t slot, const std::optional<ParkPreview>& park,
        bool exists, bool focused, bool saving)
    {
        const bool empty = !exists;
        if (empty)
        {
            OutlineRound(rt, pos, size, Theme::edge);
            if (focused)
                FillRound(rt, pos + ScreenCoordsXY{ 1, 1 }, { size.width - 2, size.height - 2 }, Theme::raised);
        }
        else
            FillRound(rt, pos, size, Theme::raised);
        if (focused)
            DrawFocusRing(rt, pos, size);
        const ScreenCoordsXY thumb = pos + ScreenCoordsXY{ 6, 6 };
        const ScreenSize thumbSize{ 92, size.height - 12 };
        if (empty)
            OutlineRound(rt, thumb, thumbSize, Theme::edge);
        else
            FillRound(rt, thumb, thumbSize, Theme::ink);
        if (!empty && park)
            DrawFitted(rt, BestImage(*park), thumb + ScreenCoordsXY{ 1, 1 }, { thumbSize.width - 2, thumbSize.height - 2 });
        const auto number = std::to_string(slot + 1);
        DrawModernText(
            rt, thumb + ScreenCoordsXY{ (thumbSize.width - TextWidth(number)) / 2, CapCentre(thumbSize.height) },
            empty || !park ? "{MUTED}" + number : "");
        const int textX = 108, textW = size.width - textX - (focused ? 28 : 10);
        const std::string label = "SLOT " + number;
        DrawModernText(
            rt, pos + ScreenCoordsXY{ size.width - 10 - TextWidth(label) - (focused ? 18 : 0), 8 }, "{MUTED}" + label);
        if (empty)
            DrawStrongText(
                rt, pos + ScreenCoordsXY{ textX, CapCentre(size.height) }, saving ? "{WHITE}Save here" : "{MUTED}Empty slot",
                textW);
        else if (!park)
            DrawStrongText(rt, pos + ScreenCoordsXY{ textX, CapCentre(size.height) }, "{WHITE}Saved park", textW);
        else
        {
            DrawStrongText(rt, pos + ScreenCoordsXY{ textX, 10 }, "{WHITE}" + park->parkName, textW - TextWidth(label) - 8);
            DrawModernEllipsised(rt, pos + ScreenCoordsXY{ textX, 27 }, textW, "{MUTED}" + Date(*park));
            DrawModernEllipsised(
                rt, pos + ScreenCoordsXY{ textX, 43 }, textW,
                "{MUTED}" + std::to_string(park->numGuests) + " guests  |  " + std::to_string(park->numRides)
                    + " rides  |  rating " + std::to_string(park->parkRating));
        }
        if (focused && (saving || !empty)) // Nothing to load from an empty slot.
            DrawButtonGlyph(rt, pos + ScreenCoordsXY{ size.width - 14, size.height / 2 - 1 }, "A");
    }
    inline void Draw(Drawing::RenderTarget& rt, const ParkPreview& park)
    {
        DrawWrappedLabel(rt, { 258, 50 }, 236, park.parkName, 2);
        if (!DrawFitted(rt, BestImage(park), { 258, 90 }, { 236, 156 }))
            DrawModernText(rt, { 258, 154 }, "Preview unavailable", {}, 236);
        DrawModernText(rt, { 258, 262 }, Date(park), {}, 236);
        DrawModernText(
            rt, { 258, 287 }, "Guests " + std::to_string(park.numGuests) + " / Rides " + std::to_string(park.numRides), {},
            236);
        DrawModernText(rt, { 258, 312 }, "Park rating " + std::to_string(park.parkRating), {}, 236);
    }
} // namespace OpenRCT2::Ui::Handheld::SavedParkPresentation
