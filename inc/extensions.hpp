#pragma once
#include "print.hpp"
#include "json.hpp"

#define REG(space, func)                                                                                               \
    ezi::Bridge::GetInstance().Register(#space "." #func, func);                                                       \
    println("Registered function:", #space "." #func)
