#pragma once
#include "json.hpp"
#include "print.hpp"

#define REG(space, func)                                                                                               \
    ezi::Bridge::GetInstance().Register(#space "." #func, func);                                                       \
    println("Registered function:", #space "." #func)
