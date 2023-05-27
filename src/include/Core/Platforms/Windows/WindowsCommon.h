#pragma once

#include <windows.h>

// Windows - Avoid conflicts with numeric limit min/max
#if defined(_MSC_VER)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif
