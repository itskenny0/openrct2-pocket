// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace OpenRCT2::Ui::Handheld
{
    // Reusable bounded controller keyboard. Existing UTF-8 is retained and erased
    // by complete code point; newly typed keys are ASCII, as labelled on screen.
    struct TextEntry
    {
        static constexpr size_t columns = 8;
        static constexpr size_t limit = 32;
        std::string text;
        bool lower{};
        std::vector<std::string> Keys() const
        {
            std::vector<std::string> keys;
            for (char c = lower ? 'a' : 'A'; c <= (lower ? 'z' : 'Z'); ++c)
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
            if (index == 43 && !text.empty())
            {
                size_t start = text.size() - 1;
                while (start && (static_cast<unsigned char>(text[start]) & 0xc0) == 0x80)
                    --start;
                text.erase(start);
            }
            else if (index == 44)
                lower = !lower;
            else if (index == 45)
                text.clear();
            else if (index < 43 && Length() < limit)
                text += index == 42 ? " " : keys[index];
        }
        size_t Move(size_t selected, int x, int y) const
        {
            const auto count = Keys().size();
            const int row = selected / columns;
            const int col = selected % columns;
            const int nextRow = std::clamp(row + y, 0, static_cast<int>((count - 1) / columns));
            const int nextCol = std::clamp(col + x, 0, static_cast<int>(columns) - 1);
            return std::min(count - 1, nextRow * columns + nextCol);
        }
    };
} // namespace OpenRCT2::Ui::Handheld
