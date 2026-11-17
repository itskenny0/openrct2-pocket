// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/MenuFeel.h"

#include <iostream>
#include <stdexcept>
using OpenRCT2::Ui::Handheld::MenuFeel;
static void Check(bool ok, const char* what = "Menu motion regression")
{
    if (!ok)
        throw std::runtime_error(what);
}
int main()
{
    using Event = MenuFeel::Event;
    MenuFeel feel;
    Check(feel.Observe("rides", 0, 1000, 1) == Event::open, "first page is an open");
    Check(feel.NeedsDraw(1000));
    Check(feel.FocusPercent(1060) == 50);
    Check(feel.Slide(1000).second == 18 && feel.Slide(1000).first == 0, "sheet rises in when the menu opens");
    Check(feel.Slide(1150).second == 0, "open slide finishes in 150 ms");
    Check(feel.NeedsDraw(1150)); // final settled frame must be painted
    Check(!feel.NeedsDraw(1151));
    feel.Press(1200);
    Check(feel.ButtonOffset(1200) == 1 && feel.NeedsDraw(1200));
    Check(feel.ButtonOffset(1290) == 0 && feel.NeedsDraw(1290));
    Check(!feel.NeedsDraw(1291)); // released button must not remain depressed
    feel.BeginDraw();
    feel.AnimatedSprite();
    Check(feel.NeedsDraw(1440));
    Check(!feel.NeedsDraw(1441)); // no continuous redraw between sprite frames
    feel.BeginDraw();
    Check(!feel.NeedsDraw(1560)); // pages without animation become idle
    feel.Press(1600);
    Check(feel.Observe("build", 0, 1601, 2) == Event::page);
    Check(feel.ButtonOffset(1601) == 0); // no press transferred to another page
    Check(feel.FocusPercent(1601) == 0);
    Check(feel.Slide(1601).first > 0, "deeper pages arrive from the right");
    feel.MarkBack();
    Check(feel.Observe("rides", 0, 1800, 1) == Event::page);
    Check(feel.Slide(1800).first < 0, "Back returns from the left");
    Check(feel.Observe("rides:1", 0, 2000, 1) == Event::tab, "tab change on the same page");
    Check(feel.Observe("rides:1", 3, 2100, 1) == Event::focus);
    Check(feel.Observe("rides:1", 3, 2101, 1) == Event::none);

    // Pointer: snaps to the first selected card, then glides to whichever card changed.
    feel.BeginDraw();
    feel.Selected({ 10, 40, 200, 28 });
    feel.EndFrame(3000);
    Check(feel.HasFocusRect() && feel.FocusRect(3000).y == 40 && !feel.Gliding(3000));
    feel.BeginDraw();
    feel.Selected({ 10, 72, 200, 28 });
    feel.EndFrame(3100);
    Check(feel.Gliding(3150), "focus glides between cards");
    const auto mid = feel.FocusRect(3150).y;
    Check(mid > 40 && mid < 72, "glide passes between the two cards");
    Check(feel.FocusRect(3100 + MenuFeel::kGlideMs).y == 72 && !feel.Gliding(3100 + MenuFeel::kGlideMs));
    feel.BeginDraw();
    feel.EndFrame(3300);
    Check(!feel.HasFocusRect(), "no pointer without a selected card");

    // Reduced motion: no slides, glides, bob or focus sweep, and idle pages stop redrawing.
    MenuFeel::reduceMotion = true;
    MenuFeel calm;
    calm.Observe("park", 0, 5000, 7);
    Check(calm.Slide(5000).first == 0 && calm.Slide(5000).second == 0 && calm.FocusPercent(5000) == 100);
    calm.BeginDraw();
    calm.Selected({ 0, 0, 10, 10 });
    calm.EndFrame(5000);
    calm.BeginDraw();
    calm.Selected({ 0, 20, 10, 10 });
    calm.EndFrame(5010);
    Check(!calm.Gliding(5020) && calm.PointerBob(5160) == 0);
    calm.NeedsDraw(5030);
    Check(!calm.NeedsDraw(5400), "reduced motion idles");
    MenuFeel::reduceMotion = false;
    std::cout << "Menu animation release, idle, page isolation, slides, glide and reduced motion passed\n";
}
