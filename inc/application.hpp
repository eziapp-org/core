#pragma once
#include <vector>
#include "platform.hpp"
#include "json.hpp"
#include "ezienv.hpp"

#if OS(WINDOWS)
    #include <gdiplus.h>
    #pragma comment(lib, "Gdiplus.lib")
#endif

namespace ezi
{
    struct SystemVersion
    {
        int major;
        int minor;
        int build;
    };

    class Webview;
    class Window;

    typedef std::vector<Window*> WindowList;
    typedef HWND                 WinId;

    class Application
    {
    private:
        WindowList windows;
        Window*    masterWindow = nullptr;
        ULONG_PTR  gdiplusToken;

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;

    private:
        Application();
        Application(const Application&)            = delete;
        Application& operator=(const Application&) = delete;
        ~Application();

    public:
        Window&       CrtWindowByOption(const Object& options);
        void          DelWindowById(WinId winId);
        Window&       GetWindowById(WinId winId);
        WindowList&   GetWindowList();
        SystemVersion GetSystemVersion();

    public:
        static Application& GetInstance();

        int Run();
        int Exit(int code);

        void ExitIfNoVisibleWindow();
    };
} // namespace ezi
