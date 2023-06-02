#pragma once

#include <CoreTypes.h>
#include <wyhash/wyhash.h>

namespace Zn
{
template<typename T>
inline u64 HashCalculate(const T& value_, sizet seed_ = 0)
{
    return wyhash(&value_, sizeof(T), seed_, _wyp);
}

template<size_t N>
inline u64 HashCalculate(const char (&value_)[N], sizet seed_ = 0)
{
    return wyhash(value_, strlen(value_), seed_, _wyp);
}

template<>
inline u64 HashCalculate(const cstring& value_, sizet seed_)
{
    return wyhash(value_, strlen(value_), seed_, _wyp);
}

template<>
inline u64 HashCalculate(const String& value_, sizet seed_)
{
    return HashCalculate(value_.c_str(), seed_);
};

inline u64 HashBytes(const void* data_, sizet length_, sizet seed_ = 0)
{
    return wyhash(data_, length_, seed_, _wyp);
}

inline u64 HashCombine(u64 first_, u64 second_)
{
    return _wymix(first_, second_);
}

template<typename... H>
u64 HashCombine(u64 first_, u64 second_, H... hashes_)
{
    u64 combinedHash = HashCombine(first_, second_);
    return HashCombine(combinedHash, hashes_...);
}
} // namespace Zn
