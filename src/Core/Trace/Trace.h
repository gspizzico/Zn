#pragma once

#ifndef TRACY_ENABLE
    #define TRACY_ENABLE 0
#endif

#define TRACY_CALLSTACK 12
#include <Core/Types.h>

// #include <tracy/Tracy.hpp>
// #include <vulkan/vulkan.h>
// #include <tracy/TracyVulkan.hpp>

// Header file that defines macros for using Tracy.

namespace Zn::Trace
{
// Define a static trace stat.
struct TraceDefinition
{
    const char* Name;
    uint32      rgb;

    constexpr TraceDefinition(const char* name, uint8&& ir, uint8&& ig, uint8 ib)
        : Name(name)
        , rgb(ir << 8 | ig << 4 | ib)
    {
    }

    constexpr TraceDefinition(const char* name, const uint32 inrgb)
        : Name(name)
        , rgb(inrgb)
    {
    }
};

// constexpr function to determine the size of a const string.
template<size_t N>
constexpr size_t TStringLength(char const (&)[N])
{
    return N - 1;
}

// Some colours..
namespace Colors
{
constexpr uint32 Red    = 0x8B000;
constexpr uint32 Orange = 0xFFA500;
constexpr uint32 Khaki  = 0xF0E68C;
constexpr uint32 Green  = 0x9ACD32;
constexpr uint32 Blue   = 0x4682B4;
constexpr uint32 Purple = 0x6A5ACD;
} // namespace Colors
} // namespace Zn::Trace

#define ZN_TRACE_ENABLED (TRACY_ENABLE)

// using GPUTraceContextPtr = TracyVkCtx;

#if ZN_TRACE_ENABLED

    ///// SCOPED TRACE /////

    // Defines a trace named trace stat with an rgb colour.
    #define ZN_DEFINE_TRACE_RGB(name, r, g, b)                                                                                             \
        namespace Zn::Trace::Definitions                                                                                                   \
        {                                                                                                                                  \
        /*dllexport for external?*/ constexpr Zn::Trace::TraceDefinition T_##name(#name, r, g, b);                                         \
        }

    // Defines a trace named trace stat with an rgb colour (hex).
    #define ZN_DEFINE_TRACE(name, hex_color)                                                                                               \
        namespace Zn::Trace::Definitions                                                                                                   \
        {                                                                                                                                  \
        /*dllexport for external?*/ constexpr Zn::Trace::TraceDefinition T_##name(#name, hex_color);                                       \
        }

    // Defines a scoped zone with the current function's name.
    #define ZN_TRACE_QUICKSCOPE()        ZoneScoped
// Defines a scoped zone from a previously defined trace stat
    #define ZN_TRACE_SCOPE(name)         ZoneScopedNC(Zn::Trace::Definitions::T_##name.Name, Zn::Trace::Definitions::T_##name.rgb)

    ///// MEMORY /////

    // Trace allocations
    #define ZN_MEMTRACE_ALLOC(ptr, size) TracyAlloc(ptr, size)
// Trace deallocations
    #define ZN_MEMTRACE_FREE(size)       TracyFree(size)

    ///// APPLICATION INFO /////

    // Adds info to current capture
    #define ZN_TRACE_INFO(info)          TracyAppInfo(info, Zn::Trace::TStringLength(info))

    ///// FRAME /////
    #define ZN_END_FRAME()               FrameMark

///// GPU ////

template<typename Gpu, typename Device, typename Queue, typename CommandBuffer>
__forceinline GPUTraceContextPtr ZN_TRACE_GPU_CONTEXT_CREATE(cstring name, Gpu gpu, Device device, Queue queue, CommandBuffer commandBuffer)
{
    GPUTraceContextPtr context = TracyVkContext(gpu, device, queue, commandBuffer);
    TracyVkContextName(context, name, strlen(name));
    return context;
}

__forceinline void ZN_TRACE_GPU_CONTEXT_DESTROY(GPUTraceContextPtr context)
{
    TracyVkDestroy(context);
}

    #define ZN_TRACE_GPU_SCOPE(name, context, commandBuffer) TracyVkZone(context, commandBuffer, name)
    #define ZN_TRACE_GPU_COLLECT(context, commandBuffer)     TracyVkCollect(context, commandBuffer)

#else
    #define ZN_DEFINE_TRACE_RGB(name, r, g, b)
    #define ZN_DEFINE_TRACE(name, hex_color)
    #define ZN_TRACE_QUICKSCOPE()
    #define ZN_TRACE_SCOPE(name)
    #define ZN_MEMTRACE_ALLOC(ptr, size)
    #define ZN_MEMTRACE_FREE(size)
    #define ZN_TRACE_INFO(info)
    #define ZN_END_FRAME()
    #define ZN_TRACE_GPU_SCOPE(...)
    #define ZN_TRACE_GPU_COLLECT(...)

template<typename Gpu, typename Device, typename Queue, typename CommandBuffer>
void* ZN_TRACE_GPU_CONTEXT_CREATE(cstring name, Gpu gpu, Device device, Queue queue, CommandBuffer commandBuffer)
{
    return nullptr;
}

// static void ZN_TRACE_GPU_CONTEXT_DESTROY(GPUTraceContextPtr context)
//{
// }
#endif
