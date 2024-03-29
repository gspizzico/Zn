#pragma once

#include <Core/HAL/PlatformTypes.h>

#if ZN_DEBUG
    #define check(condition)                                                                                                               \
        if (!(condition))                                                                                                                  \
        {                                                                                                                                  \
            char buffer[512];                                                                                                              \
            std::snprintf(buffer, sizeof(buffer), "Assertion failed! %s line %d\n`" #condition "`\n", __FILE__, __LINE__);                 \
            PlatformMisc::DebugMessage(&buffer[0]);                                                                                        \
            __debugbreak();                                                                                                                \
            std::exit(-1);                                                                                                                 \
        }
    #define checkMsg(condition, message, ...)                                                                                              \
        if (!(condition))                                                                                                                  \
        {                                                                                                                                  \
            char buffer[512];                                                                                                              \
            std::snprintf(                                                                                                                 \
                buffer, sizeof(buffer), "Assertion failed! %s line %d\n`" #condition "` " message "\n", __FILE__, __LINE__, __VA_ARGS__);  \
            PlatformMisc::DebugMessage(&buffer[0]);                                                                                        \
            __debugbreak();                                                                                                                \
            std::exit(-1);                                                                                                                 \
        }

#else
    #define check(condition)
    #define checkMsg(condition, message, ...)
#endif
