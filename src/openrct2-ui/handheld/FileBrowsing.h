// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld::FileBrowsing
{
    namespace fs = std::filesystem;
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
    struct Entry { fs::path path; bool directory{}; };
    struct State
    {
        fs::path directory;
        std::string pattern, error;
        std::vector<Entry> entries;
        bool Supported(const fs::path& path) const
        {
            auto name = path.filename().string();
            if (name.find(".pending.") != std::string::npos) return false;
            auto extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){ return std::tolower(c); });
            return !extension.empty() && pattern.find("*" + extension + ";") != std::string::npos;
        }
        void Open(fs::path path, std::string filter)
        { directory = std::move(path); pattern = std::move(filter); if (pattern.empty() || pattern.back() != ';') pattern += ';'; Refresh(); }
        void Refresh()
        {
            entries.clear(); error.clear(); std::error_code ec;
            fs::directory_iterator it(directory, ec), end;
            if (ec) { error = "Cannot read folder: " + ec.message(); return; }
            while (it != end) {
                const auto status = it->symlink_status(ec);
                if (ec) { error = "Cannot read folder entry: " + ec.message(); break; }
                if (fs::is_directory(status)) entries.push_back({it->path(),true});
                else if (fs::is_regular_file(status) && Supported(it->path())) entries.push_back({it->path(),false});
                it.increment(ec);
                if (ec) { error = "Cannot read folder: " + ec.message(); break; }
            }
            if (!error.empty()) entries.clear();
            std::sort(entries.begin(), entries.end(), [](const Entry& a,const Entry& b) {
                return a.directory != b.directory ? a.directory : a.path.filename() < b.path.filename();
            });
        }
        void Parent() { const auto parent=directory.parent_path(); if (!parent.empty()) directory=parent; Refresh(); }
        std::vector<std::string> Rows() const
        {
            std::vector<std::string> rows{"Parent folder", "Refresh folder"};
            for (const auto& e:entries) rows.push_back((e.directory ? "Folder: " : "") + e.path.filename().string());
            return rows;
        }
    };
}
