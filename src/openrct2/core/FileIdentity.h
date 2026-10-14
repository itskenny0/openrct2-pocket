// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <filesystem>
#include <fstream>
#include <optional>
namespace OpenRCT2::FileIdentity
{
    namespace fs=std::filesystem;
    struct Identity
    {
        fs::path path;
        uintmax_t size{};
        fs::file_time_type modified{};
        uint64_t digest = 14695981039346656037ULL;
        bool operator==(const Identity&) const = default;
    };
    // Full byte fingerprint detects replacements even with preserved size/mtime.
    // No park import, object loading or gameplay mutation occurs here.
    inline std::optional<Identity> Capture(const fs::path& path)
    {
        std::error_code ec;
        if (!fs::is_regular_file(fs::symlink_status(path, ec)) || ec) return {};
        Identity id; id.path = path;
        id.size = fs::file_size(path, ec); if (ec) return {};
        id.modified = fs::last_write_time(path, ec); if (ec) return {};
        std::ifstream input(path, std::ios::binary);
        if (!input) return {};
        char buffer[16384]; uintmax_t count = 0;
        while (input.read(buffer, sizeof(buffer)) || input.gcount())
            for (std::streamsize i = 0; i < input.gcount(); ++i) {
                id.digest = (id.digest ^ static_cast<unsigned char>(buffer[i])) * 1099511628211ULL;
                ++count;
            }
        if (!input.eof() || count != id.size) return {};
        if (fs::file_size(path, ec) != id.size || ec) return {};
        if (fs::last_write_time(path, ec) != id.modified || ec) return {};
        return id;
    }
    inline bool Current(const Identity& id) { return Capture(id.path) == id; }
}
