#pragma once
#include <memory>
#include <string>
#include <functional>
#include <Core/AssertionMacros.h>
#include "Core/Containers/Vector.h"
#include "Core/Containers/Map.h"

// glm
// The following define will default align glm:: types.
// There are still alignment problems though. for example
// struct
// {
//		glm::vec3
//		glm::vec3
//		float
// }
// is GPU aligned to 32 while is cpp aligned to 48.
// Leaving it commented because we might find a way to fix this.
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// #define DELEGATE_NAMESPACE TDelegate //TODO: If we want to override the cpp namespace, use this.
#include <delegate.hpp>
// #undef DELEGATE_NAMESPACE

using int8   = char;
using int16  = short;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = unsigned char;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using cstring = const char*;
using sizet   = size_t;

// Testing alternative format

using i8  = char;
using i16 = short;
using i32 = int32_t;
using i64 = int64_t;
using u8  = unsigned char;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

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

enum class ThreadPriority // #TODO move to approriate header
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
    case DataType::Vec2:
        return sizeof(glm::vec2);
    case DataType::Vec3:
        return sizeof(glm::vec3);
    case DataType::Vec4:
        return sizeof(glm::vec4);
    case DataType::Mat3:
        return sizeof(glm::mat3);
    case DataType::Mat4:
        return sizeof(glm::mat4);
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
constexpr const T* ArrayData(const T (&arr)[N]) noexcept
{
    return &arr[0];
}
} // namespace Zn
