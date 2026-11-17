// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "FileBrowsing.h"
#include "TextEntry.h"
#ifndef _WIN32
    #include <sys/stat.h>
#endif

namespace OpenRCT2::Ui::Handheld::NamedSave
{
    namespace fs = std::filesystem;
    inline bool SafeName(const std::string& name)
    {
        if (name.empty() || TextEntry{ name }.Length() > TextEntry::limit || name.back() == '.' || name.back() == ' ')
            return false;
        for (unsigned char c : name)
            if (c < 32 || c == 127 || std::string("/\\:*?\"<>|").find(c) != std::string::npos)
                return false;
        return true;
    }
    struct Draft
    {
        fs::path destination, directory;
        std::optional<FileBrowsing::Identity> baseline;
#ifndef _WIN32
        uint64_t device{}, inode{};
#endif
        static std::optional<Draft> Review(const fs::path& folder, const std::string& name, std::string& error)
        {
            error.clear();
            if (!SafeName(name))
            {
                error = "Use 1-32 characters without path symbols";
                return {};
            }
            try
            {
                Draft d;
                d.directory = fs::canonical(folder);
                if (!fs::is_directory(d.directory))
                    throw std::runtime_error("Destination folder unavailable");
#ifndef _WIN32
                struct stat info{};
                if (::stat(d.directory.c_str(), &info) != 0)
                    throw std::runtime_error("Destination folder unavailable");
                d.device = info.st_dev;
                d.inode = info.st_ino;
#endif
                d.destination = d.directory / fs::u8path(name);
                d.destination.replace_extension(".park");
                std::error_code ec;
                const auto status = fs::symlink_status(d.destination, ec);
                if (status.type() != fs::file_type::not_found)
                {
                    d.baseline = FileBrowsing::Capture(d.destination);
                    if (!d.baseline)
                        throw std::runtime_error("Destination is unreadable, a link or not a regular file");
                }
                else if (ec && ec != std::errc::no_such_file_or_directory)
                    throw std::runtime_error(ec.message());
                return d;
            }
            catch (const std::exception& e)
            {
                error = e.what();
                return {};
            }
        }
        bool Current() const
        {
            std::error_code ec;
            if (fs::canonical(directory, ec) != directory || ec || !fs::is_directory(directory, ec) || ec)
                return false;
#ifndef _WIN32
            struct stat info{};
            if (::stat(directory.c_str(), &info) != 0 || info.st_dev != device || info.st_ino != inode)
                return false;
#endif
            if (baseline)
                return FileBrowsing::Current(*baseline);
            const auto status = fs::symlink_status(destination, ec);
            return status.type() == fs::file_type::not_found && (!ec || ec == std::errc::no_such_file_or_directory);
        }
    };
} // namespace OpenRCT2::Ui::Handheld::NamedSave
