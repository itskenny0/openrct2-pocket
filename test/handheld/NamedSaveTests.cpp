// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/NamedSave.h"
#include "../../src/openrct2-ui/handheld/SaveSlots.h"

#include <iostream>
using namespace OpenRCT2::Ui::Handheld;
namespace fs = std::filesystem;
static void check(bool ok)
{
    if (!ok)
        throw std::runtime_error("Named save safety failed");
}
static std::string read(fs::path p)
{
    std::ifstream f(p);
    return { std::istreambuf_iterator<char>(f), {} };
}
int main()
{
    const auto root = fs::temp_directory_path() / "pocket-named-save-tests";
    fs::remove_all(root);
    fs::create_directories(root);
    std::string error;
    for (const auto* name : { "", "../park", "end.", "end ", "x/y", "x\\y", "x:y" })
        check(!NamedSave::Draft::Review(root, name, error));
    auto fresh = NamedSave::Draft::Review(root, "Résumé.sv6", error);
    check(fresh && fresh->destination.filename() == fs::u8path("Résumé.park") && fresh->Current());
    auto writer = [](fs::path p) {
        std::ofstream(p) << "native bytes";
        return true;
    };
    check(WriteSaveAtomically(fresh->destination, writer, [&] { return fresh->Current(); }));
    auto replacement = NamedSave::Draft::Review(root, "Résumé.park", error);
    check(replacement && replacement->baseline);
    check(!WriteSaveAtomically(
        replacement->destination,
        [](fs::path p) {
            std::ofstream(p) << "partial";
            return false;
        },
        [&] { return replacement->Current(); }));
    check(read(replacement->destination) == "native bytes");
    check(!WriteSaveAtomically(
        replacement->destination,
        [&](fs::path p) {
            std::ofstream(p) << "new save";
            std::ofstream(replacement->destination) << "external bytes";
            return true;
        },
        [&] { return replacement->Current(); }));
    check(read(replacement->destination) == "external bytes");
    auto stage = replacement->destination;
    stage += ".pending.park";
    fs::create_symlink(replacement->destination, stage);
    check(!WriteSaveAtomically(replacement->destination, writer));
    check(fs::is_symlink(stage));
    check(read(replacement->destination) == "external bytes");
    fs::remove(stage);
    fs::create_symlink(replacement->destination, root / "alias.park");
    check(!NamedSave::Draft::Review(root, "alias", error));
    fs::create_directories(root / "directory.park");
    check(!NamedSave::Draft::Review(root, "directory", error));
    auto folder = NamedSave::Draft::Review(root, "Fresh", error);
    check(folder && folder->Current());
    fs::rename(root, root.string() + "-moved");
    fs::create_directory(root);
    check(!folder->Current());
    fs::remove_all(root);
    fs::remove_all(root.string() + "-moved");
    std::cout << "Named save UTF-8/extension/review/stale/partial/staging/folder guards PASS\n";
}
