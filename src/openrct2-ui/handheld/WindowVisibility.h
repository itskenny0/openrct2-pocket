/*****************************************************************************/
// Copyright (c) 2026 OpenRCT2 developers. GPL-3.0-only.
#pragma once
#include <openrct2/interface/WindowClasses.h>
namespace OpenRCT2::Ui::Handheld
{
    constexpr bool IsDesktopChrome(WindowClass cls)
    {
        switch (cls)
        {
            case WindowClass::topToolbar:
            case WindowClass::gameStatusBar:
            case WindowClass::parkInfoPanel:
            case WindowClass::dateInfoPanel:
            case WindowClass::newsTicker:
            case WindowClass::titleLogo:
            case WindowClass::titleExit:
            case WindowClass::titleOptions:
            case WindowClass::titleVersion:
                return true;
            default:
                return false;
        }
    }
} // namespace OpenRCT2::Ui::Handheld
