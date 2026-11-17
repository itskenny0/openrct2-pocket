// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../../openrct2/core/FileIdentity.h"
#include <algorithm>
#include <chrono>
#include <ctime>
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
    inline size_t DetailPages(const std::string& text)
    {
        const auto length=std::count_if(text.begin(),text.end(),[](unsigned char c){return (c&0xc0)!=0x80;});
        return std::max<size_t>(1,(length+47)/48);
    }
    inline std::string Detail(const std::string& text,size_t page)
    {
        page=std::min(page,DetailPages(text)-1);
        size_t start=0, end=0, points=0;
        for(;start<text.size() && points<page*48;++start) if((static_cast<unsigned char>(text[start])&0xc0)!=0x80) ++points;
        while(start<text.size() && (static_cast<unsigned char>(text[start])&0xc0)==0x80) ++start;
        points=0;end=start;
        for(;end<text.size() && points<48;++end) if((static_cast<unsigned char>(text[end])&0xc0)!=0x80) ++points;
        while(end<text.size() && (static_cast<unsigned char>(text[end])&0xc0)==0x80) ++end;
        return text.substr(start,end-start);
    }
    using Identity=OpenRCT2::FileIdentity::Identity;
    using OpenRCT2::FileIdentity::Capture;
    using OpenRCT2::FileIdentity::Current;
    struct Entry { fs::path path; bool directory{}; };
    // Rows are "Up one folder" then the entries. Opening a folder always re-reads it, so there is
    // no separate Refresh row.
    inline constexpr size_t kFirst = 1;
    // When a file was last written, as "16 Nov, 22:40". The file clock is converted through "now"
    // on both clocks, which works on every C++17 standard library (no clock_cast).
    inline std::string Modified(const fs::path& path)
    {
        std::error_code error;
        const auto written = fs::last_write_time(path, error);
        if (error) return {};
        const auto system = std::chrono::system_clock::now()
            + std::chrono::duration_cast<std::chrono::system_clock::duration>(written - fs::file_time_type::clock::now());
        const std::time_t time = std::chrono::system_clock::to_time_t(system);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &time);
#else
        localtime_r(&time, &local);
#endif
        char text[32];
        return std::strftime(text, sizeof(text), "%d %b, %H:%M", &local) ? text : "";
    }
    // "Pocket-1.park" reads as "Pocket 1": no extension, separators as spaces.
    inline std::string DisplayName(const fs::path& path, bool directory)
    {
        auto name = directory ? path.filename().u8string() : path.stem().u8string();
        std::replace(name.begin(), name.end(), '_', ' ');
        std::replace(name.begin(), name.end(), '-', ' ');
        return name;
    }
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
            std::vector<std::string> rows{"Up one folder"};
            for (const auto& e:entries) rows.push_back((e.directory ? "Folder: " : "") + e.path.filename().string());
            return rows;
        }
    };
}
