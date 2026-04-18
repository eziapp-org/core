#include "extensions.hpp"
#include "bridge.hpp"
#include "application.hpp"
#include "window.hpp"

// 特殊按键map
static std::unordered_map<std::string, UINT> specialKeyMap = {
    { "enter", VK_RETURN },
    { "backspace", VK_BACK },
    { "tab", VK_TAB },
    { "shift", VK_SHIFT },
    { "ctrl", VK_CONTROL },
    { "alt", VK_MENU },
    { "pause", VK_PAUSE },
    { "capslock", VK_CAPITAL },
    { "escape", VK_ESCAPE },
    { "space", VK_SPACE },
    { "pageup", VK_PRIOR },
    { "pagedown", VK_NEXT },
    { "end", VK_END },
    { "home", VK_HOME },
    { "left", VK_LEFT },
    { "up", VK_UP },
    { "right", VK_RIGHT },
    { "down", VK_DOWN },
    { "insert", VK_INSERT },
    { "delete", VK_DELETE },
};

namespace ezi{
    namespace Private
    {
        static Window& GetWindowById(const String& id)
        {
            WinId winId = reinterpret_cast<WinId>(std::stoull(id));
            return Application::GetInstance().GetWindowById(winId);
        }
    }
    
    namespace shortcut
    {
        Object registerHotKey(Object args)
        {
            auto& senderWindow = Private::GetWindowById(args["senderWinId"]);
            Object options = args["options"];

            // 解析修饰键
            UINT modifiers = 0;
            if(options.value("ctrl", false))
                modifiers |= MOD_CONTROL;
            if(options.value("alt", false))
                modifiers |= MOD_ALT;
            if(options.value("shift", false))
                modifiers |= MOD_SHIFT;
            if(options.value("meta", false))
                modifiers |= MOD_WIN;


            // 获取按键的虚拟键码
            std::string key = options["key"];
            UINT vk = 0;
            if(key.length() == 1)
            {
                // 获取该字符变成大写的ASCII码
                vk = toupper(key[0]);
            }
            else
            {
                auto it = specialKeyMap.find(key);
                if(it != specialKeyMap.end())
                {
                    vk = it->second;
                }
                else
                {
                    throw std::runtime_error("Unsupported key: " + key);
                }
            }

            if(!RegisterHotKey(senderWindow.GetWinId(), args["id"], modifiers, vk)){
                throw std::runtime_error("Failed to register hotkey");
            }
            return "success";
        }
    }

    namespace shortcut
    {
        void Mount()
        {
            REG(shortcut, registerHotKey);
        }

    }
}