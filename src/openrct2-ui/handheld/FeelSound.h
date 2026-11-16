// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "MenuFeel.h"

#include <openrct2/audio/Audio.h>

namespace OpenRCT2::Ui::Handheld
{
    // One sound vocabulary for every Pocket surface, driven by what changed on screen:
    // a light tick for focus, a tab click, a firmer click into a page and a softer one back.
    // Volumes are in hundredths of a decibel below the player's sound setting.
    inline void PlayFeelSound(MenuFeel::Event event, bool back = false)
    {
        using Event = MenuFeel::Event;
        using Audio::SoundId;
        switch (event)
        {
            case Event::focus:
                Audio::Play(SoundId::click3, -1500, Audio::kAudioPlayAtCentre);
                break;
            case Event::tab:
                Audio::Play(SoundId::click2, -900, Audio::kAudioPlayAtCentre);
                break;
            case Event::page:
                Audio::Play(back ? SoundId::click2 : SoundId::click1, back ? -800 : -500, Audio::kAudioPlayAtCentre);
                break;
            case Event::open:
                Audio::Play(SoundId::click1, -400, Audio::kAudioPlayAtCentre);
                break;
            case Event::none:
                break;
        }
    }
} // namespace OpenRCT2::Ui::Handheld
