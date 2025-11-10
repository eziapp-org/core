#pragma once
#include "platform.hpp"
#include <vector>

#if OS(WINDOWS)
    #include <windows.h>
#endif

namespace ezi
{
    struct MenuItem
    {
        enum class Type
        {
            NORMAL,
            SEPARATOR,
            SUBMENU
        };

        Type                  type;
        int                   id;
        std::string           label;
        bool                  enabled;
        bool                  checked;
        std::vector<MenuItem> submenuItems;
    };

    class Tray
    {
    private:
#if OS(WINDOWS)
        typedef HWND WinId;
        HWND         hTrayWnd;
        HMENU        hTrayMenu;

        NOTIFYICONDATA nid;

        bool isShown;

        static LRESULT CALLBACK TrayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
        WinId                 mainWindow;
        std::string           appName;
        std::vector<MenuItem> menuItems;
        WinId                 eventReceiver;

    private:
        Tray();
        ~Tray();
        Tray(const Tray&)            = delete;
        Tray& operator=(const Tray&) = delete;

    public:
        static Tray& GetInstance();

        void Show(WinId mainWindow);
        void Hide();
        bool IsShown();

        void SetContextMenu(std::vector<MenuItem> menuItems);
        void SetEventReceiver(WinId window);
    };
    namespace tray
    {
        void Mount();
    }
}