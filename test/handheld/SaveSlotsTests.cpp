// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/SaveSlots.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
static void require(bool condition)
{
    if (!condition)
        throw std::runtime_error("Save preservation check failed");
}
static std::string read(const fs::path& path)
{
    std::ifstream stream(path);
    return { std::istreambuf_iterator<char>(stream), {} };
}
int main()
{
    const auto root = fs::temp_directory_path() / "openrct2-pocket-save-tests";
    fs::remove_all(root);
    const auto save = root / "Pocket-1.park";
    using OpenRCT2::Ui::Handheld::WriteSaveAtomically;
    auto complete = [](const fs::path& path) {
        std::ofstream(path) << "complete park";
        return true;
    };
    require(WriteSaveAtomically(save, complete));
    require(read(save) == "complete park");
    // Failed serialization may leave partial output, but must preserve the existing park.
    require(!WriteSaveAtomically(save, [](const fs::path& path) {
        std::ofstream(path) << "partial";
        return false;
    }));
    require(read(save) == "complete park");
    require(!fs::exists(save.string() + ".pending.park"));
    require(!WriteSaveAtomically(save, [](const fs::path& path) -> bool {
        std::ofstream(path) << "interrupted";
        throw std::runtime_error("serialization interrupted");
    }));
    require(read(save) == "complete park");
    require(!fs::exists(save.string() + ".pending.park"));
    // Replacement uses a fresh staged file rather than overwriting the destination in-place.
    require(WriteSaveAtomically(save, [](const fs::path& path) {
        std::ofstream(path) << "updated park";
        return true;
    }));
    require(read(save) == "updated park");
    // Publishing to a directory fails and leaves that directory intact.
    const auto blocked = root / "blocked.park";
    fs::create_directory(blocked);
    require(!WriteSaveAtomically(blocked, complete));
    require(fs::is_directory(blocked));
    require(!fs::exists(blocked.string() + ".pending.park"));
    fs::remove_all(root);
    std::cout << "Save publication and failure-preservation checks passed\n";
}
