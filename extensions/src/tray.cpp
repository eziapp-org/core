#include "tray.hpp"
#include "extensions.hpp"
#include "bridge.hpp"
#include "resource.hpp"
#include "window.hpp"
#include "application.hpp"
#include "dialog.hpp"

#define WM_TRAYICON (WM_USER + 1)

#define TRAY_MENU_EXIT_ID 1001
#define TRAY_MENU_SHOW_MAIN_WINDOW_ID 1002

namespace ezi
{

    namespace Private
    {
        static Window& GetWindowById(const String& id)
        {
            WinId winId = reinterpret_cast<WinId>(std::stoull(id));
            return Application::GetInstance().GetWindowById(winId);
        }

        static std::vector<MenuItem> ParseMenuItemsFromJson(const std::vector<Object>& items)
        {
            std::vector<MenuItem> menuItems;
            for(const auto& item : items)
            {
                MenuItem    menuItem;
                std::string type = item["type"];
                if(type == "normal")
                    menuItem.type = MenuItem::Type::NORMAL;
                else if(type == "separator")
                    menuItem.type = MenuItem::Type::SEPARATOR;
                else if(type == "submenu")
                    menuItem.type = MenuItem::Type::SUBMENU;
                else
                    throw std::runtime_error("Invalid menu item type: " + type);

                menuItem.id      = item.contains("id") ? static_cast<int>(item["id"]) : 0;
                menuItem.label   = item.contains("label") ? static_cast<std::string>(item["label"]) : "";
                menuItem.enabled = item.contains("enabled") ? static_cast<bool>(item["enabled"]) : true;
                menuItem.checked = item.contains("checked") ? static_cast<bool>(item["checked"]) : false;

                if(menuItem.id < 2000 || menuItem.id >= 10000)
                {
                    throw std::runtime_error("Menu item id must be between 2000 and 9999");
                }

                if(menuItem.type == MenuItem::Type::SUBMENU && item.contains("submenu"))
                {
                    menuItem.submenuItems = ParseMenuItemsFromJson(item["submenu"]);
                }

                menuItems.push_back(menuItem);
            }
            return menuItems;
        }

        static void AppendBaseMenuItems(HMENU hMenu, bool hasCustomItems)
        {
            static auto appName = CFGRES<std::string>("application.name", "EziApp");
            if(hasCustomItems)
            {
                AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
            }
            AppendMenu(hMenu, MF_STRING, TRAY_MENU_SHOW_MAIN_WINDOW_ID, utf8ToGbk("显示主窗口").c_str());
            AppendMenu(hMenu, MF_STRING, TRAY_MENU_EXIT_ID, utf8ToGbk("退出 " + appName).c_str());
        }

        static void ClearTrayMenu(HMENU hMenu)
        {
            int count = GetMenuItemCount(hMenu);
            for(int i = count - 1; i >= 0; --i)
            {
                RemoveMenu(hMenu, 0, MF_BYPOSITION);
            }
        }

        static void AppendParsedMenu(HMENU hMenu, const std::vector<MenuItem>& items)
        {
            for(const auto& item : items)
            {
                UINT flags = NULL;
                if(!item.enabled)
                    flags |= MF_GRAYED;
                if(item.checked)
                    flags |= MF_CHECKED;

                if(item.type == MenuItem::Type::SEPARATOR)
                {
                    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
                }
                else if(item.type == MenuItem::Type::SUBMENU)
                {
                    HMENU hSubMenu = CreatePopupMenu();
                    AppendParsedMenu(hSubMenu, item.submenuItems);
                    AppendMenu(
                        hMenu, flags | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), utf8ToGbk(item.label).c_str());
                }
                else
                {

                    AppendMenu(hMenu, flags | MF_STRING, item.id, utf8ToGbk(item.label).c_str());
                }
            }
        }
    }

    Tray& Tray::GetInstance()
    {
        static Tray instance;
        return instance;
    }
