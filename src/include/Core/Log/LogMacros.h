#pragma once
#include "Core/Build.h"
#include "Core/Log/Log.h"

#define CONCAT(A, B)            A##B

#define CATEGORY_VAR_NAME(Name) CONCAT(Name, Category)

#define GET_CATEGORY(Name)      Zn::LogCategories::CATEGORY_VAR_NAME(Name)

// Log declarations/definitions macros.
#define DECLARE_LOG_CATEGORY(Name)                                                                                                         \
    namespace Zn::LogCategories                                                                                                            \
    {                                                                                                                                      \
    extern Zn::AutoLogCategory CATEGORY_VAR_NAME(Name);                                                                                    \
    }

#define DEFINE_LOG_CATEGORY(Name, Verbosity) Zn::AutoLogCategory Zn::LogCategories::CATEGORY_VAR_NAME(Name) {#Name, Verbosity};

#define DEFINE_STATIC_LOG_CATEGORY(Name, Verbosity)                                                                                        \
    namespace Zn::LogCategories                                                                                                            \
    {                                                                                                                                      \
    static Zn::AutoLogCategory CATEGORY_VAR_NAME(Name) {#Name, Verbosity};                                                                 \
    }

#if ZN_LOGGING
    #define FASTER_LOGGING 1
    #if FASTER_LOGGING
        #define ZN_LOG(LogCategory, Verbosity, Format, ...)                                                                                \
            {                                                                                                                              \
                if (GET_CATEGORY(LogCategory).Category().IsSuppressed(Verbosity) == false)                                                 \
                    Zn::Log::LogMsg(GET_CATEGORY(LogCategory).Category().name, Verbosity, Format, __VA_ARGS__);                            \
            }
    #else
        #define ZN_LOG(LogCategory, Verbosity, Format, ...) Zn::Log::LogMsg(#LogCategory, Verbosity, Format, __VA_ARGS__);
    #endif
#else
    #define ZN_LOG(...)
#endif
