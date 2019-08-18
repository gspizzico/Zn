#pragma once
#include "Core/Build.h"
#include "Core/Log/Log.h"

#if ZN_LOGGING
#define FASTER_LOGGING 1
#if FASTER_LOGGING 
#define ZN_LOG(LogCategory, Verbosity, Format, ...)\
{\
	static Name NAME_##LogCategory = Name(#LogCategory);\
	Zn::Log::LogMsg(NAME_##LogCategory, Verbosity, Format, __VA_ARGS__);\
}
#else
	#define ZN_LOG(LogCategory, Verbosity, Format, ...)\
	Zn::Log::LogMsg(#LogCategory, Verbosity, Format, __VA_ARGS__);
#endif
#else
#define ZN_LOG(...)
#endif

