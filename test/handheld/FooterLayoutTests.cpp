// SPDX-License-Identifier: GPL-3.0-only
#include <iostream>
#include <openrct2-ui/handheld/Panel.h>
using namespace OpenRCT2::Ui::Handheld;
int main()
{
    std::vector<std::vector<ControlHint>> cases{
        { { "D-pad", "Navigate" }, { "A", "Select" }, { "B", "Park" }, { "L1/R1", "Section" } },
        { { "D-pad", "Choose" },
          { "A", "Select" },
          { "B", "Back" },
          { "L1/R1", "Tab" },
          { "X", "Access" },
          { "SELECT", "Sections", HintPriority::guide } },
        { { "A", "Build" }, { "B", "Undo" }, { "L1/R1", "Piece" }, { "X", "Speed +" }, { "Y", "Speed -" } },
        { { "D-pad", "Move" },
          { "A", "Place" },
          { "B", "Back" },
          { "X", "Remove" },
          { "Y", "Adjust" },
          { "SELECT", "Surface", HintPriority::guide },
          { "L1/R1", "Zoom", HintPriority::secondary } },
        { { "D-pad", "Move" },
          { "A", "Inspect" },
          { "B", "Back" },
          { "SELECT", "Build" },
          { "START", "Park" },
          { "L1/R1", "Zoom", HintPriority::secondary },
          { "X", "Rotate", HintPriority::guide } },
        { { "D-pad", "Choose" },
          { "A", "Select" },
          { "B", "Back" },
          { "L1/R1", "Page" },
          { "START", "Ride", HintPriority::guide },
          { "X", "Camera" } }
    };
    for (const auto& c : cases)
    {
        auto p = MeasureFooter({ 512, 384 }, c);
        std::sort(p.begin(), p.end(), [](auto a, auto b) { return a.x < b.x; });
        int right = 0;
        std::cout << "height=" << FooterHeight({ 512, 384 }, c) << " ";
        for (auto e : p)
        {
            std::cout << e.hint.key << ":" << e.x << ".." << e.x + e.width << " ";
            if (e.x < right || e.x + e.width > 504)
                return 1;
            right = e.x + e.width;
        }
        std::cout << "\n";
        if (FooterHeight({ 512, 384 }, c) != 28)
            return 2;
    }
}
