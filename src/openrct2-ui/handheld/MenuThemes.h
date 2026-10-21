// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PocketPalette.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace OpenRCT2::Ui::Handheld::MenuThemes
{
    // A separate optional preference, kept outside parks and the native config.
    // Unknown/missing preferences use this experiment's Midway default.
    class Settings
    {
        std::filesystem::path _file;
        bool _loaded{};
    public:
        void Load(const std::filesystem::path& profile)
        {
            if (_loaded) return;
            _loaded=true; _file=profile/"pocket-theme.cfg";
            std::ifstream in(_file);
            std::string name; std::getline(in,name);
            PocketPalette::SetStyle(name=="slate"?PocketPalette::Style::slate:PocketPalette::Style::midway);
        }
        bool Toggle()
        {
            using namespace PocketPalette;
            SetStyle(style==Style::midway?Style::slate:Style::midway);
            std::error_code error;
            std::filesystem::create_directories(_file.parent_path(),error);
            if(error) return false;
            auto pending=_file; pending+=".pending";
            const auto status=std::filesystem::symlink_status(pending,error);
            if(status.type()!=std::filesystem::file_type::not_found || (error && error!=std::errc::no_such_file_or_directory)) return false;
            error.clear();
            std::ofstream out(pending,std::ios::trunc);
            out<<(style==Style::midway?"midway\n":"slate\n");out.close();
            bool success=bool(out);
            if(success){std::filesystem::rename(pending,_file,error);success=!error;}
            if(!success)std::filesystem::remove(pending,error);
            return success;
        }
    };
    inline Settings settings;
    inline std::string Choice()
    {
        return PocketPalette::style==PocketPalette::Style::midway?"Theme: Midway":"Theme: Slate";
    }
}
