// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "MenuFeel.h"
#include "PocketPalette.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace OpenRCT2::Ui::Handheld
{
    // On/off preferences that are not theme or motion, as "key=1" lines in pocket-prefs.cfg.
    // Defaults are chosen for a first launch; older builds ignore the file.
    struct Preferences
    {
        bool smartCursor{};       // Cursor snaps to rides, stalls and guests when it passes them.
        bool resumeOnLaunch{};    // Skip the title and continue the last park.
        bool hudGoal{ true };     // The scenario goal's progress in the HUD.
        bool hudClock{};          // Time and battery in the HUD.
        bool saveOnSleep{ true }; // Autosave when the device sleeps or the app is closed.
        bool primerSeen{};        // The first-run controls introduction has been shown.
        bool* Find(const std::string& key)
        {
            return key == "smartCursor"   ? &smartCursor
                : key == "resumeOnLaunch" ? &resumeOnLaunch
                : key == "hudGoal"        ? &hudGoal
                : key == "hudClock"       ? &hudClock
                : key == "saveOnSleep"    ? &saveOnSleep
                : key == "primerSeen"     ? &primerSeen
                                          : nullptr;
        }
    };
    inline Preferences preferences;
} // namespace OpenRCT2::Ui::Handheld

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
            if (_loaded)
                return;
            _loaded = true;
            _file = profile / "pocket-theme.cfg";
            std::ifstream in(_file);
            std::string name;
            std::getline(in, name);
            PocketPalette::SetStyle(
                name == "slate"       ? PocketPalette::Style::slate
                    : name == "pixel" ? PocketPalette::Style::pixel
                                      : PocketPalette::Style::daylight);
            std::ifstream motion(profile / "pocket-motion.cfg");
            std::string mode;
            std::getline(motion, mode);
            MenuFeel::reduceMotion = mode == "reduced";
            std::ifstream prefs(profile / "pocket-prefs.cfg");
            for (std::string line; std::getline(prefs, line);)
                if (const auto equals = line.find('='); equals != std::string::npos)
                    if (auto* flag = preferences.Find(line.substr(0, equals)))
                        *flag = line.substr(equals + 1) == "1";
        }
        bool SavePreferences()
        {
            std::error_code error;
            const auto file = _file.parent_path() / "pocket-prefs.cfg";
            std::filesystem::create_directories(file.parent_path(), error);
            if (error)
                return false;
            auto pending = file;
            pending += ".pending";
            std::ofstream out(pending, std::ios::trunc);
            for (const auto* key : { "smartCursor", "resumeOnLaunch", "hudGoal", "hudClock", "saveOnSleep", "primerSeen" })
                out << key << '=' << (*preferences.Find(key) ? 1 : 0) << '\n';
            out.close();
            bool success = bool(out);
            if (success)
            {
                std::filesystem::rename(pending, file, error);
                success = !error;
            }
            if (!success)
                std::filesystem::remove(pending, error);
            return success;
        }
        bool TogglePreference(const std::string& key)
        {
            if (auto* flag = preferences.Find(key))
                *flag = !*flag;
            return SavePreferences();
        }
        bool Loaded() const
        {
            return _loaded;
        }
        // Same atomic write as the theme, in its own file so older builds ignore it.
        bool ToggleMotion()
        {
            MenuFeel::reduceMotion = !MenuFeel::reduceMotion;
            std::error_code error;
            const auto file = _file.parent_path() / "pocket-motion.cfg";
            std::filesystem::create_directories(file.parent_path(), error);
            if (error)
                return false;
            auto pending = file;
            pending += ".pending";
            std::ofstream out(pending, std::ios::trunc);
            out << (MenuFeel::reduceMotion ? "reduced\n" : "full\n");
            out.close();
            bool success = bool(out);
            if (success)
            {
                std::filesystem::rename(pending, file, error);
                success = !error;
            }
            if (!success)
                std::filesystem::remove(pending, error);
            return success;
        }
        bool Toggle()
        {
            using namespace PocketPalette;
            // Daylight -> Slate -> Pixel -> Daylight.
            SetStyle(style == Style::daylight ? Style::slate : style == Style::slate ? Style::pixel : Style::daylight);
            std::error_code error;
            std::filesystem::create_directories(_file.parent_path(), error);
            if (error)
                return false;
            auto pending = _file;
            pending += ".pending";
            const auto status = std::filesystem::symlink_status(pending, error);
            if (status.type() != std::filesystem::file_type::not_found
                || (error && error != std::errc::no_such_file_or_directory))
                return false;
            error.clear();
            std::ofstream out(pending, std::ios::trunc);
            out << (style == Style::daylight ? "daylight\n" : style == Style::slate ? "slate\n" : "pixel\n");
            out.close();
            bool success = bool(out);
            if (success)
            {
                std::filesystem::rename(pending, _file, error);
                success = !error;
            }
            if (!success)
                std::filesystem::remove(pending, error);
            return success;
        }
    };
    inline Settings settings;
    inline std::string MotionChoice()
    {
        return MenuFeel::reduceMotion ? "Motion: Reduced" : "Motion: Full";
    }
    inline std::string Choice()
    {
        return PocketPalette::style == PocketPalette::Style::daylight ? "Theme: Daylight"
            : PocketPalette::style == PocketPalette::Style::slate     ? "Theme: Slate"
                                                                      : "Theme: Pixel";
    }
} // namespace OpenRCT2::Ui::Handheld::MenuThemes
