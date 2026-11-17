// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/openrct2-ui/handheld/ListScroll.h"

#include <iostream>
#include <stdexcept>
using OpenRCT2::Ui::Handheld::ListScroll;
static void Check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
int main()
{
    ListScroll state;
    uint64_t time = 1000;
    Check(state.offset(0, 100, 180, 30, time) == 0, "New list did not start at top");
    Check(state.offset(3, 100, 180, 30, time += 16) == 0, "List moved before focus approached the edge");
    const auto first = state.offset(5, 100, 180, 30, time += 16);
    Check(first > 0 && first < 30 && state.moving(), "Boundary swapped a page instead of moving smoothly");
    const auto second = state.offset(5, 100, 180, 30, time += 16);
    Check(second > first && second < 30, "Scroll did not advance between frames");
    for (int i = 0; i < 30; ++i)
        state.offset(5, 100, 180, 30, time += 16);
    Check(!state.moving() && state.offset(5, 100, 180, 30, time) == 30, "Scrolling did not settle on one-row movement");
    for (size_t selected = 6; selected < 100; ++selected)
    {
        const int offset = state.offset(selected, 100, 180, 30, time += 10);
        Check(
            offset <= static_cast<int>(selected) * 30 && offset + 180 >= static_cast<int>(selected + 1) * 30,
            "Held repeats hid selected item");
    }
    Check(state.offset(0, 0, 180, 30, time += 16) == 0, "Empty list retained stale offset");
    Check(state.offset(1, 2, 180, 30, time += 16) == 0, "Shrinking list escaped bounds");
    Check(state.offset(99, 100, 180, 30, time += 16) == 2820, "Fast jump did not reveal selected end");
    Check(state.offset(0, 100, 180, 30, time += 16) == 0, "Return to first item traversed whole list");
    ListScroll other;
    Check(other.offset(0, 10, 180, 30, time) == 0, "Different view inherited position");
    std::cout << "PASS continuous movement, animation, focus visibility, fast jumps and empty/shrunk lists\n";
}
