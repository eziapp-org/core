#pragma once
#include "platform.hpp"
#include <unordered_map>
#include <functional>
#include <string>

#include "json.hpp"

#if OS(WINDOWS)
    #include <WebView2.h>
    #include <wil/com.h>
#endif

namespace ezi
{
    typedef std::function<Object(Object)>        Function;
    typedef std::unordered_map<String, Function> Functions;

#if OS(WINDOWS)
    typedef wil::com_ptr<ICoreWebView2> View;
    typedef HWND                        WinId;
#endif
    class Bridge
    {
    private:
        Functions functions;

    private:
        Bridge()                         = default;
        Bridge(const Bridge&)            = delete;
        Bridge& operator=(const Bridge&) = delete;

    public:
        static Bridge& GetInstance();

    public:
        void ExposeTo(View& view, WinId winId);
        Json Call(String func, Json args);
        void Register(String name, Function func);
    };
} // namespace ezi