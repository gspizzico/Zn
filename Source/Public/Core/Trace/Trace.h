#pragma once

#include <tracy/Tracy.hpp>

// Header file that defines macros for using Tracy.

namespace Zn::Trace
{
	// Define a static trace stat.
	struct TraceDefinition
	{
		const char* Name;
		uint32_t rgb;

		constexpr TraceDefinition(const char* name, uint8_t&& ir, uint8_t&& ig, uint8_t ib) :
			Name(name), rgb(ir << 8 | ig << 4 | ib)
		{}

		constexpr TraceDefinition(const char* name, const uint32_t inrgb) :
			Name(name), rgb(inrgb)
		{}
	};

	// constexpr function to determine the size of a const string.
	template<size_t N>
	constexpr size_t TStringLength(char const(&)[N])
	{
		return N - 1;
	}

	// Some colours..
	namespace Colors
	{
		constexpr uint32_t Red = 0x8B000;
		constexpr uint32_t Orange = 0xFFA500;
		constexpr uint32_t Khaki = 0xF0E68C;
		constexpr uint32_t Green = 0x9ACD32;
		constexpr uint32_t Blue = 0x4682B4;
		constexpr uint32_t Purple = 0x6A5ACD;
	}
}

#define ZN_TRACE_ENABLED (TRACY_ENABLE)

#if ZN_TRACE_ENABLED

///// SCOPED TRACE /////

// Defines a trace named trace stat with an rgb colour.
#define ZN_DEFINE_TRACE_RGB(name, r,g,b)\
namespace Zn::Trace::Definitions{\
/*dllexport for external?*/constexpr Zn::Trace::TraceDefinition T_##name(#name, r,g,b);\
}

// Defines a trace named trace stat with an rgb colour (hex).
#define ZN_DEFINE_TRACE(name, hex_color)\
namespace Zn::Trace::Definitions{\
/*dllexport for external?*/constexpr Zn::Trace::TraceDefinition T_##name(#name, hex_color);\
}

// Defines a scoped zone with the current function's name.
#define ZN_TRACE_QUICKSCOPE() ZoneScoped
// Defines a scoped zone from a previously defined trace stat
#define ZN_TRACE_SCOPE(name) ZoneScopedNC(Zn::Trace::Definitions::T_##name.Name,Zn::Trace::Definitions::T_##name.rgb)

///// MEMORY /////

// Trace allocations
#define ZN_MEMTRACE_ALLOC(ptr, size) TracyAlloc(ptr, size)
// Trace deallocations
#define ZN_MEMTRACE_FREE(size) TracyFree(size)

///// APPLICATION INFO /////

// Adds info to current capture
#define ZN_TRACE_INFO(info) TracyAppInfo(info, Zn::Trace::TStringLength(info))

///// FRAME /////
#define ZN_END_FRAME() FrameMark

#else
#define ZN_DEFINE_TRACE_RGB(name, r,g,b)
#define ZN_DEFINE_TRACE(name, hex_color)
#define ZN_TRACE_QUICKSCOPE()
#define ZN_TRACE_SCOPE(name)
#define ZN_MEMTRACE_ALLOC(ptr, size)
#define ZN_MEMTRACE_FREE(size)
#define ZN_TRACE_INFO(info)
#define ZN_END_FRAME()
#endif

