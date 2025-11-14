#include "dialog.hpp"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

namespace ezi
{
    Dialog::Dialog(WinId winId, String appName)
    {
        this->winId   = winId;
        this->appName = appName;
    }

    void Dialog::Alert(String message)
    {
        Message(appName + " 提示", message);
    }

    bool Dialog::Confirm(String message)
    {
        DialogButtons buttons { { true, "是" }, { false, "否" } };
        return Message(appName + " 提示", message, MessageType::None, buttons, false);
    }

    BeforeCloseResult Dialog::BeforeCloseRequest(BeforeCloseArgs args)
    {
        DialogButtons buttons;
        buttons.push_back({ (uint8_t) BeforeCloseResult::Close, "关闭" });
        if(!args.extraButton.empty())
            buttons.push_back({ (uint8_t) BeforeCloseResult::Extra, args.extraButton });
        buttons.push_back({ (uint8_t) BeforeCloseResult::Cancel, "取消" });

        return (BeforeCloseResult) Message(
            appName + " 关闭窗口确认", args.message, MessageType::Warning, buttons, (int) BeforeCloseResult::Cancel);
    }

    PermissionResult Dialog::PermissionRequest(String permissionName)
    {
        DialogButtons buttons { { (uint8_t) PermissionResult::AlwaysAllow, "本次运行允许" },
            { (uint8_t) PermissionResult::AllowOnce, "允许一次" },
            { (uint8_t) PermissionResult::Deny, "拒绝" } };

        return (PermissionResult) Message(appName + " 请求权限",
            appName + " 想要 " + permissionName,
            MessageType::Warning,
            buttons,
            (int) PermissionResult::AllowOnce);
    }

    int Dialog::Message(String title, String message, MessageType type, DialogButtons buttons, int defaultButton)
    {
        std::wstring wTitle   = utf8ToUtf16(title);
        std::wstring wMessage = utf8ToUtf16(message);

        std::unique_ptr<TASKDIALOG_BUTTON[]> pButtons;

        std::vector<std::wstring> wButtonTexts;
        wButtonTexts.resize(buttons.size());

        int buttonCount = buttons.size();

        if(buttonCount == 0)
        {
            buttonCount               = 1;
            pButtons                  = std::make_unique<TASKDIALOG_BUTTON[]>(1);
            pButtons[0].nButtonID     = IDOK;
            pButtons[0].pszButtonText = L"确定";
            defaultButton             = IDOK;
        }
        else
        {
            pButtons = std::make_unique<TASKDIALOG_BUTTON[]>(buttonCount);
            for(int i = 0; i < buttonCount; ++i)
            {
                wButtonTexts[i]           = utf8ToUtf16(buttons[i].text);
                pButtons[i].nButtonID     = buttons[i].id;
                pButtons[i].pszButtonText = wButtonTexts[i].c_str();
            }
        }

        TASKDIALOGCONFIG _config = { 0 };
        _config.hwndParent       = this->winId;
        _config.cbSize           = sizeof(TASKDIALOGCONFIG);
        _config.dwFlags          = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
        switch(type)
        {
        case MessageType::Info:
            _config.pszMainIcon = TD_INFORMATION_ICON;
            break;
        case MessageType::Warning:
            _config.pszMainIcon = TD_WARNING_ICON;
            break;
        case MessageType::Error:
            _config.pszMainIcon = TD_ERROR_ICON;
            break;
        default:
            _config.pszMainIcon = nullptr;
            break;
        }
        _config.pszWindowTitle = wTitle.c_str();
        _config.pszContent     = wMessage.c_str();
        _config.cButtons       = buttonCount;
        _config.pButtons       = pButtons.get();
        if(defaultButton >= 0)
        {
            _config.nDefaultButton = defaultButton;
        }

        int nButtonPressed = 0;

        HRESULT hr = TaskDialogIndirect(&_config, &nButtonPressed, nullptr, nullptr);
        if(FAILED(hr))
        {
            return defaultButton;
        }
        return nButtonPressed;
    }
}