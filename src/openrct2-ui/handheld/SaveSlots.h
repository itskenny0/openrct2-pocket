// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <exception>
#include <filesystem>

namespace OpenRCT2::Ui::Handheld
{
    // Publish only a complete save. A serialization/rename failure keeps the old park.
    template<typename Writer, typename Validate>
    bool WriteSaveAtomically(const std::filesystem::path& path, Writer write, Validate valid)
    {
        auto temporary = path;
        temporary += ".pending.park";
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        // Never follow or remove another writer's existing staging entry.
        const auto stage = std::filesystem::symlink_status(temporary, error);
        if (stage.type() != std::filesystem::file_type::not_found || (error && error != std::errc::no_such_file_or_directory))
            return false;
        error.clear();
        bool success = false;
        try
        {
            if (!error && valid() && write(temporary) && valid())
            {
                std::filesystem::rename(temporary, path, error);
                success = !error;
            }
        }
        catch (const std::exception&)
        {
            success = false;
        }
        if (!success)
            std::filesystem::remove(temporary, error);
        return success;
    }
    template<typename Writer>
    bool WriteSaveAtomically(const std::filesystem::path& path, Writer write)
    {
        return WriteSaveAtomically(path, write, [] { return true; });
    }
} // namespace OpenRCT2::Ui::Handheld
