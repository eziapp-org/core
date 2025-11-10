#pragma once

// 区分不同的操作系统
#define OS_UNKNOWN 0
#define WINDOWS 1
#define LINUX 2
#define HARMONY 3
#define OPENHARMONY 4
#define MACOS 5
#define ANDROID 6
#define IOS 7

#define EZI_PLATFORM OS_UNKNOWN

#ifdef _WIN32
    #undef EZI_PLATFORM
    #define EZI_PLATFORM WINDOWS
#elif __linux__
    #undef EZI_PLATFORM
    #define EZI_PLATFORM LINUX
#elif __APPLE__
    #undef EZI_PLATFORM
    #define EZI_PLATFORM MACOS
#endif

#define OS(platform) (EZI_PLATFORM == platform)

// 区分编译器
#define COMPILER_UNKNOWN 0
#define GCC 1
#define MSVC 2
#define CLANG 3

#define EZI_COMPILER COMPILER_UNKNOWN

#ifdef __GNUC__
    #undef EZI_COMPILER
    #define EZI_COMPILER GCC
#elif _MSC_VER
    #undef EZI_COMPILER
    #define EZI_COMPILER MSVC
#elif __clang__
    #undef EZI_COMPILER
    #define EZI_COMPILER CLANG
#endif

#define COMPILER(compiler) (EZI_COMPILER == compiler)

// 区分构建类型
#define BUILDTYPE_UNKNOWN 0
#define DEBUG 1
#define RELEASE 2
#define EZI_BUILDTYPE BUILDTYPE_UNKNOWN

#ifdef _DEBUG
    #undef EZI_BUILDTYPE
    #define EZI_BUILDTYPE DEBUG
#else
    #undef EZI_BUILDTYPE
    #define EZI_BUILDTYPE RELEASE
#endif
#define BUILDTYPE(type) (EZI_BUILDTYPE == type)

// Windows的字符转换 gbk与unicode utf16 互转
#if OS(WINDOWS)
    #include <windows.h>
    #include <string>

inline std::string utf16ToGbk(const std::wstring& utf16_str)
{
    int len = WideCharToMultiByte(936, 0, utf16_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if(len <= 0)
        return "";

    std::string gbk(len - 1, '\0');
    WideCharToMultiByte(936, 0, utf16_str.c_str(), -1, &gbk[0], len, nullptr, nullptr);
    return gbk;
}

inline std::string utf16ToUtf8(const std::wstring& utf16_str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if(len <= 0)
        return "";

    std::string utf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}

inline std::wstring utf8ToUtf16(const std::string& utf8_str)
{
    if(utf8_str.empty())
        return L"";

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if(wide_len <= 0)
        return L"";

    std::wstring wide_str(wide_len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_len);
    return wide_str;
}

inline std::string utf8ToGbk(const std::string& utf8_str)
{
    if(utf8_str.empty())
        return "";

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if(wide_len <= 0)
        return "";

    std::wstring wide_str(wide_len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_len);

    return utf16ToGbk(wide_str);
}

#endif