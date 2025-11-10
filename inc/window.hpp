#pragma once
#include <string>
#include "platform.hpp"
#if OS(WINDOWS)
    #include <WebView2.h>
    #include <wil/com.h>
    #include <gdiplus.h>
#endif
#include "webview.hpp"
#include "json.hpp"
#include <functional>

namespace ezi
{

    enum class BackgroundMode
    {
        opaque,
        transparent,
        mica,
        acrylic
    };

    enum class WindowStatus
    {
        Loading,
        Switching,
        Ready,
        Error,
        Destroyed,
    };

    struct Size
    {
        int width;
        int height;
    };

    struct Position
    {
        int x;
        int y;
    };

    struct Splash
    {
        Gdiplus::Image* image;

        float width;
        float height;
        float aplha;
    };

    class Window
    {
    private:
        String       title;
        String       url;
        WinId        winId;
        Controller   controller;
        View         view;
        WindowStatus status;
        String       accentColor;
        Splash       splash;

        std::function<bool()> onCloseCallback;

        BackgroundMode backgroundMode = BackgroundMode::opaque;

    public:
        Window(const Object& options);

    public:
        void ExecuteScript(String script);

    public:
        String       GetUrl() const;
        WinId        GetWinId() const;
        WindowStatus GetStatus() const;
        Controller   GetController();
        View         GetView();
        float        GetWidth() const;
        float        GetHeight() const;
        float        GetScaleFactor() const;
        String       GetTitle() const;
        String       GetAccentColor() const;
        Splash&      GetSplash();

        BackgroundMode GetBackgroundMode() const;
        Size           GetSize() const;
        Position       GetPosition() const;

        std::function<bool()>& GetOnCloseCallback();

    public:
        void SetController(Controller controller);
        void SetView(View view);
        void SetStatus(WindowStatus status);
        void SetOnCloseCallback(std::function<bool()> callback);

    public:
        void SetUrl(String url);
        void SetTitle(String title);
        void SetCaptionColor(DWORD color);
        void SetBackgroundMode(BackgroundMode mode);
        void SetSize(Size size);
        void SetPosition(Position position);
        void SetMaximizable(bool enable);
        void SetMinimizable(bool enable);
        void SetMovable(bool enable);
        void SetFocusable(bool enable);
        void SetBorderless(bool enable);

    public:
        void Close();
        void Reload();
        void Focus();
        void Blur();
        void Minimize();
        void Maximize();
        void Restore();
        void Hide();
        void Show();
        void Drag();

    public:
        bool IsMaximizable();
        bool IsMaximized();
        bool IsMinimizable();
        bool IsMinimized();
        bool IsMovable();
        bool IsFocusable();
        bool IsFocused();
        bool IsVisible();
        bool IsBorderless();
    };
}