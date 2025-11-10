#include "bridge.hpp"
#include "print.hpp"
#include "windowm.hpp"
#include "terminal.hpp"
#include "tray.hpp"

#if OS(WINDOWS)
    #include <wrl.h>
using namespace Microsoft::WRL;
#endif

namespace ezi
{
    void Bridge::ExposeTo(View& view, WinId winId)
    {
        static bool mounted = false;
        if(!mounted)
        {
            mounted = true;
            windowm::Mount();
#if BUILDTYPE(DEBUG)
            terminal::Mount();
#endif
            tray::Mount();
        }

        view->add_WebMessageReceived(
            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this, winId](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                {
                    wil::unique_cotaskmem_string _message;
                    args->get_WebMessageAsJson(&_message);
                    std::wstring message(_message.get());

                    Json request                   = Json::parse(utf16ToUtf8(message));
                    request["args"]["senderWinId"] = std::to_string(reinterpret_cast<uintptr_t>(winId));
                    println("request:", utf8ToGbk(request.dump()));

                    Json result;
                    try
                    {
                        result = Call(request["func"], request["args"]);
                    }
                    catch(const std::exception& e)
                    {
                        result = { { "error", e.what() } };
                    }

                    Json response = {
                        { "id", request["id"] },
                        { "result", result },
                    };

                    auto utf8Response = response.dump();
                    println("response:", utf8ToGbk(utf8Response));

                    sender->PostWebMessageAsJson(utf8ToUtf16(utf8Response).c_str());
                    return S_OK;
                })
                .Get(),
            nullptr);
    }

    Json Bridge::Call(String func, Json args)
    {
        auto it = functions.find(func);
        Json result;
        if(it != functions.end() && it->second)
        {
            result = it->second(args);
        }
        else
        {
            throw std::runtime_error("Function not found: " + func);
        }

        return result;
    }

    Bridge& Bridge::GetInstance()
    {
        static Bridge instance;
        return instance;
    }
    void Bridge::Register(String name, Function func)
    {
        functions[name] = func;
    }
} // namespace ezi