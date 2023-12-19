#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
struct AppEvents
{
    static inline TDelegate<void(uint32, uint32)> OnWindowSizeChanged;
    static inline TDelegate<void()>               OnWindowMinimized;
    static inline TDelegate<void()>               OnWindowRestored;
};
} // namespace Zn
