#include "application.hpp"
#include "platform.hpp"
#include "print.hpp"
#include "window.hpp"

#if OS(WINDOWS)
    #include <shlwapi.h>
    #include <windows.h>
#endif
#include "resource.hpp"
#include "ezienv.hpp"
#include "tray.hpp"
#include "dialog.hpp"

namespace ezi
{
    Application::Application()
#if OS(WINDOWS)
    {
        // 检查是否单例模式
        if(CFGRES<bool>("application.singleInstance", false))
        {
            String mutexName = "EziAppSingleInstanceMutex_" + CFGRES<String>("application.package", "com.ezi.app");
            HANDLE hMutex    = CreateMutexA(NULL, FALSE, mutexName.c_str());
            if(GetLastError() == ERROR_ALREADY_EXISTS)
            {
                Dialog dialog(nullptr, CFGRES<String>("application.name", "Ezi App"));
                dialog.Alert("应用已经在运行中！");
                exit(0);
            }
        }

        // 初始化COM
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        // 初始化WebView2环境
        auto& webview = Webview::GetInstance();
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

    Application& Application::GetInstance()
    {
        static Application instance;
        return instance;
    }

    int Application::Run()
#if OS(WINDOWS)
    {
        MSG msg;
        while(GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }
#endif

    Window& Application::CrtWindowByOption(const Object& options)
    {
        Window* window = new Window(options);
        if(windows.empty())
        {
            masterWindow = window;
        }

        String src = at<String>(options, "src", "index.html");
        if(src.starts_with("http"))
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

    void Application::DelWindowById(WinId winId)
    {
        auto isMaster        = (masterWindow && masterWindow->GetWinId() == winId);
        auto isRecordPostion = false;

        if(isMaster)
        {
            masterWindow = nullptr;
            auto config  = ezi::Resource::GetInstance().GetConfig();
            if(config.contains("window") && config["window"].contains("position"))
            {
                auto position = config["window"]["position"];
                if(position.is_string() && position.get<String>() == "remembered")
                {
                    isRecordPostion = true;
                }
            }
        }

        if(isRecordPostion)
        {
            Window&  win    = GetWindowById(winId);
            Position winPos = win.GetPosition();
            EziEnv::GetInstance().SetRememberedWindowPosition(winPos);
        }

        for(auto it = windows.begin(); it != windows.end(); ++it)
        {
            if((*it)->GetWinId() == winId)
            {
                delete *it;
                windows.erase(it);
                break;
            }
        }
        if(windows.empty())
        {
            Exit(0);
        }
        else
        {
            ExitIfNoVisibleWindow();
        }
    }

    Window& Application::GetWindowById(WinId winId)
    {
        for(auto& win : windows)
        {
            if(win->GetWinId() == winId)
            {
                return *win;
            }
        }
        throw std::runtime_error("Window not found");
    }

    WindowList& Application::GetWindowList()
    {
        return windows;
    }

    int Application::Exit(int code)
    {
        for(auto& win : windows)
        {
            win->Close();
        }
        windows.clear();
        Webview::GetInstance().GetEnv()->Release();
        CoUninitialize();
        PostQuitMessage(code);
        return code;
    }

    void Application::ExitIfNoVisibleWindow()
    {
        if(Tray::GetInstance().IsShown())
            return;
        bool hasWindowVisible = false;
        for(auto& win : windows)
        {
            if(win->IsVisible())
            {
                hasWindowVisible = true;
                break;
            }
        }
        if(!hasWindowVisible)
        {
#if BUILDTYPE(DEBUG)
            std::cout << "\x1b[33mapplication without tray cannot hide all windows.\x1b[0m" << std::endl;
#endif
            Exit(0);
        }
    }

    SystemVersion Application::GetSystemVersion()
    {
        SystemVersion version = { 0, 0, 0 };
#if OS(WINDOWS)
        typedef LONG NTSTATUS;
        typedef NTSTATUS(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

        HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
        if(hMod)
        {
            RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
            if(fxPtr != nullptr)
            {
                RTL_OSVERSIONINFOW osInfo  = { 0 };
                osInfo.dwOSVersionInfoSize = sizeof(osInfo);
                if(0x00000000 == fxPtr(&osInfo))
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