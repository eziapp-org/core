#include "window.hpp"
#include "application.hpp"
#include "json.hpp"
#include "platform.hpp"
#include "print.hpp"
#include "utils.hpp"

#if OS(WINDOWS)
#include <dwmapi.h>
#include <windows.h>
using namespace Gdiplus;
#endif

#include "dialog.hpp"
#include "resource.hpp"

namespace ezi
{
namespace Private
{
#if OS(WINDOWS)
static void UpdateWindowTheme(HWND hwnd)
{
    // 使用 bool 会导致设置失败
    BOOL isDark = Utils::IsDarkMode();
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));
}

static void DrawSplashScreen(Graphics &graphics, Window &window)
{
    auto &splash = window.GetSplash();

    auto winWidth = window.GetWidth();
    auto winHeight = window.GetHeight();

    float x = (winWidth - splash.width) / 2;
    float y = (winHeight - splash.height) / 2;

    if (splash.image == nullptr)
    {
        return;
    }
    ColorMatrix colorMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,         1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, splash.aplha, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    ImageAttributes imgAttr;
    imgAttr.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

    graphics.DrawImage(splash.image, RectF(x, y, splash.width, splash.height), 0, 0, splash.image->GetWidth(),
                       splash.image->GetHeight(), UnitPixel, &imgAttr);
}
#endif
} // namespace Private
} // namespace ezi

namespace ezi
{
#if OS(WINDOWS)

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window *window = (Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (uMsg)
    {
    case WM_TIMER: {
        if (wParam == 1)
        {
            auto status = window->GetStatus();
            if (status != WindowStatus::Switching)
            {
                KillTimer(hwnd, 1);
                return 0;
            }
            if (status == WindowStatus::Switching)
            {
                auto &splash = window->GetSplash();
                // 如果是透明背景，立即切换到Ready状态
                // 因为透明窗口绘制不了slpash
                if (window->GetBackgroundMode() == BackgroundMode::transparent)
                {
                    window->SetStatus(WindowStatus::Ready);
                    KillTimer(hwnd, 1);
                    return 0;
                }
                splash.aplha -= 0.1f;
                if (splash.aplha < 0.0f)
                {
                    window->SetStatus(WindowStatus::Ready);
                    KillTimer(hwnd, 1);
                    return 0;
                }
                // 发起绘制请求
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        break;
    }
    case WM_NCCALCSIZE: {
        if (window != nullptr && window->IsBorderless())
            return 0;
        break;
    }

    case WM_SETTINGCHANGE: {
        Private::UpdateWindowTheme(hwnd);
        if (window->GetAccentColor() == "system")
        {
            auto view = window->GetView();
            if (!view)
                break;
            String accentColor = Utils::ColorRefToHex(Utils::GetAccentColor());
            view->ExecuteScript(
                utf8ToUtf16("document.body.style.setProperty('--ezi-accent-color', '" + accentColor + "');").c_str(),
                nullptr);
        }
        break;
    }
    case WM_PAINT: {
        if (window->GetBackgroundMode() == BackgroundMode::transparent)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)CreateSolidBrush(RGB(0, 255, 1)));
            EndPaint(hwnd, &ps);
            break;
        }
        if (window->GetStatus() == WindowStatus::Ready)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
            EndPaint(hwnd, &ps);
            return 0;
        }
        PAINTSTRUCT ps;

        HDC hdc = BeginPaint(hwnd, &ps);

        int width = ps.rcPaint.right - ps.rcPaint.left;
        int height = ps.rcPaint.bottom - ps.rcPaint.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        Graphics graphics(memDC);

        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        // 抗锯齿
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        GraphicsState state = graphics.Save();

        // 获取系统缩放因子
        float scaleFactor = window->GetScaleFactor();
        graphics.ScaleTransform(scaleFactor, scaleFactor);
        Private::DrawSplashScreen(graphics, *window);
        graphics.Restore(state);
        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_CLOSE: {
        auto &callback = window->GetOnCloseCallback();
        if (callback)
        {
            bool showClose = callback();
            if (!showClose)
                return 0;
        }
        Application::GetInstance().DelWindowById(hwnd);
        break;
    }

    case WM_SIZE: {
        InvalidateRect(hwnd, nullptr, FALSE);
        auto controller = window->GetController();
        if (controller)
        {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            controller->put_Bounds(bounds);
        }
        break;
    }
    case WM_HOTKEY: {
        window->ExecuteScript("window.__ShortCutCallback_" + std::to_string(wParam) + "();");
        break;
    }

    case WM_GETMINMAXINFO: {
        if (window == nullptr)
            break;
        MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
        auto minSize = window->GetMinSize();
        auto scaleFactor = window->GetScaleFactor();
        mmi->ptMinTrackSize.x = minSize.width * scaleFactor;
        mmi->ptMinTrackSize.y = minSize.height * scaleFactor;
        return 0;
    }
    case WM_SYSCOMMAND:
        if (window == nullptr)
            break;
        if (!window->IsMovable() && (wParam & 0xFFF0) == SC_MOVE)
            return 0; // 禁止移动
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

#if OS(WINDOWS)
void Window::Show()
{
    ShowWindow(this->winId, SW_SHOW);
    UpdateWindow(this->winId);
}
#endif

#if OS(WINDOWS)
Window::Window(const Object &options)
{
    status = WindowStatus::Loading;
    auto &app = ezi::Application::GetInstance();

    // 注册窗口类
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);

    auto className = app.GetWindowClassName();
    auto cClassName = className.c_str();
    if (!GetClassInfoEx(hInstance, cClassName, &wcex))
    {
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.lpszClassName = cClassName;
        wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
        wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(1));

        if (!RegisterClassEx(&wcex))
        {
            auto code = GetLastError();
            MessageBox(nullptr, utf8ToGbk("注册窗口类失败，应用即将关闭。" + std::to_string(code)).c_str(), "Error",
                       MB_OK);
            exit(1);
        }
    }

