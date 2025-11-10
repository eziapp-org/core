#pragma once

#include "platform.hpp"

#if OS(WINDOWS)
    #include <dwmapi.h>
    #include <winrt/Windows.UI.ViewManagement.h>
    #include <winrt/Windows.System.Profile.h>
    #pragma comment(lib, "windowsapp")
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::System::Profile;
#endif

namespace ezi
{

    namespace Utils
    {
        bool        IsDarkMode();
        COLORREF    GetAccentColor();
        std::string GetArg(std::string key);
        std::string ColorRefToHex(COLORREF color);
    }
}