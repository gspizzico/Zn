#pragma once
#include "Core/Log/Log.h"

#define ZN_LOG(LogCategory, Verbosity, Format, ...)\
Zn::Log::LogMsg(#LogCategory, Verbosity, Format, __VA_ARGS__);