#if OS(WINDOWS)
    LRESULT CALLBACK Tray::TrayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch(uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_TRAYICON:
            switch(lParam)
            {
            case WM_LBUTTONUP:
            {
                PostMessage(hwnd, WM_COMMAND, TRAY_MENU_SHOW_MAIN_WINDOW_ID, 0);
                break;
            }
            case WM_RBUTTONUP:
            {
                auto& tray = Tray::GetInstance();
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(tray.hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                break;
            }
            default:
                break;
            }
            break;
        case WM_COMMAND:
        {
            auto eventId = LOWORD(wParam);
            switch(eventId)
            {
            case TRAY_MENU_EXIT_ID:
            {
                Application::GetInstance().Exit(0);
                break;
            }
            case TRAY_MENU_SHOW_MAIN_WINDOW_ID:
            {
                auto mainWindow = Tray::GetInstance().mainWindow;
                // 如果窗口早被删除，就选第一个存在的窗口
                if(!IsWindow(mainWindow))
                {
                    auto& windows = Application::GetInstance().GetWindowList();
                    mainWindow    = windows.at(0)->GetWinId();
                }
                if(IsIconic(mainWindow))
                    ShowWindow(mainWindow, SW_RESTORE);
                else
                {
                    ShowWindow(mainWindow, SW_SHOW);
                    SetForegroundWindow(mainWindow);
                }
                break;
            }
            default:
            {
                println("Tray menu item clicked: ", eventId);
                auto& tray = Tray::GetInstance();
                if(tray.eventReceiver == nullptr)
                    break;
                try
                {
                    auto& win = Private::GetWindowById(std::to_string(reinterpret_cast<uint64_t>(tray.eventReceiver)));
                    win.ExecuteScript("window.__TrayMenuItemClickCallback_(" + std::to_string(eventId) + ");");
                }
                catch(...)
                {
                    Dialog dialog(nullptr, tray.appName);

                    DialogButtons buttons = { { 100, "忽略" }, { 200, "退出程序" } };

                    int result = dialog.Message(
                        tray.appName + " 错误", "托盘事件接收窗口丢失", MessageType::Error, buttons, 200);
                    if(result == 200)
                    {
                        Application::GetInstance().Exit(0);
                    }
                }
                break;
            }
            }
            break;
        }

        default:
            break;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
#endif
    Tray::Tray()
    {
#if OS(WINDOWS)
        appName = CFGRES<std::string>("application.name", "EziApp");
        // 注册窗口类
    #define EziTrayClassName "EziTrayClass"

        HINSTANCE hInstance = GetModuleHandle(nullptr);

        WNDCLASS wc      = { 0 };
        wc.lpfnWndProc   = TrayProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = EziTrayClassName;
        RegisterClass(&wc);
        // 创建托盘窗口
        hTrayWnd
            = CreateWindowEx(0, EziTrayClassName, "EziTrayWindow", 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
        hTrayMenu = CreatePopupMenu();

        nid.cbSize           = sizeof(nid);
        nid.hWnd             = hTrayWnd;
        nid.uID              = 1;
        nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(1));
        strncpy_s(nid.szTip, utf8ToGbk(appName).c_str(), appName.size());
        isShown = false;
#endif
    }
    Tray::~Tray()
    {
#if OS(WINDOWS)
        if(isShown)
            Shell_NotifyIcon(NIM_DELETE, &nid);
        DestroyMenu(hTrayMenu);
        DestroyWindow(hTrayWnd);
        UnregisterClass("EziTrayClass", GetModuleHandle(nullptr));
#endif
    }
#if OS(WINDOWS)

    void Tray::Show(WinId mainWindow)
    {
        if(isShown)
            return;
        isShown = true;

        this->mainWindow = mainWindow;
        if(GetMenuItemCount(hTrayMenu) == 0)
        {
            Private::AppendBaseMenuItems(hTrayMenu, false);
        }
        Shell_NotifyIcon(NIM_ADD, &nid);
    }

    void Tray::Hide()
    {
        if(!isShown)
            return;
        isShown = false;
        Shell_NotifyIcon(NIM_DELETE, &nid);
        Application::GetInstance().ExitIfNoVisibleWindow();
    }
#endif

    bool Tray::IsShown()
    {
        return isShown;
    }

    void Tray::SetContextMenu(std::vector<MenuItem> menuItems)
    {
        this->menuItems = menuItems;
        Private::ClearTrayMenu(hTrayMenu);
        Private::AppendParsedMenu(hTrayMenu, menuItems);
        Private::AppendBaseMenuItems(hTrayMenu, !menuItems.empty());
    }

    void Tray::SetEventReceiver(WinId window)
    {
        this->eventReceiver = window;
    }

    namespace tray
    {
        Object show(Object args)
        {
            auto& window       = Private::GetWindowById(args["mainWindowId"]);
            auto& senderWindow = Private::GetWindowById(args["senderWinId"]);
            auto& tray         = Tray::GetInstance();
            tray.SetEventReceiver(senderWindow.GetWinId());
            tray.Show(window.GetWinId());
            return "success";
        }

        Object hide(Object args)
        {
            Tray::GetInstance().Hide();
            return "success";
        }

        Object setContextMenu(Object args)
        {
            std::vector<MenuItem> menuItems = Private::ParseMenuItemsFromJson(args["menuItems"]);
            Tray::GetInstance().SetContextMenu(menuItems);
            return "success";
        }
    }

    namespace tray
    {
        void Mount()
        {
            Tray::GetInstance();

            REG(tray, show);
            REG(tray, hide);
            REG(tray, setContextMenu);
        }
    }
}