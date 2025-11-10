#pragma once
#include "platform.hpp"

namespace ezi
{
#if BUILDTYPE(DEBUG)
    namespace terminal
    {
        void Mount();
    }
#endif
}