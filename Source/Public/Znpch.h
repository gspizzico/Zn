#pragma once

#include <Core/Build.h>
#include <Core/HAL/BasicTypes.h>
#include <Core/Containers/Vector.h>
#include <Core/Log/Log.h>
#include <Core/Log/LogMacros.h>
#include <Core/Trace/Trace.h>

// Windows - Avoid conflicts with numeric limit min/max
#if defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif