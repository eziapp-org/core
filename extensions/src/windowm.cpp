#include "windowm.hpp"
#include "extensions.hpp"
#include "bridge.hpp"
#include "application.hpp"
#include "window.hpp"
#include "dialog.hpp"

namespace ezi
{
    namespace Private
    {
        static Window& GetWindowById(const String& id)
        {
            WinId winId = reinterpret_cast<WinId>(std::stoull(id));
            return Application::GetInstance().GetWindowById(winId);
        }
    }

    // 窗口管理器
    namespace windowm
    {
        Object getWindowList(Object args)
        {
            Array       result;
            WindowList& windows = Application::GetInstance().GetWindowList();
            for(size_t i = 0; i < windows.size(); i++)
            {
                Window* window = windows[i];
                result.push_back({
                    { "id", std::to_string(reinterpret_cast<uintptr_t>(window->GetWinId())) },
                    { "title", window->GetTitle() },
                });
            }
            return result;
        }

        Object createWindow(Object args)
        {
            Object result;
            auto&  window = Application::GetInstance().CrtWindowByOption(args["options"]);

            result["id"]    = std::to_string(reinterpret_cast<uintptr_t>(window.GetWinId()));
            result["title"] = window.GetTitle();

            return result;
        }

        Object getCurrentWindow(Object args)
        {
            Object result;
            auto   window   = Private::GetWindowById(args["senderWinId"]);
            result["id"]    = std::to_string(reinterpret_cast<uintptr_t>(window.GetWinId()));
            result["title"] = window.GetTitle();
            return result;
        }

    }

    // 窗口的对应操作
    namespace windowm
    {
        Object show(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Show();
            return "success";
        }

        Object maximize(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Maximize();
            return "success";
        }

        Object minimize(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Minimize();
            return "success";
        }

        Object restore(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Restore();
            return "success";
        }

        Object close(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Close();
            return "success";
        }

        Object reload(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Reload();
            return "success";
        }

        Object focus(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Focus();
            return "success";
        }

        Object blur(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Blur();
            return "success";
        }

        Object hide(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Hide();
            return "success";
        }

        Object drag(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            window.Drag();
            return "success";
        }

        Object isMaximizable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsMaximizable();
        }

        Object isMaximized(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsMaximized();
        }

        Object isMinimizable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsMinimizable();
        }

        Object isMinimized(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsMinimized();
        }

        Object isMovable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsMovable();
        }

        Object isClosed(Object args)
        {
            try
            {
                Private::GetWindowById(args["winId"]);
                return false;
            }
            catch(...)
            {
                return true;
            }
        }

        Object isFocusable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsFocusable();
        }

        Object isFocused(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsFocused();
        }

        Object isVisible(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsVisible();
        }

        Object isBorderless(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            return window.IsBorderless();
        }

        Object getBackgroundMode(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            switch(window.GetBackgroundMode())
            {
            case BackgroundMode::opaque:
                return "opaque";
            case BackgroundMode::transparent:
                return "transparent";
            case BackgroundMode::mica:
                return "mica";
            case BackgroundMode::acrylic:
                return "acrylic";
            default:
                return "opaque";
            }
        }

        Object getSize(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            Size  size   = window.GetSize();
            return Object {
                { "width", size.width },
                { "height", size.height },
            };
        }

        Object getPosition(Object args)
        {
            auto&    window = Private::GetWindowById(args["winId"]);
            Position pos    = window.GetPosition();
            return Object {
                { "x", pos.x },
                { "y", pos.y },
            };
        }

        Object setTitle(Object args)
        {
            auto&  window = Private::GetWindowById(args["winId"]);
            String title  = args["title"];
            window.SetTitle(title);
            return "success";
        }

        Object setBackgroundMode(Object args)
        {
            auto&          window = Private::GetWindowById(args["winId"]);
            BackgroundMode mode;
            String         modeStr = args["mode"];
            if(modeStr == "opaque")
            {
                mode = BackgroundMode::opaque;
            }
            else if(modeStr == "transparent")
            {
                mode = BackgroundMode::transparent;
            }
            else if(modeStr == "mica")
            {
                mode = BackgroundMode::mica;
            }
            else if(modeStr == "acrylic")
            {
                mode = BackgroundMode::acrylic;
            }
            else
            {
                mode = BackgroundMode::opaque;
            }
            window.SetBackgroundMode(mode);
            return "success";
        }

        Object setSize(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            Size  size;
            size.width  = args["width"];
            size.height = args["height"];
            window.SetSize(size);
            return "success";
        }

        Object setPosition(Object args)
        {
            auto&    window = Private::GetWindowById(args["winId"]);
            Position pos;
            pos.x = args["x"];
            pos.y = args["y"];
            window.SetPosition(pos);
            return "success";
        }

        Object setMaximizable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            bool  enable = args["enable"];
            window.SetMaximizable(enable);
            return "success";
        }

        Object setMinimizable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            bool  enable = args["enable"];
            window.SetMinimizable(enable);
            return "success";
        }

        Object setMovable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            bool  enable = args["enable"];
            window.SetMovable(enable);
            return "success";
        }

        Object setFocusable(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            bool  enable = args["enable"];
            window.SetFocusable(enable);
            return "success";
        }

        Object setBorderless(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);
            bool  enable = args["enable"];
            window.SetBorderless(enable);
            return "success";
        }

        Object setBeforeCloseMessage(Object args)
        {
            auto& window = Private::GetWindowById(args["winId"]);

            if(!args.contains("callbackName") || !args.contains("options"))
            {
                window.SetOnCloseCallback(nullptr);
                return "unkonwn arguments";
            }
            String callbackName = args["callbackName"];
            Object options      = args["options"];
            String content      = options["content"];
            String extraButton  = options["extraButton"];

            auto& feedbackWindow = Private::GetWindowById(args["senderWinId"]);

            window.SetOnCloseCallback(
                [callbackName, content, extraButton, &window, &feedbackWindow]()
                {
                    auto            dialog = Dialog(window.GetWinId(), window.GetTitle());
                    BeforeCloseArgs args { content, extraButton };
                    auto            result = dialog.BeforeCloseRequest(args);

                    String resultStr;
                    switch(result)
                    {
                    case BeforeCloseResult::Close:
                        resultStr = "close";
                        break;
                    case BeforeCloseResult::Extra:
                        resultStr = "extra";
                        break;
                    case BeforeCloseResult::Cancel:
                        resultStr = "cancel";
                        break;
                    }

                    // clang-format off
                    String script =
                    "window."+callbackName+"&&"
                    "window."+callbackName+"('"+resultStr+"');";
                    // clang-format on

                    feedbackWindow.ExecuteScript(script);

                    if(result == BeforeCloseResult::Close)
                    {
                        return true;
                    }
                    return false;
                });

            return "success";
        }
    }

    namespace windowm
    {
        void Mount()
        {
            REG(windowm, getWindowList);
            REG(windowm, createWindow);
            REG(windowm, getCurrentWindow);

            REG(windowm, show);
            REG(windowm, maximize);
            REG(windowm, minimize);
            REG(windowm, restore);
            REG(windowm, close);
            REG(windowm, reload);
            REG(windowm, focus);
            REG(windowm, blur);
            REG(windowm, hide);
            REG(windowm, drag);

            REG(windowm, isMaximizable);
            REG(windowm, isMaximized);
            REG(windowm, isMinimizable);
            REG(windowm, isMinimized);
            REG(windowm, isMovable);
            REG(windowm, isClosed);
            REG(windowm, isFocusable);
            REG(windowm, isFocused);
            REG(windowm, isVisible);
            REG(windowm, isBorderless);

            REG(windowm, getBackgroundMode);
            REG(windowm, getSize);
            REG(windowm, getPosition);

            REG(windowm, setTitle);
            REG(windowm, setBackgroundMode);
            REG(windowm, setSize);
            REG(windowm, setPosition);
            REG(windowm, setMaximizable);
            REG(windowm, setMinimizable);
            REG(windowm, setMovable);
            REG(windowm, setFocusable);
            REG(windowm, setBorderless);
            REG(windowm, setBeforeCloseMessage);
        }
    }
}