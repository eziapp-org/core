#pragma once
#include "application.hpp"
#include <vector>

namespace ezi
{
#if OS(WINDOWS)
    typedef HWND WinId;
#endif
    enum class MessageType
    {
        Info,
        Warning,
        Error,
        None
    };

    struct DialogButton
    {
        uint8_t id;
        String  text;
    };

    typedef std::vector<DialogButton> DialogButtons;

    enum class PermissionResult
    {
        AlwaysAllow,
        AllowOnce,
        Deny
    };

    enum class BeforeCloseResult
    {
        Close,
        Extra,
        Cancel
    };

    struct BeforeCloseArgs
    {
        String message;
        String extraButton;
    };

    class Dialog
    {
    private:
        WinId  winId;
        String appName;

    public:
        Dialog(WinId winId, String appName);

    public:
        void Alert(String message);
        bool Confirm(String message);

        BeforeCloseResult BeforeCloseRequest(BeforeCloseArgs args);
        PermissionResult  PermissionRequest(String permissionName);

        int Message(String title,
            String         message,
            MessageType    type          = MessageType::None,
            DialogButtons  buttons       = {},
            int            defaultButton = -1);
    };

}