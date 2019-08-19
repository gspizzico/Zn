#pragma once
#include "Core/Build.h"
#include "Core/Log/Log.h"

#if ZN_LOGGING
#define FASTER_LOGGING 1
#if FASTER_LOGGING 
#define ZN_LOG(LogCategory, Verbosity, Format, ...)\
{\
	if(GET_CATEGORY(LogCategory).Category().IsSuppressed(Verbosity) == false)\
		Zn::Log::LogMsg(GET_CATEGORY(LogCategory).Category().m_Name, Verbosity, Format, __VA_ARGS__);\
}
#else
	#define ZN_LOG(LogCategory, Verbosity, Format, ...)\
	Zn::Log::LogMsg(#LogCategory, Verbosity, Format, __VA_ARGS__);
#endif
#else
#define ZN_LOG(...)
#endif

