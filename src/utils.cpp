#include "utils.hpp"

namespace ezi
{
    namespace Utils
    {
        UISettings uiSettings;

        bool IsDarkMode()
        {
            auto color  = uiSettings.GetColorValue(UIColorType::Background);
            bool isDark = (color.R + color.G + color.B) / 3 < 128;
            return isDark;
        }

        COLORREF GetAccentColor()
        {
            auto accent = uiSettings.GetColorValue(UIColorType::Accent);
            return RGB(accent.R, accent.G, accent.B);
        }

        std::string GetArg(std::string key)
        {
#if COMPILER(MSVC)
            int    argc = __argc;
            char** argv = __argv;
            for(int i = 0; i < argc; i++)
            {
                std::string arg = argv[i];
                if(arg == key && i + 1 < argc)
                {
                    return argv[i + 1];
                }
            }
#endif
            return "";
        }

        std::string ColorRefToHex(COLORREF color)
        {
            char hex[8];
            std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
            return std::string(hex);
        }

    }
}