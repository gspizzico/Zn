#pragma once

#include <Core/CorePlatform.h>

#if ZN_DEBUG
#define check(condition)                                                                                                                   \
    if (!(condition))                                                                                                                      \
    {                                                                                                                                      \
        char buffer[512];                                                                                                                  \
        std::snprintf(buffer, sizeof(buffer), "Assertion failed! %s line %d\n`" #condition "`\n", __FILE__, __LINE__);                     \
        Zn::PlatformMisc::LogDebug(&buffer[0]);                                                                                            \
        __debugbreak();                                                                                                                    \
    }
#define checkMsg(condition, message, ...)                                                                                                  \
    if (!(condition))                                                                                                                      \
    {                                                                                                                                      \
        char buffer[512];                                                                                                                  \
        std::snprintf(                                                                                                                     \
            buffer, sizeof(buffer), "Assertion failed! %s line %d\n`" #condition "` " message "\n", __FILE__, __LINE__, __VA_ARGS__);      \
        Zn::PlatformMisc::LogDebug(&buffer[0]);                                                                                            \
        __debugbreak();                                                                                                                    \
    }

#else
#define check(condition)
#define checkMsg(condition, message, ...)
#endif
