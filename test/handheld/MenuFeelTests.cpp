// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/MenuFeel.h"
#include <iostream>
#include <stdexcept>
using OpenRCT2::Ui::Handheld::MenuFeel;
static void Check(bool ok) { if (!ok) throw std::runtime_error("Menu motion regression"); }
int main()
{
    MenuFeel feel;
    feel.Observe("rides", 0, 1000);
    Check(feel.NeedsDraw(1000));
    Check(feel.FocusPercent(1060) == 50);
    Check(feel.NeedsDraw(1120)); // final settled frame must be painted
    Check(!feel.NeedsDraw(1121));
    feel.Press(1200);
    Check(feel.ButtonOffset(1200) == 1 && feel.NeedsDraw(1200));
    Check(feel.ButtonOffset(1290) == 0 && feel.NeedsDraw(1290));
    Check(!feel.NeedsDraw(1291)); // released button must not remain depressed
    feel.BeginDraw(); feel.AnimatedSprite();
    Check(feel.NeedsDraw(1440));
    Check(!feel.NeedsDraw(1441)); // no continuous redraw between sprite frames
    feel.BeginDraw();
    Check(!feel.NeedsDraw(1560)); // pages without animation become idle
    feel.Press(1600); feel.Observe("build", 0, 1601);
    Check(feel.ButtonOffset(1601) == 0); // no press transferred to another page
    Check(feel.FocusPercent(1601) == 0);
    std::cout << "Menu animation release, idle and page isolation passed\n";
}
