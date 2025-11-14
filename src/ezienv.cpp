#include "ezienv.hpp"
#include "platform.hpp"
#include "resource.hpp"
#include "window.hpp"
#include <fstream>

#if OS(WINDOWS)
    #include <shlobj.h>
    #include <windows.h>
#endif
#include "dialog.hpp"
#include "print.hpp"

namespace ezi
{
    EziEnv::EziEnv()
    {
        // 从改软件的env文件中获取env所属程序的路径
        // 并对比其和自身是否是同一个程序
        // 如果是初次运行则创建该文件
        // 如果所属路径是其他程序并且存在，就提醒冲突
        // 如果不存在则提示清除数据并重新创建
        auto  config      = Resource::GetInstance().GetConfig();
        auto  package     = CFGRES<std::string>("application.package", "com.ezi.app");
        auto  appName     = CFGRES<std::string>("application.name", "EziApp");
        PWSTR appDataPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath);
        envFilePath = utf16ToUtf8(appDataPath) + "\\EziApps\\" + package + ".env";
        CoTaskMemFree(appDataPath);

        wchar_t programPathC[MAX_PATH];

        Dialog dialog(nullptr, appName);

        DWORD result = GetModuleFileNameW(NULL, programPathC, MAX_PATH);
        if(result == 0 || result == MAX_PATH)
        {
            dialog.Message("错误", "无法获取程序路径", MessageType::Error);
            exit(1);
        }

        auto programPath = utf16ToUtf8(std::wstring(programPathC));

        std::ifstream envFile(envFilePath);

        if(envFile.is_open())
        {
            try
            {
                std::vector<uint8_t> binary(
                    (std::istreambuf_iterator<char>(envFile)), std::istreambuf_iterator<char>());

                envData = Json::from_msgpack(binary);
            }
            catch(const Json::parse_error& e)
            {
                println("failed to parse env file:", envFilePath);
            }
        }

        if(envData.contains("ownerPath") == false)
        {
            // 首次运行，创建.env文件
            envData = Json {};

            envData["ownerPath"] = programPath;
            envData["appName"]   = appName;
            envData["package"]   = package;
            envData["version"]   = CFGRES<std::string>("application.version", "0.0.0");
            std::ofstream newEnvFile(envFilePath);
            newEnvFile << envData.dump();
            std::vector<uint8_t> binary = Json::to_msgpack(envData);
            newEnvFile.write(reinterpret_cast<const char*>(binary.data()), binary.size());
        }

        auto ownerPath    = envData.value("ownerPath", "");
        auto ownerAppName = envData.value("appName", "unkown");
        if(ownerPath != programPath)
        {
            std::ifstream ownerFile(ownerPath);
            if(ownerFile.is_open())
            {
                // 应用冲突且存在
                std::string msg
                    = "与其他应用程序冲突,想要继续运行此应用请先卸载\n应用程序：" + ownerAppName + "\n" + ownerPath;
                dialog.Message("包名冲突:" + package, msg, MessageType::Error);
                exit(1);
            }
            else
            {
                // 应用冲突但不存在
                DialogButtons buttons { { true, "清除数据并继续" }, { false, "退出应用" } };

                std::string msg
                    = "你最近似乎移动了此应用的位置,想要继续运行此应用,你必须先清除 " + appName + " 的数据。";

                bool result = dialog.Message("清除数据", msg, MessageType::Warning, buttons, false);
                if(!result)
                {
                    exit(0);
                }
                // 清除数据并重建.env文件
                isNeedReset          = true;
                envData["ownerPath"] = programPath;
                envData["appName"]   = appName;
                envData["package"]   = package;
                envData["version"]   = CFGRES<std::string>("application.version", "0.0.0");
                std::ofstream        newEnvFile(envFilePath);
                std::vector<uint8_t> binary = Json::to_msgpack(envData);
                newEnvFile.write(reinterpret_cast<const char*>(binary.data()), binary.size());
            }
        }
    }

    EziEnv& EziEnv::GetInstance()
    {
        static EziEnv instance;
        return instance;
    }

    void EziEnv::SaveVar(std::string key, Object value)
    {
        envData[key] = value;
        std::ofstream        envFile(envFilePath);
        std::vector<uint8_t> binary = Json::to_msgpack(envData);
        envFile.write(reinterpret_cast<const char*>(binary.data()), binary.size());
    }

    std::string EziEnv::GetVar(std::string key)
    {
        return envData.value(key, "");
    }

    bool EziEnv::IsNeedReset() const
    {
        return isNeedReset;
    }
    Position EziEnv::GetRememberedWindowPosition()
    {
        if(envData.contains("windowPosition"))
        {
            auto position = envData["windowPosition"];
            if(position.contains("x") && position.contains("y"))
            {
                return { position["x"].get<int>(), position["y"].get<int>() };
            }
        }
        throw std::runtime_error("Invalid windowPosition data");
    }
    void EziEnv::SetRememberedWindowPosition(const Position& pos)
    {
        Object position { { "x", pos.x }, { "y", pos.y } };
        SaveVar("windowPosition", position);
    }
}