#pragma once
#include "Core/Build.h"
#include "Core/Log/Log.h"

#if ZN_LOGGING
#define ZN_LOG(LogCategory, Verbosity, Format, ...)\
Zn::Log::LogMsg(#LogCategory, Verbosity, Format, __VA_ARGS__);
#else
#define ZN_LOG(...)
#endif

