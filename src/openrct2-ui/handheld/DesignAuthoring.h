// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "RideManagement.h"
#include "TextEntry.h"

#include <filesystem>
#include <openrct2/Context.h>
#include <openrct2/PlatformEnvironment.h>
#include <openrct2/core/File.h>
#include <openrct2/core/MemoryStream.h>
#include <openrct2/rct2/T6Exporter.h>
#include <openrct2/ride/TrackDesignRepository.h>

namespace OpenRCT2::Ui::Handheld::DesignAuthoring
{
    namespace fs = std::filesystem;
    inline std::optional<RideManagement::Target> target;
    inline std::unique_ptr<TrackDesign> design;
    inline std::vector<uint8_t> baseline;
    inline std::optional<std::vector<uint8_t>> destinationBaseline;
    inline fs::path destination;
    inline bool reviewed{};
    inline RideStatus status{};
    inline std::string sourceName;
    inline uint64_t serial{};

    inline GameActions::Result Error(std::string message)
    {
        GameActions::Result result;
        result.error = GameActions::Status::disallowed;
        result.errorMessage = std::move(message);
        return result;
    }
    inline std::unique_ptr<TrackDesign> Capture(const RideManagement::Target& source)
    {
        const auto* ride = RideManagement::Resolve(source);
        if (!ride || !ride->flags.has(RideFlag::tested) || !RideHasRatings(*ride))
            return nullptr;
        TrackDesignState state{};
        return ride->saveToTrackDesign(state);
    }
    // Compare the actual native exported payload, never struct padding or map pointers.
    inline std::vector<uint8_t> Bytes(const TrackDesign& source)
    {
        MemoryStream stream;
        if (!RCT2::T6Exporter(source).SaveTrack(&stream))
            throw std::runtime_error("Native design exporter failed");
        const auto* begin = static_cast<const uint8_t*>(stream.GetData());
        return { begin, begin + stream.GetLength() };
    }
    inline bool Active()
    {
        return target.has_value() && design != nullptr;
    }
    inline bool Valid()
    {
        if (!Active())
            return false;
        const auto* ride = RideManagement::Resolve(*target);
        if (!ride || ride->status != status || ride->getName() != sourceName)
            return false;
        try
        {
            const auto current = Capture(*target);
            return current && Bytes(*current) == baseline;
        }
        catch (...)
        {
            return false;
        }
    }
    inline void CancelReview()
    {
        reviewed = false;
        destination.clear();
        destinationBaseline.reset();
    }
    inline void End()
    {
        CancelReview();
        target.reset();
        design.reset();
        baseline.clear();
    }
    inline GameActions::Result Begin(const RideManagement::Target& source)
    {
        End();
        try
        {
            design = Capture(source);
            if (!design)
                return Error("Requires a tested ride with native ratings");
            baseline = Bytes(*design);
            target = source;
            const auto* ride = RideManagement::Resolve(source);
            status = ride->status;
            sourceName = ride->getName();
            return {};
        }
        catch (const std::exception& e)
        {
            End();
            return Error(e.what());
        }
    }
    inline std::string DefaultName()
    {
        const auto* ride = target ? RideManagement::Resolve(*target) : nullptr;
        TextEntry entry{ ride ? ride->getName() : "Pocket design" };
        for (auto& c : entry.text)
            if (static_cast<unsigned char>(c) < 32 || std::string("/\\:*?\"<>|").find(c) != std::string::npos)
                c = '_';
        while (entry.Length() > TextEntry::limit)
            entry.Press(43);
        while (!entry.text.empty() && (entry.text.back() == '.' || entry.text.back() == ' '))
            entry.text.pop_back();
        return entry.text.empty() ? "Pocket design" : entry.text;
    }
    inline bool SafeName(const std::string& name)
    {
        if (name.empty() || TextEntry{ name }.Length() > TextEntry::limit || name.back() == '.' || name.back() == ' ')
            return false;
        for (unsigned char c : name)
            if (c < 32 || c == 127 || std::string("/\\:*?\"<>|").find(c) != std::string::npos)
                return false;
        return true;
    }
    inline std::optional<std::vector<uint8_t>> Existing(const fs::path& path)
    {
        if (fs::is_symlink(path))
            throw std::runtime_error("Symbolic-link destinations are unavailable");
        if (!fs::exists(path))
            return std::nullopt;
        if (!fs::is_regular_file(path) || fs::file_size(path) > 10 * 1024 * 1024)
            throw std::runtime_error("Destination is not a supported design file");
        return File::ReadAllBytes(path.string());
    }
    inline GameActions::Result Review(const std::string& name)
    {
        CancelReview();
        if (!Valid())
            return Error("Ride changed; reopen Save design");
        if (!SafeName(name))
            return Error("Use 1-32 characters without path symbols");
        try
        {
            destination = fs::u8path(
                              GetContext()->GetPlatformEnvironment().GetDirectoryPath(DirBase::user, DirId::trackDesigns))
                / fs::u8path(name + trackDesignGetExtension(design->version));
            destinationBaseline = Existing(destination);
            reviewed = true;
            return {};
        }
        catch (const std::exception& e)
        {
            CancelReview();
            return Error(e.what());
        }
    }
    inline bool HasReview()
    {
        return reviewed;
    }
    inline bool Overwrite()
    {
        return reviewed && destinationBaseline.has_value();
    }
    inline std::string Filename()
    {
        return destination.filename().string();
    }
    inline GameActions::Result Confirm()
    {
        if (!reviewed || !Valid())
            return Error("Export review expired; reopen Save design");
        fs::path temporary;
        try
        {
            if (Existing(destination) != destinationBaseline)
            {
                CancelReview();
                return Error("File changed; review again");
            }
            fs::create_directories(destination.parent_path());
            do
            {
                temporary = destination.parent_path() / (".pocket-design-" + std::to_string(++serial) + ".tmp");
            } while (fs::exists(temporary) || fs::is_symlink(temporary));
            if (!RCT2::T6Exporter(*design).SaveTrack(temporary.string().c_str()))
                throw std::runtime_error("Native design export failed");
            if (!Valid() || Existing(destination) != destinationBaseline)
                throw std::runtime_error("Ride or file changed; review again");
            fs::rename(temporary, destination);
            End();
            TrackRepositoryScan();
            return {};
        }
        catch (const std::exception& e)
        {
            if (!temporary.empty())
            {
                std::error_code ignored;
                fs::remove(temporary, ignored);
            }
            CancelReview();
            return Error(e.what());
        }
    }
} // namespace OpenRCT2::Ui::Handheld::DesignAuthoring
