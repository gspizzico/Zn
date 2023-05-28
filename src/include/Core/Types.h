#pragma once

#include <string>
#include <functional>
#include <Core/Platform.h>
#include <Core/AssertionMacros.h>
#include <Core/Containers/Vector.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/Set.h>
// #define DELEGATE_NAMESPACE TDelegate //TODO: If we want to override the cpp namespace, use this.
#include <delegate/delegate.hpp>
// #undef DELEGATE_NAMESPACE

using int8    = char;
using int16   = short;
using int32   = int32_t;
using int64   = int64_t;
using intptr  = intptr_t;
using uint8   = uint8_t;
using uint16  = uint16_t;
using uint32  = uint32_t;
using uint64  = uint64_t;
using uintptr = uintptr_t;
using ptrdiff = ptrdiff_t;

using cstring = const char*;
using sizet   = size_t;

// Testing alternative format

using i8   = char;
using i16  = short;
using i32  = int32;
using i64  = int64_t;
using iptr = intptr_t;
using u8   = uint8_t;
using u16  = uint16_t;
using u32  = uint32_t;
using u64  = uint64_t;
using uptr = uintptr_t;

using f32 = float;
using f64 = double;

static constexpr u64 u64_max = std::numeric_limits<u64>::max();
static constexpr i64 i64_max = std::numeric_limits<i64>::max();
static constexpr u32 u32_max = std::numeric_limits<u32>::max();
static constexpr i32 i32_max = std::numeric_limits<i32>::max();
static constexpr u16 u16_max = std::numeric_limits<u16>::max();
static constexpr i16 i16_max = std::numeric_limits<i16>::max();
static constexpr u8  u8_max  = std::numeric_limits<u8>::max();
static constexpr i8  i8_max  = std::numeric_limits<i8>::max();

namespace Zn
{
using String = std::string;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedFromThis = std::enable_shared_from_this<T>;

template<typename Signature>
using TDelegate = DELEGATE_NAMESPACE_INTERNAL::delegate<Signature>;

enum class ThreadPriority : u8 // #TODO move to approriate header
{
    Idle,
    Lowest,
    Low,
    Normal,
    High,
    Highest,
    TimeCritical
};

enum class DataType : u8
{
    UInt8,
    UInt16,
    UInt32,
    Int8,
    Int16,
    Int32,
    Float,
    Float16,
    Double,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    COUNT,
};

constexpr sizet SizeOfDataType(DataType e)
{
    switch (e)
    {
    case DataType::UInt8:
        return sizeof(u8);
    case DataType::UInt16:
        return sizeof(u16);
    case DataType::UInt32:
        return sizeof(u32);
    case DataType::Int8:
        return sizeof(i8);
    case DataType::Int16:
        return sizeof(i16);
    case DataType::Int32:
        return sizeof(i32);
    case DataType::Float:
        return sizeof(f32);
    case DataType::Double:
        return sizeof(f64);
    // case DataType::Vec2:
    //     return sizeof(glm::vec2);
    // case DataType::Vec3:
    //     return sizeof(glm::vec3);
    // case DataType::Vec4:
    //     return sizeof(glm::vec4);
    // case DataType::Mat3:
    //     return sizeof(glm::mat3);
    // case DataType::Mat4:
    //     return sizeof(glm::mat4);
    default:
    case DataType::Float16: // Unsupported yet
    case DataType::COUNT:
        break;
    }

    return 0;
}

template<typename T, u32 N>
constexpr u32 ArrayLength(const T (&)[N]) noexcept
{
    return N;
}

template<typename T, u32 N>
constexpr u32 ArrayElementSize(const T (&)[N]) noexcept
{
    return sizeof(T);
}

template<typename T, u32 N>
constexpr const T* ArrayData(const T (&arr_)[N]) noexcept
{
    return &arr_[0];
}

// constexpr function to determine the size of a const string.
template<size_t N>
constexpr size_t StrLen(char const (&)[N])
{
    return N - 1;
}
} // namespace Zn
