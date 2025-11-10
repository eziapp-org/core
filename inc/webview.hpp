#pragma once
#include "platform.hpp"
#if OS(WINDOWS)
    #include <WebView2.h>
    #include <wil/com.h>
#endif

#if OS(WINDOWS)
typedef wil::com_ptr<ICoreWebView2Environment> Env;
typedef wil::com_ptr<ICoreWebView2Controller>  Controller;
typedef wil::com_ptr<ICoreWebView2>            View;
typedef HWND                                   WinId;
#endif

namespace ezi
{
    class Window;

    class Webview
    {
    private:
        Env env;

    private:
        Webview()                          = default;
        Webview(const Webview&)            = delete;
        Webview& operator=(const Webview&) = delete;

    public:
        static Webview& GetInstance();
        Env             GetEnv();

    public:
        void CreateEnv();
        void CreateController(Window& window);
    };
}