    // 不能使用window.GetScaleFactor获取dpi，因为窗口还没有创建
    float scaleFactor = 1.0f;
    {
        HDC screen = GetDC(nullptr);
        int dpi = GetDeviceCaps(screen, LOGPIXELSX);
        scaleFactor = dpi / 96.0f;
        ReleaseDC(nullptr, screen);
    }

    // 设置标题
    title = options.value("title", "EziApplication");

    BackgroundMode bgMode = BackgroundMode::opaque;
    String bgModeStr = options.value("backgroundMode", "opaque");
    if (bgModeStr == "opaque")
    {
        bgMode = BackgroundMode::opaque;
    }
    else if (bgModeStr == "transparent")
    {
        bgMode = BackgroundMode::transparent;
    }
    else if (bgModeStr == "mica")
    {
        bgMode = BackgroundMode::mica;
    }
    else if (bgModeStr == "acrylic")
    {
        bgMode = BackgroundMode::acrylic;
    }

    // 窗口大小
    int width = at(options, "size.width", 800) * scaleFactor;
    int height = at(options, "size.height", 600) * scaleFactor;

    // 窗口位置
    RECT desktopRect;
    GetWindowRect(GetDesktopWindow(), &desktopRect);
    int centerX = (desktopRect.right - desktopRect.left - width) / 2;
    int centerY = (desktopRect.bottom - desktopRect.top - height) / 2;

    int x = centerX;
    int y = centerY;

    if (options.contains("position"))
    {
        auto position = options["position"];
        if (position.is_object())
        {
            if (position.contains("x"))
                x = position.get<int>() * scaleFactor;
            if (position.contains("y"))
                y = position.get<int>() * scaleFactor;
        }
        else if (position.is_string())
        {
            auto posStr = position.get<String>();
            if (posStr == "remembered")
            {
                try
                {
                    auto winPos = EziEnv::GetInstance().GetRememberedWindowPosition();

                    x = winPos.x * scaleFactor;
                    y = winPos.y * scaleFactor;
                }
                catch (const std::exception &e)
                {
                    println("Get remembered window position failed");
                }
            }
        }
    }

