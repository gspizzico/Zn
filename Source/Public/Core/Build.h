#pragma once

#ifndef ZN_DEBUG
    #ifndef ZN_RELEASE
    #error "Build macros have not been defined!"
    #endif
#endif

#define ZN_LOGGING (ZN_DEBUG)