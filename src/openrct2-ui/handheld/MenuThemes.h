// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "MenuFeel.h"
#include "PocketPalette.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace OpenRCT2::Ui::Handheld::MenuThemes
{
    // A separate optional preference, kept outside parks and the native config.
    // Unknown/missing preferences use this experiment's Daylight default.
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
            PocketPalette::SetStyle(name=="slate"?PocketPalette::Style::slate:name=="pixel"?PocketPalette::Style::pixel:PocketPalette::Style::daylight);
            std::ifstream motion(profile/"pocket-motion.cfg");
            std::string mode; std::getline(motion,mode);
            MenuFeel::reduceMotion = mode=="reduced";
        }
        // Same atomic write as the theme, in its own file so older builds ignore it.
        bool ToggleMotion()
        {
            MenuFeel::reduceMotion=!MenuFeel::reduceMotion;
            std::error_code error;
            const auto file=_file.parent_path()/"pocket-motion.cfg";
            std::filesystem::create_directories(file.parent_path(),error);
            if(error) return false;
            auto pending=file; pending+=".pending";
            std::ofstream out(pending,std::ios::trunc);
            out<<(MenuFeel::reduceMotion?"reduced\n":"full\n");out.close();
            bool success=bool(out);
            if(success){std::filesystem::rename(pending,file,error);success=!error;}
            if(!success)std::filesystem::remove(pending,error);
            return success;
        }
        bool Toggle()
        {
            using namespace PocketPalette;
            // Daylight -> Slate -> Pixel -> Daylight.
            SetStyle(style==Style::daylight?Style::slate:style==Style::slate?Style::pixel:Style::daylight);
            std::error_code error;
            std::filesystem::create_directories(_file.parent_path(),error);
            if(error) return false;
            auto pending=_file; pending+=".pending";
            const auto status=std::filesystem::symlink_status(pending,error);
            if(status.type()!=std::filesystem::file_type::not_found || (error && error!=std::errc::no_such_file_or_directory)) return false;
            error.clear();
            std::ofstream out(pending,std::ios::trunc);
            out<<(style==Style::daylight?"daylight\n":style==Style::slate?"slate\n":"pixel\n");out.close();
            bool success=bool(out);
            if(success){std::filesystem::rename(pending,_file,error);success=!error;}
            if(!success)std::filesystem::remove(pending,error);
            return success;
        }
    };
    inline Settings settings;
    inline std::string MotionChoice()
    {
        return MenuFeel::reduceMotion?"Motion: Reduced":"Motion: Full";
    }
    inline std::string Choice()
    {
        return PocketPalette::style==PocketPalette::Style::daylight?"Theme: Daylight"
            :PocketPalette::style==PocketPalette::Style::slate?"Theme: Slate":"Theme: Pixel";
    }
}
