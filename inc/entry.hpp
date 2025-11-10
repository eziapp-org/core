#pragma once
#include "platform.hpp"

int entry(int argc, char* argv[]);
#if OS(WINDOWS)
    #include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return entry(0, nullptr);
}
#else
int main(int argc, char* argv[])
{
    return entry(argc, argv);
}
#endif