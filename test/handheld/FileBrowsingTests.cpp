// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/FileBrowsing.h"
#include <iostream>
#include <stdexcept>
using namespace OpenRCT2::Ui::Handheld::FileBrowsing;
static void require(bool v) { if (!v) throw std::runtime_error("File browsing contract failed"); }
int main()
{
    const auto root=fs::temp_directory_path()/"pocket-file-browser-tests";
    fs::remove_all(root); fs::create_directories(root/"autosave");
    std::ofstream(root/"valid.park") << "old bytes";
    std::ofstream(root/"legacy.SV6") << "legacy";
    std::ofstream(root/"bad.park.pending.park") << "partial";
    std::ofstream(root/"backup.park.bak") << "backup";
    fs::create_symlink(root/"valid.park",root/"alias.park");
    State s; s.Open(root,"*.park;*.sv6");
    require(s.entries.size()==3 && s.entries[0].directory && s.Rows().size()==5);
    auto id=Capture(root/"valid.park"); require(id && Current(*id));
    const auto stamp=fs::last_write_time(id->path);
    std::ofstream(id->path)<<"new bytes"; fs::last_write_time(id->path,stamp);
    require(!Current(*id)); // same size/date, different contents
    require(!Capture(root/"alias.park"));
    fs::remove(id->path); require(!Current(*id));
    s.Open(root/"autosave",s.pattern); require(s.entries.empty() && s.error.empty());
    s.Parent(); require(s.directory==root);
    s.Open(root/"missing",s.pattern); require(!s.error.empty() && s.entries.empty());
    for(int i=0;i<300;++i) std::ofstream(root/(std::to_string(i)+".park"))<<i;
    s.Open(root,"*.park"); require(s.entries.size()==301);
    std::string unicode;
    for(int i=0;i<80;++i) unicode+="界";
    require(DetailPages(unicode)==2 && Detail(unicode,0).size()==144 && Detail(unicode,1).size()==96);
    require(Detail(unicode,0)+Detail(unicode,1)==unicode && Detail(unicode,999)==Detail(unicode,1));
    fs::remove_all(root);
    std::cout<<"File enumeration, staging exclusion, identity and folder recovery PASS\n";
}
