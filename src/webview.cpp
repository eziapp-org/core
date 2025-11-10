#include "window.hpp"
#include "webview.hpp"
#include "print.hpp"
#include "application.hpp"

#if OS(WINDOWS)
    #include <shlobj.h>
    #include <shlwapi.h>
    #include <windows.h>
    #include <WebView2.h>
    #include "WebView2EnvironmentOptions.h"
    #include <wrl.h>
    #pragma comment(lib, "shlwapi.lib")
using namespace Microsoft::WRL;
#endif

#include "resource.hpp"
#include "bridge.hpp"
#include "dialog.hpp"
#include "utils.hpp"
#include "ezienv.hpp"

namespace ezi
{
    std::wstring GetMimeType(const std::wstring& uri)
    {
        static const std::unordered_map<std::wstring, std::wstring> mimeMap = {
            { L".html", L"text/html" },
            { L".js", L"text/javascript" },
            { L".css", L"text/css" },
            { L".png", L"image/png" },
            { L".jpg", L"image/jpeg" },
            { L".jpeg", L"image/jpeg" },
            { L".svg", L"image/svg+xml" },
            { L".json", L"application/json" },
        };

        auto ext = PathFindExtensionW(uri.c_str());
        auto it  = mimeMap.find(ext);

        if(it != mimeMap.end())
        {
            return it->second;
        }

        return L"text/plain";
    }

#if OS(WINDOWS)
    void Webview::CreateEnv()
    {
        auto options = Make<CoreWebView2EnvironmentOptions>();
        options->put_AdditionalBrowserArguments(L"--disable-web-security");
        auto envHandler = [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
        {
            println("webview env created");
            this->env = env;
            return S_OK;
        };

        PWSTR appDataPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath);
        std::wstring EziAppDataFolder = std::wstring(appDataPath) + L"\\EziApps";
        CoTaskMemFree(appDataPath);

        auto hr = CreateCoreWebView2EnvironmentWithOptions(nullptr,
            EziAppDataFolder.c_str(),
            options.Get(),
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(envHandler).Get());
        if(FAILED(hr))
        {
            auto code = GetLastError();
            println("CreateCoreWebView2EnvironmentWithOptions failed,code:", code);
            std::string msg;
            switch(code)
            {
            case 298:
                msg = "运行环境需要Webview2组件，请下载并安装。";
                break;

            default:
                msg = "应用启动失败，错误码：" + std::to_string(code);
                break;
            }
            MessageBox(nullptr, utf8ToGbk(msg).c_str(), "错误", MB_OK | MB_ICONERROR);
            exit(1);
        }
    }
#endif

#if OS(WINDOWS)
    void Webview::CreateController(Window& window)
    {
        auto controllerHandler = [this, &window](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
        {
            wil::com_ptr<ICoreWebView2Controller2> controller2 = nullptr;
            if(SUCCEEDED(controller->QueryInterface(IID_PPV_ARGS(&controller2))))
            {
                COREWEBVIEW2_COLOR bgColor = { 0, 255, 255, 255 };
                controller2->put_DefaultBackgroundColor(bgColor);
            }

            View view;
            controller->get_CoreWebView2(&view);

            // 禁用默认快捷键
            controller->add_AcceleratorKeyPressed(
                Microsoft::WRL::Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                    [](ICoreWebView2Controller* sender, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT
                    {
                        UINT key;
                        args->get_VirtualKey(&key);
                        // 输入框需要
                        switch(key)
                        {
                        case 'C':
                        case 'V':
                        case 'X':
                        case 'A':
                        case 'Z':
                        case 'Y':
                        case VK_LEFT:
                        case VK_RIGHT:
                        case VK_UP:
                        case VK_DOWN:
                            break;
                        default:
                            args->put_Handled(TRUE);
                            break;
                        }
                        return S_OK;
                    })
                    .Get(),
                nullptr);

            static auto& app        = Application::GetInstance();
            static auto  appName    = CFGRES<String>("application.name", "Ezi App");
            static auto  appVersion = CFGRES<String>("application.version", "0.0.0");

            static wil::unique_cotaskmem_string webviewVersion;
            env->get_BrowserVersionString(&webviewVersion);

            // clang-format off
            static String eziVersion = EZI_VERSION;
            static String buildType  = (BUILDTYPE(DEBUG) ? "Debug" : "Release");
            static String buildDate  = __DATE__ " " __TIME__;

            static SystemVersion _osVersion = app.GetSystemVersion();
            static String osVersionStr = []{
                return std::to_string(_osVersion.major) + "." +
                       std::to_string(_osVersion.minor) + "." +
                       std::to_string(_osVersion.build);
            }();
            
            static String startupTransition = []{
                if(_osVersion.major >= 10 && _osVersion.minor >= 0 && _osVersion.build >= 22000)
                {
                    return "FallIn";
                }
                return "ScaleIn";
            }();

            String accentColor = window.GetAccentColor();
            if(accentColor == "system")
            {
                accentColor = Utils::ColorRefToHex(Utils::GetAccentColor());
            }

            String injectScript = 
            "window.Ezi={"
                "EziVersion:'" + eziVersion + "',"
                "BuildType:'" + buildType + "',"
                "BuildDate:'" + buildDate + "',"
                "Platform:'windows',"
                "OSVersion:'" + osVersionStr + "',"
                "EziGitHash:'" + GIT_HASH + "',"
                "WebViewVersion:'" + utf16ToUtf8(webviewVersion.get()) + "'"
            "};"

            "const eziStyle=document.createElement('style');"
            "eziStyle.innerHTML= `"
            "body{"
                "--ezi-accent-color: " + accentColor + ";"
                "-webkit-user-select: none;"
                "user-select: none;"
                "overflow: hidden;"
                "animation: " + startupTransition + " 0.5s cubic-bezier(0.00, 0.87, 0.00, 1.00),"
                "fadeIn 0.4s cubic-bezier(0.01, 0.46, 0.00, 0.81);"
            "}"
            "@keyframes fadeIn {from {opacity: 0;} to {opacity: 1;}}"
            "@keyframes ScaleIn {from {transform: scale(0.9);} to {transform: scale(1);}}"
            "@keyframes FallIn {from {transform: translateY(500px);} to {transform: translateY(0px);}}"
            "img,a{"
                "-webkit-user-drag: none;"
                "user-drag: none;"
            "}"
            "::-webkit-scrollbar{"
                "width: 8px;"
                "height: 8px;"
                "background-color: transparent;"
            "}"
            "::-webkit-scrollbar-corner{"
                "width: 0;"
                "height: 0;"
                "background-color: transparent;"
            "}"
            "::-webkit-scrollbar-thumb{"
                "background-color: rgba(0, 0, 0, 0.3);"
                "border-radius: 8px;"
                "border: 3px solid transparent;"
                "background-clip: content-box;"
                "cursor: grab;"
            "}"
            "::-webkit-scrollbar-thumb:hover{"
                "background-color: rgba(0, 0, 0, 0.4);"
                "border: 2px solid transparent;"
            "}"
            "@media (prefers-color-scheme: dark){"
                "::-webkit-scrollbar-thumb{"
                    "background-color: rgba(255, 255, 255, 0.2);"
                "}"
                "::-webkit-scrollbar-thumb:hover{"
                    "background-color: rgba(255, 255, 255, 0.4);"
                "}"
            "}`;"
            "function injectEziStyle(retry){"
                "retry = retry||0;"
                "if(!document.head && retry<100){"
                    "requestAnimationFrame(()=>injectEziStyle(retry));"
                "}else{"
                    "const href = location.href;"
                    "if(href.startsWith('http://localhost')||href.startsWith('http://127.0.0.1')){"
                        "window.addEventListener('load',()=>{"
                            "document.head.appendChild(eziStyle);"
                        "});"
                    "}else{"
                        "document.head.appendChild(eziStyle);"
                    "}"
                "}"
            "}"
            "injectEziStyle(0);";
            // clang-format on

            // 注入脚本
            view->AddScriptToExecuteOnDocumentCreated(utf8ToUtf16(injectScript).c_str(), nullptr);

            std::wstring filter
                = (L"https://" + utf8ToUtf16(CFGRES<String>("application.package", "com.ezi.app")) + L"/*");
            view->AddWebResourceRequestedFilter(filter.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

            // 资源请求拦截
            view->add_WebResourceRequested(
                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                    [this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT
                    {
                        wil::com_ptr<ICoreWebView2WebResourceRequest> request;
                        args->get_Request(&request);
                        wil::unique_cotaskmem_string uri;
                        request->get_Uri(&uri);
                        String url = utf16ToUtf8(uri.get());
                        println("WebResourceRequested", url);

                        auto assetsBinarys = Resource::GetInstance().GetAssetData(url);

                        ULONG written = 0;

                        wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                        if(assetsBinarys.size() > 0)
                        {
                            std::wstring          mime = GetMimeType(uri.get());
                            wil::com_ptr<IStream> stream
                                = SHCreateMemStream(assetsBinarys.data(), assetsBinarys.size());
                            this->env->CreateWebResourceResponse(
                                stream.get(), 200, L"OK", (L"Content-Type: " + mime).c_str(), &response);
                        }
                        else
                        {
                            this->env->CreateWebResourceResponse(nullptr, 404, L"Not Found", nullptr, &response);
                        }

                        args->put_Response(response.get());
                        return S_OK;
                    })
                    .Get(),
                nullptr);

            // 脚本弹窗处理
            view->add_ScriptDialogOpening(
                Callback<ICoreWebView2ScriptDialogOpeningEventHandler>(
                    [&window](ICoreWebView2* sender, ICoreWebView2ScriptDialogOpeningEventArgs* args) -> HRESULT
                    {
                        wil::unique_cotaskmem_string message;
                        args->get_Message(&message);

                        COREWEBVIEW2_SCRIPT_DIALOG_KIND kind;
                        args->get_Kind(&kind);

                        auto dialog = Dialog(window.GetWinId(), window.GetTitle());

                        switch(kind)
                        {
                        case COREWEBVIEW2_SCRIPT_DIALOG_KIND_ALERT:
                            dialog.Alert(utf16ToUtf8(message.get()));
                            break;
                        case COREWEBVIEW2_SCRIPT_DIALOG_KIND_CONFIRM:
                        {
                            bool result = dialog.Confirm(utf16ToUtf8(message.get()));
                            if(result)
                            {
                                args->Accept();
                            }
                            break;
                        }
                        case COREWEBVIEW2_SCRIPT_DIALOG_KIND_PROMPT:
                        {
                            wil::unique_cotaskmem_string defaultText;
                            args->get_DefaultText(&defaultText);
                            args->put_ResultText(defaultText.get());
                            args->Accept();
                            break;
                        }
                        case COREWEBVIEW2_SCRIPT_DIALOG_KIND_BEFOREUNLOAD:
                        {
                            // 在WndProc中处理
                            break;
                        }
                        default:
                            dialog.Alert(utf16ToUtf8(message.get()));
                            break;
                        }

                        return S_OK;
                    })
                    .Get(),
                nullptr);

            // 权限请求处理
            view->add_PermissionRequested(
                Callback<ICoreWebView2PermissionRequestedEventHandler>(
                    [](ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args) -> HRESULT
                    {
                        args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                        return S_OK;
                    })
                    .Get(),
                nullptr);

            // 导航结束处理
            view->add_NavigationCompleted(
                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                    [&window](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
                    {
                        if(window.GetStatus() == WindowStatus::Loading)
                            window.SetStatus(WindowStatus::Switching);
                        return S_OK;
                    })
                    .Get(),
                nullptr);

            // 打开新链接处理
            view->add_NewWindowRequested(
                Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                    [&window](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
                    {
                        wil::unique_cotaskmem_string uri;
                        args->get_Uri(&uri);

                        auto dialog = Dialog(window.GetWinId(), window.GetTitle());
                        // todo
                        if(dialog.PermissionRequest("使用默认浏览器打开链接 " + utf16ToUtf8(uri.get()))
                            != PermissionResult::Deny)
                        {
                            ShellExecuteW(nullptr, L"open", uri.get(), nullptr, nullptr, SW_SHOWNORMAL);
                        }
                        args->put_Handled(TRUE);
                        return S_OK;
                    })
                    .Get(),
                nullptr);

            // 浏览器窗口设置
            wil::com_ptr<ICoreWebView2Settings> settings;
            view->get_Settings(&settings);
    #if BUILDTYPE(DEBUG)
            settings->put_AreDevToolsEnabled(true);
            settings->put_AreDefaultContextMenusEnabled(true);
            settings->put_AreDefaultScriptDialogsEnabled(false);
            settings->put_IsStatusBarEnabled(false);
            settings->put_IsZoomControlEnabled(false);
    #else
            settings->put_AreDevToolsEnabled(false);
            settings->put_AreDefaultContextMenusEnabled(false);
            settings->put_AreDefaultScriptDialogsEnabled(false);
            settings->put_IsStatusBarEnabled(false);
            settings->put_IsZoomControlEnabled(false);
    #endif

            wil::com_ptr<ICoreWebView2Settings5> settings5;
            if(SUCCEEDED(settings->QueryInterface(IID_PPV_ARGS(&settings5))))
            {
                wil::unique_cotaskmem_string currentUA;
                settings5->get_UserAgent(&currentUA);
                std::wstring eziAppUA = std::wstring(currentUA.get()) + L" EziApp/"
                    + utf8ToUtf16(eziVersion + " " + appName + "/" + appVersion);
                settings5->put_UserAgent(eziAppUA.c_str());
            }

            // 清除数据
            static auto reseted = [&view]
            {
                if(!(EziEnv::GetInstance().IsNeedReset()))
                {
                    return false;
                }
                auto package = CFGRES<String>("application.package", "com.ezi.app");
                auto origin  = "https://" + package + "/";

                std::wstring payload = L"{\"origin\":\"" + utf8ToUtf16(origin) + L"\",\"storageTypes\":\"all\"}";
                view->CallDevToolsProtocolMethod(L"Storage.clearDataForOrigin",
                    payload.c_str(),
                    Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                        [](HRESULT error, LPCWSTR resultJson) -> HRESULT
                        {
                            println("Cleared webview data:", utf16ToUtf8(resultJson));
                            return S_OK;
                        })
                        .Get());
                return true;
            }();

            // 设置窗口内容
            RECT bounds;
            GetClientRect(window.GetWinId(), &bounds);
            controller->put_Bounds(bounds);
            window.SetController(controller);
            window.SetView(view);
            auto& bridge = Bridge::GetInstance();
            bridge.ExposeTo(view, window.GetWinId());
            view->Navigate(utf8ToUtf16(window.GetUrl().c_str()).c_str());
            return S_OK;
        };
        env->CreateCoreWebView2Controller(window.GetWinId(),
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(controllerHandler).Get());
    }
#endif

    Env Webview::GetEnv()
    {
        return this->env;
    }

    Webview& Webview::GetInstance()
    {
        static Webview instance;
        return instance;
    }
}