// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld
{
    // Reusable bounded controller keyboard. Existing UTF-8 is retained and erased
    // by complete code point; newly typed keys are ASCII, as labelled on screen.
    //
    // Keys() is the logical key list, and its indices are what screens confirm on (Done is 46,
    // Back 47), so they never change. Layout() places those keys on a 10-column QWERTY board with
    // wide Shift, Space, Delete and Done keys, and Move() walks that board.
    struct TextEntry
    {
        static constexpr size_t columns = 10;
        static constexpr size_t limit = 32;
        enum Key : size_t
        {
            kSpace = 42,
            kDelete = 43,
            kCase = 44,
            kClear = 45,
            kDone = 46,
            kBack = 47
        };
        std::string text;
        bool lower{};
        // Until Shift is pressed, case follows the text the way a phone keyboard does: a capital
        // to start and after each space, lower case otherwise. "Bob Smith" needs no Shift.
        bool manualCase{};
        bool Lower() const
        {
            if (manualCase)
                return lower;
            return !text.empty() && text.back() != ' ';
        }
        std::vector<std::string> Keys() const
        {
            std::vector<std::string> keys;
            const bool small = Lower();
            for (char c = small ? 'a' : 'A'; c <= (small ? 'z' : 'Z'); ++c)
                keys.emplace_back(1, c);
            for (char c = '0'; c <= '9'; ++c)
                keys.emplace_back(1, c);
            for (const auto* key : { "-", ".", "'", "&", "(", ")", "Space", "Delete", "Case", "Clear", "Done", "Back" })
                keys.emplace_back(key);
            return keys;
        }
        size_t Length() const
        {
            return std::count_if(text.begin(), text.end(), [](unsigned char c) { return (c & 0xc0) != 0x80; });
        }
        void Press(size_t index)
        {
            const auto keys = Keys();
            if (index >= keys.size())
                return;
            if (index == kDelete && !text.empty())
            {
                size_t start = text.size() - 1;
                while (start && (static_cast<unsigned char>(text[start]) & 0xc0) == 0x80)
                    --start;
                text.erase(start);
            }
            else if (index == kCase)
            {
                lower = !Lower();
                manualCase = true;
            }
            else if (index == kClear)
            {
                text.clear();
                manualCase = false;
            }
            else if (index < kDelete && Length() < limit)
            {
                text += index == kSpace ? " " : keys[index];
                if (index == kSpace)
                    manualCase = false; // Back to automatic capitals for the next word.
            }
        }

        struct Slot
        {
            size_t key;
            int column, span;
        };
        static const std::vector<std::vector<Slot>>& Layout()
        {
            static const std::vector<std::vector<Slot>> rows = [] {
                std::vector<std::vector<Slot>> result;
                auto letters = [](const char* row, std::vector<Slot> tail = {}) {
                    std::vector<Slot> slots;
                    int column = 0;
                    for (const char* c = row; *c; ++c)
                        slots.push_back({ static_cast<size_t>(*c - 'A'), column++, 1 });
                    for (auto slot : tail)
                    {
                        slot.column = column++;
                        slots.push_back(slot);
                    }
                    return slots;
                };
                std::vector<Slot> digits;
                for (int i = 0; i < 10; ++i)
                    digits.push_back({ static_cast<size_t>(26 + (i + 1) % 10), i, 1 }); // 1..9 then 0
                result.push_back(digits);
                result.push_back(letters("QWERTYUIOP"));
                result.push_back(letters("ASDFGHJKL", { { 38, 0, 1 } }));                           // '
                result.push_back(letters("ZXCVBNM", { { 36, 0, 1 }, { 37, 0, 1 }, { 39, 0, 1 } })); // - . &
                result.push_back({ { kCase, 0, 2 }, { 40, 2, 1 }, { 41, 3, 1 }, { kSpace, 4, 4 }, { kDelete, 8, 2 } });
                result.push_back({ { kClear, 0, 3 }, { kDone, 3, 7 } });
                return result;
            }();
            return rows;
        }
        // Where a logical key sits on the board. Back has no key (B does it), so it reads as Done.
        static std::pair<size_t, size_t> Find(size_t key)
        {
            const auto& rows = Layout();
            for (size_t r = 0; r < rows.size(); ++r)
                for (size_t s = 0; s < rows[r].size(); ++s)
                    if (rows[r][s].key == key)
                        return { r, s };
            return { rows.size() - 1, rows.back().size() - 1 };
        }
        // Left/Right step through the row; Up/Down land on the key under the current key's centre,
        // so moving off Space lands on V/B/N/M rather than the far left.
        size_t Move(size_t selected, int x, int y) const
        {
            const auto& rows = Layout();
            auto [r, s] = Find(selected);
            if (y)
            {
                const auto& from = rows[r][s];
                const int centre = from.column * 2 + from.span; // In half columns.
                r = static_cast<size_t>(std::clamp(static_cast<int>(r) + y, 0, static_cast<int>(rows.size()) - 1));
                s = 0;
                for (size_t i = 0; i < rows[r].size(); ++i)
                    if (rows[r][i].column * 2 < centre)
                        s = i;
            }
            if (x)
                s = static_cast<size_t>(std::clamp(static_cast<int>(s) + x, 0, static_cast<int>(rows[r].size()) - 1));
            return rows[r][s].key;
        }
    };
} // namespace OpenRCT2::Ui::Handheld