    // 强调色
    accentColor = at<String>(options, "accentColor", "system");

    // 获取splash配置
    String defaultSplashSrc = CFGRES<String>("window.splashscreen.src", "logo.png");

    splash.width = at<float>(options, "splashscreen.size.width", 150);
    splash.height = at<float>(options, "splashscreen.size.height", 150);
    String splashSrc = at<String>(options, "splashscreen.src", defaultSplashSrc);
    splash.image = Resource::GetInstance().GetImage(splashSrc);
    splash.aplha = 1.0f;

    // 创建窗口
    this->winId = CreateWindowEx(0, cClassName, utf8ToGbk(title).c_str(), WS_OVERLAPPEDWINDOW, x, y, width, height,
                                 nullptr, nullptr, hInstance, nullptr);

    if (this->winId == NULL)
    {
        auto code = GetLastError();
        MessageBox(nullptr, utf8ToGbk("窗口创建失败，应用即将关闭。" + std::to_string(code)).c_str(), "Error", MB_OK);
        exit(1);
    }

    // 创建WebView2控制器
    Webview::GetInstance().CreateController(*this);

    // 关联窗口实例
    SetWindowLongPtr(this->winId, GWLP_USERDATA, (LONG_PTR)this);

    // 无边框
    if (at<bool>(options, "borderless", false))
    {
        SetBorderless(true);
    }

    // maximizable
    if (at<bool>(options, "maximizable", true) == false)
    {
        SetMaximizable(false);
    }

    // minimizable
    if (at<bool>(options, "minimizable", true) == false)
    {
        SetMinimizable(false);
    }

    // movable
    if (at<bool>(options, "movable", true) == false)
    {
        SetMovable(false);
    }

    // resizable
    if (at<bool>(options, "resizable", true) == false)
    {
        SetResizable(false);
    }

    // minSize
    if (options.contains("minSize"))
    {
        auto minSize = options["minSize"];
        if (minSize.is_object())
        {
            Size size;
            size.width = at<int>(minSize, "width", 0) * scaleFactor;
            size.height = at<int>(minSize, "height", 0) * scaleFactor;
            SetMinSize(size);
        }
    }

    // alwaysOnTop
    if (at<bool>(options, "alwaysOnTop", false))
    {
        SetAlwaysOnTop(true);
    }

    // 设置背景模式
    SetBackgroundMode(bgMode);

    // 更新深色模式
    Private::UpdateWindowTheme(this->winId);
}
#endif

void Window::SetUrl(String url)
{
    this->url = url;
    if (view)
    {
        view->Navigate(utf8ToUtf16(url).c_str());
    }
    else
    {
        println("navigate fail, view is not created");
    }
}
void Window::SetTitle(String title)
{
    this->title = title;
    if (winId)
    {
        SetWindowText(winId, utf8ToGbk(title).c_str());
    }
}

String Window::GetUrl() const
{
    return this->url;
}

WinId Window::GetWinId() const
{
    return this->winId;
}

Controller Window::GetController()
{
    return this->controller;
}

void Window::SetController(Controller controller)
{
    this->controller = controller;
}

View Window::GetView()
{
    return this->view;
}

void Window::SetView(View view)
{
    this->view = view;
}

