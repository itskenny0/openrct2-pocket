// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/MenuThemes.h"
#include <iostream>
#include <stdexcept>
using namespace OpenRCT2::Ui::Handheld;
static void Check(bool value) { if(!value) throw std::runtime_error("Theme preference regression"); }
int main()
{
    const auto profile=std::filesystem::temp_directory_path()/"pocket-menu-theme-test";
    std::filesystem::remove_all(profile);
    MenuThemes::Settings setting;setting.Load(profile);
    Check(PocketPalette::style==PocketPalette::Style::daylight);
    Check(setting.Toggle());
    Check(PocketPalette::style==PocketPalette::Style::slate && PocketPalette::panel==12);
    MenuThemes::Settings reload;reload.Load(profile);
    Check(PocketPalette::style==PocketPalette::Style::slate);
    Check(reload.Toggle());
    MenuThemes::Settings again;again.Load(profile);
    Check(PocketPalette::style==PocketPalette::Style::daylight);
    std::ofstream(profile/"pocket-theme.cfg")<<"future-theme\n";
    MenuThemes::Settings unknown;unknown.Load(profile);
    Check(PocketPalette::style==PocketPalette::Style::daylight);
    std::ofstream(profile/"pocket-theme.cfg.pending")<<"another writer";
    Check(!unknown.Toggle());
    Check(PocketPalette::style==PocketPalette::Style::slate); // still usable this session
    std::string old;std::ifstream in(profile/"pocket-theme.cfg");std::getline(in,old);
    Check(old=="future-theme"); // failed write preserves existing preference
    std::filesystem::remove_all(profile);
    std::cout<<"Theme persistence, fallback and failure preservation passed\n";
}
