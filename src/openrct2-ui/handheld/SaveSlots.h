// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <exception>
#include <filesystem>

namespace OpenRCT2::Ui::Handheld
{
    // Publish only a complete save. A serialization/rename failure keeps the old park.
    template<typename Writer>
    bool WriteSaveAtomically(const std::filesystem::path& path, Writer write)
    {
        auto temporary = path;
        temporary += ".pending.park";
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        bool success = false;
        try
        {
            if (!error && write(temporary))
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
} // namespace OpenRCT2::Ui::Handheld
