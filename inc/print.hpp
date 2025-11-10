#pragma once
#include "platform.hpp"
#include <iostream>

#if BUILDTYPE(DEBUG)
    #include <string>
    #include <sstream>

    #if OS(WINDOWS)
        #include <windows.h>
    #endif

class Printer
{
private:
    std::ostringstream oss;

public:
    Printer() { oss << "[debug] "; }
    ~Printer()
    {
        std::string str = oss.str();
    #if OS(WINDOWS)
        OutputDebugString(str.c_str());
    #else
        std::cout << str;
    #endif
    }
    template <typename T> Printer& operator<<(const T& value)
    {
        oss << value;
        return *this;
    }
    void                                         out() { }
    template <typename T, typename... Args> void out(const T& first, const Args&... rest)
    {
        oss << first;
        if(sizeof...(rest) > 0)
            oss << " ";
        out(rest...);
    }
};
#endif

#if BUILDTYPE(DEBUG)
    #define print(...) Printer().out(__VA_ARGS__)
    #define println(...) Printer().out(__VA_ARGS__, "\n")
#else
    #define print(...)
    #define println(...)
#endif
