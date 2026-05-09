#include "application.hpp"
#include "platform.hpp"
#include "print.hpp"
#include "window.hpp"

#if OS(WINDOWS)
#include <shlwapi.h>
#include <windows.h>
#endif
#include "dialog.hpp"
#include "ezienv.hpp"
#include "resource.hpp"
#include "tray.hpp"

namespace ezi
{
Application::Application()
#if OS(WINDOWS)
{
    auto packageName = CFGRES<String>("application.package", "com.ezi.app");
    this->windowClassName = "EziWindowClass_" + packageName;

    // 检查是否单例模式
    if (CFGRES<bool>("application.singleInstance", false))
    {
        String mutexName = "EziAppSingleInstanceMutex_" + packageName;
        HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName.c_str());
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            HWND hWnd = FindWindow(this->windowClassName.c_str(), NULL);
            if (hWnd)
            {
                ShowWindow(hWnd, SW_SHOW);
                SetForegroundWindow(hWnd);
            }
            exit(0);
        }
    }

    // 初始化COM
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // 初始化WebView2环境
    auto &webview = Webview::GetInstance();
    webview.CreateEnv();
    // 初始化GDI+
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    // 初始化EziEnv
    EziEnv::GetInstance();
}
#endif

Application::~Application()
{
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

Application &Application::GetInstance()
{
    static Application instance;
    return instance;
}

int Application::Run()
#if OS(WINDOWS)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
#endif

Window &Application::CrtWindowByOption(const Object &options)
{
    Window *window = new Window(options);
    if (windows.empty())
    {
        masterWindow = window;
    }

    String src = at<String>(options, "src", "index.html");
    if (src.starts_with("http"))
        window->SetUrl(src);
    else
    {
#if BUILDTYPE(DEBUG)
        window->SetUrl(CFGRES<String>("application.devEntry", "http://localhost:5173") + src);
#else
        static String baseUrl = "https://" + CFGRES<String>("application.package", "com.ezi.app") + "/";
        window->SetUrl(baseUrl + src);
#endif
    }

    windows.push_back(window);
    return *window;
}

// 只能在WM_CLOSE消息中调用
void Application::DelWindowById(WinId winId)
{
    // 判断是否要记录窗口位置
    if (masterWindow != nullptr && masterWindow->GetWinId() == winId)
    {
        // 删除的窗口是主窗口
        masterWindow = nullptr;

        // 判断是否需要记录窗口位置
        auto config = ezi::Resource::GetInstance().GetConfig();
        if (config.contains("window") && config["window"].contains("position"))
        {
            auto position = config["window"]["position"];
            if (position.is_string() && position.get<String>() == "remembered")
            {
                Position winPos = GetWindowById(winId).GetPosition();
                EziEnv::GetInstance().SetRememberedWindowPosition(winPos);
            }
        }
    }

    // 删除窗口
    for (auto it = windows.begin(); it != windows.end(); ++it)
    {
        if ((*it)->GetWinId() == winId)
        {
            delete *it;
            windows.erase(it);
            break;
        }
    }

    if (windows.empty())
    {
        // 没有窗口要退出应用
        if (!exiting)
        {
            Exit(0);
        }
        else
        {
            PostQuitMessage(0);
        }
    }
    else
    {
        // 没有可以被用户看到的窗口也要退出应用
        ExitIfNoVisibleWindow();
    }
}

Window &Application::GetWindowById(WinId winId)
{
    for (auto &win : windows)
    {
        if (win->GetWinId() == winId)
        {
            return *win;
        }
    }
    throw std::runtime_error("Window not found");
}

String Application::GetWindowClassName()
{
    return windowClassName;
}

WindowList &Application::GetWindowList()
{
    return windows;
}

int Application::Exit(int code)
{
    if (exiting)
    {
        PostQuitMessage(code);
        return code;
    }
    exiting = true;

    // 同步关闭窗口，确保 Window/controller/view 在 COM 反初始化前释放
    std::vector<HWND> winIds;
    winIds.reserve(windows.size());
    for (auto *win : windows)
    {
        if (win)
        {
            winIds.push_back(win->GetWinId());
        }
    }
    for (auto hwnd : winIds)
    {
        if (hwnd)
        {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
    }

    Webview::GetInstance().Shutdown();
    CoUninitialize();
    PostQuitMessage(code);
    return code;
}

void Application::ExitIfNoVisibleWindow()
{
    if (Tray::GetInstance().IsShown())
        return;
    bool hasWindowVisible = false;
    for (auto &win : windows)
    {
        if (win->IsVisible())
        {
            hasWindowVisible = true;
            break;
        }
    }
    if (!hasWindowVisible)
    {
#if BUILDTYPE(DEBUG)
        std::cout << "\x1b[33mapplication without tray cannot hide all windows.\x1b[0m" << std::endl;
#endif
        Exit(0);
    }
}

SystemVersion Application::GetSystemVersion()
{
    SystemVersion version = {0, 0, 0};
#if OS(WINDOWS)
    typedef LONG NTSTATUS;
    typedef NTSTATUS(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr)
        {
            RTL_OSVERSIONINFOW osInfo = {0};
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            if (0x00000000 == fxPtr(&osInfo))
            {
                version.major = static_cast<int>(osInfo.dwMajorVersion);
                version.minor = static_cast<int>(osInfo.dwMinorVersion);
                version.build = static_cast<int>(osInfo.dwBuildNumber);
            }
        }
    }
#endif
    return version;
}

} // namespace ezi