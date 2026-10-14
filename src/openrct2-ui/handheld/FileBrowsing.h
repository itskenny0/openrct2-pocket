// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "../../openrct2/core/FileIdentity.h"
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