void Window::SetStatus(WindowStatus status)
{
    if (status == WindowStatus::Ready)
    {
        // 窗口准备好之后设置webview框架
        RECT bounds;
        GetClientRect(winId, &bounds);
        controller->put_Bounds(bounds);

        controller->put_IsVisible(TRUE);
    }
    else if (status == WindowStatus::Switching)
    {
        SetTimer(winId, 1, 16, nullptr);
    }

    this->status = status;
    InvalidateRect(this->winId, nullptr, TRUE);
}
void Window::SetCaptionColor(DWORD color)
{
    DwmSetWindowAttribute(this->winId, DWMWA_CAPTION_COLOR, &color, sizeof(color));
}
void Window::SetBackgroundMode(BackgroundMode mode)
{
    backgroundMode = mode;
    DWM_SYSTEMBACKDROP_TYPE type;
    switch (mode)
    {
    case BackgroundMode::opaque:
        type = DWM_SYSTEMBACKDROP_TYPE::DWMSBT_AUTO;
        break;
    case BackgroundMode::mica:
        type = DWM_SYSTEMBACKDROP_TYPE::DWMSBT_MAINWINDOW;
        break;
    case BackgroundMode::acrylic:
        type = DWM_SYSTEMBACKDROP_TYPE::DWMSBT_TRANSIENTWINDOW;
        break;
    case BackgroundMode::transparent:
        type = DWM_SYSTEMBACKDROP_TYPE::DWMSBT_NONE;
        break;

    default:
        break;
    }
    DwmSetWindowAttribute(this->winId, DWMWINDOWATTRIBUTE::DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));

    if (mode == BackgroundMode::transparent)
    {
        MARGINS margins = {0, 0, 0, 0};
        DwmExtendFrameIntoClientArea(this->winId, &margins);
        SetWindowLong(this->winId, GWL_EXSTYLE, GetWindowLong(this->winId, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(this->winId, RGB(0, 255, 1), 255, LWA_COLORKEY);
        InvalidateRect(this->winId, nullptr, TRUE);
    }
    else
    {
        MARGINS margins = {-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(this->winId, &margins);
        SetLayeredWindowAttributes(this->winId, 0, 255, LWA_ALPHA);
        SetWindowLong(this->winId, GWL_EXSTYLE, GetWindowLong(this->winId, GWL_EXSTYLE) & ~WS_EX_LAYERED);
        InvalidateRect(this->winId, nullptr, TRUE);
    }
}
WindowStatus Window::GetStatus() const
{
    return this->status;
}
float Window::GetWidth() const
{
    RECT rect;
    GetClientRect(this->winId, &rect);
    auto scaleFactor = GetScaleFactor();
    return (rect.right - rect.left) / scaleFactor;
}
float Window::GetHeight() const
{
    RECT rect;
    GetClientRect(this->winId, &rect);
    auto scaleFactor = GetScaleFactor();
    return (rect.bottom - rect.top) / scaleFactor;
}
float Window::GetScaleFactor() const
{
    auto dpi = GetDpiForWindow(this->winId);
    return dpi / 96.0f;
}
String Window::GetTitle() const
{
    return this->title;
}

void Window::Close()
{
    PostMessage(this->winId, WM_CLOSE, 0, 0);
    // 窗口示例在WM_CLOSE消息中被删除，这里不需要删除窗口实例
}

void Window::Reload()
{
    if (view)
    {
        view->Reload();
    }
}

void Window::Focus()
{
    auto winId = this->GetWinId();
    SetForegroundWindow(winId);
    SetFocus(winId);
}

void Window::Blur()
{
    SetFocus(nullptr);
}

void Window::Maximize()
{
    ShowWindow(this->winId, SW_MAXIMIZE);
}

void Window::Minimize()
{
    ShowWindow(this->winId, SW_MINIMIZE);
}

void Window::Restore()
{
    ShowWindow(this->winId, SW_RESTORE);
}

void Window::Hide()
{
    ShowWindow(this->winId, SW_HIDE);
    Application::GetInstance().ExitIfNoVisibleWindow();
}

void Window::Drag()
{
    ReleaseCapture();
    SendMessage(this->winId, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

bool Window::IsVisible()
{
    return IsWindowVisible(this->winId);
}

bool Window::IsMaximizable()
{
    return (GetWindowLong(this->winId, GWL_STYLE) & WS_MAXIMIZEBOX) != 0;
}

bool Window::IsMaximized()
{
    return IsZoomed(this->winId);
}

bool Window::IsMinimizable()
{
    return (GetWindowLong(this->winId, GWL_STYLE) & WS_MINIMIZEBOX) != 0;
}

bool Window::IsMinimized()
{
    return IsIconic(this->winId);
}

bool Window::IsMovable()
{
    return movable;
}

bool Window::IsFocusable()
{
    return (GetWindowLong(this->winId, GWL_STYLE) & WS_TABSTOP) != 0;
}

bool Window::IsFocused()
{
    return GetForegroundWindow() == this->winId;
}

bool Window::IsBorderless()
{
    return this->isBorderless;
}

BackgroundMode Window::GetBackgroundMode() const
{
    return this->backgroundMode;
}

Size Window::GetSize() const
{
    RECT rect;
    GetWindowRect(this->winId, &rect);
    auto scaleFactor = GetScaleFactor();
    return Size{static_cast<int>((rect.right - rect.left) / scaleFactor),
                static_cast<int>((rect.bottom - rect.top) / scaleFactor)};
}

Position Window::GetPosition() const
{
    RECT rect;
    GetWindowRect(this->winId, &rect);
    auto scaleFactor = GetScaleFactor();
    return Position{static_cast<int>(rect.left / scaleFactor), static_cast<int>(rect.top / scaleFactor)};
}

void Window::SetSize(Size size)
{
    auto scaleFactor = GetScaleFactor();
    SetWindowPos(this->winId, nullptr, 0, 0, static_cast<int>(size.width * scaleFactor),
                 static_cast<int>(size.height * scaleFactor), SWP_NOMOVE | SWP_NOZORDER);
}

void Window::SetPosition(Position position)
{
    auto scaleFactor = GetScaleFactor();
    SetWindowPos(this->winId, nullptr, static_cast<int>(position.x * scaleFactor),
                 static_cast<int>(position.y * scaleFactor), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Window::SetMaximizable(bool enable)
{
    LONG style = GetWindowLong(this->winId, GWL_STYLE);
    if (enable)
    {
        style |= WS_MAXIMIZEBOX;
    }
    else
    {
        style &= ~WS_MAXIMIZEBOX;
    }
    SetWindowLong(this->winId, GWL_STYLE, style);
}

void Window::SetMinimizable(bool enable)
{
    LONG style = GetWindowLong(this->winId, GWL_STYLE);
    if (enable)
    {
        style |= WS_MINIMIZEBOX;
    }
    else
    {
        style &= ~WS_MINIMIZEBOX;
    }
    SetWindowLong(this->winId, GWL_STYLE, style);
}

void Window::SetMovable(bool enable)
{
    movable = enable;
}

void Window::SetFocusable(bool enable)
{
    LONG style = GetWindowLong(this->winId, GWL_STYLE);
    if (enable)
    {
        style |= WS_TABSTOP;
    }
    else
    {
        style &= ~WS_TABSTOP;
    }
    SetWindowLong(this->winId, GWL_STYLE, style);
}

void Window::SetBorderless(bool enable)
{
    this->isBorderless = enable;
    SetWindowPos(this->winId, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

String Window::GetAccentColor() const
{
    return this->accentColor;
}

Splash &Window::GetSplash()
{
    return this->splash;
}

void Window::ExecuteScript(String script)
{
    if (this->view)
    {
        this->view->ExecuteScript(utf8ToUtf16(script).c_str(), nullptr);
    }
}

void Window::SetOnCloseCallback(std::function<bool()> callback)
{
    onCloseCallback = callback;
}

std::function<bool()> &Window::GetOnCloseCallback()
{
    return onCloseCallback;
}

void Window::SetAlwaysOnTop(bool enable)
{
    SetWindowPos(this->winId, enable ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void Window::SetMinSize(Size size)
{
    minSize = size;
}

void Window::SetResizable(bool enable)
{
    LONG style = GetWindowLong(this->winId, GWL_STYLE);
    if (enable)
    {
        style |= WS_THICKFRAME;
    }
    else
    {
        style &= ~WS_THICKFRAME;
    }
    SetWindowLong(this->winId, GWL_STYLE, style);
}

Size Window::GetMinSize() const
{
    return minSize;
}
} // namespace ezi
