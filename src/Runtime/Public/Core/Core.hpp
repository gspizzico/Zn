/*
MIT License

Copyright (c) 2023 Giuseppe Spizzico

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#ifndef _ZN_CORE_TYPES_H_
#define _ZN_CORE_TYPES_H_

#include <functional>
#include <type_traits>

#define ENABLE_BITMASK_OPERATORS(EnumType)                                                                                                 \
    inline EnumType operator|(EnumType a, EnumType b)                                                                                      \
    {                                                                                                                                      \
        return static_cast<EnumType>(static_cast<std::underlying_type<EnumType>::type>(a) |                                                \
                                     static_cast<std::underlying_type<EnumType>::type>(b));                                                \
    }                                                                                                                                      \
    inline EnumType operator&(EnumType a, EnumType b)                                                                                      \
    {                                                                                                                                      \
        return static_cast<EnumType>(static_cast<std::underlying_type<EnumType>::type>(a) &                                                \
                                     static_cast<std::underlying_type<EnumType>::type>(b));                                                \
    }                                                                                                                                      \
    inline EnumType operator~(EnumType a)                                                                                                  \
    {                                                                                                                                      \
        return static_cast<EnumType>(~static_cast<std::underlying_type<EnumType>::type>(a));                                               \
    }                                                                                                                                      \
    inline EnumType& operator|=(EnumType& a, EnumType b)                                                                                   \
    {                                                                                                                                      \
        return a = a | b;                                                                                                                  \
    }                                                                                                                                      \
    inline EnumType& operator&=(EnumType& a, EnumType b)                                                                                   \
    {                                                                                                                                      \
        return a = a & b;                                                                                                                  \
    }                                                                                                                                      \
    inline bool EnumHasAll(EnumType a, EnumType b)                                                                                         \
    {                                                                                                                                      \
        return (a & b) == b;                                                                                                               \
    }                                                                                                                                      \
    inline bool EnumHasAny(EnumType a, EnumType b)                                                                                         \
    {                                                                                                                                      \
        return std::underlying_type_t<EnumType>(a & b) > 0;                                                                                \
    }                                                                                                                                      \
    inline bool EnumHasNone(EnumType a, EnumType b)                                                                                        \
    {                                                                                                                                      \
        return !EnumHasAll(a, b);                                                                                                          \
    }

// Usage
// enum class MyEnum : unsigned int
//{
//    None  = 0,
//    Flag1 = 1 << 0,
//    Flag2 = 1 << 1,
//    Flag3 = 1 << 2,
//};
//
// ENABLE_BITMASK_OPERATORS(MyEnum);

#if ZN_TYPES_UNDER_NAMESPACE
namespace Zn
{
#endif
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

#if !ZN_TYPES_UNDER_NAMESPACE
namespace Zn
{
#endif

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
constexpr u32 SizeOf(const T (&)[N]) noexcept
{
    return N;
}

template<typename T, u32 N>
constexpr u32 SizeOfElement(const T (&)[N]) noexcept
{
    return sizeof(T);
}

template<typename T, u32 N>
constexpr u32 SizeOfArray(const T (&)[N]) noexcept
{
    return sizeof(T) * N;
}

template<typename T, u32 N>
constexpr const T* DataPtr(const T (&arr_)[N]) noexcept
{
    return &arr_[0];
}

template<typename T, u32 N>
constexpr T* DataPtr(T (&arr_)[N]) noexcept
{
    return &arr_[0];
}

// constexpr function to determine the size of a const string.
template<size_t N>
constexpr size_t StrLen(char const (&)[N])
{
    return N - 1;
}

enum class StorageUnit : uint64
{
    Byte     = 1,
    KiloByte = Byte << 10,
    MegaByte = Byte << 20,
    GigaByte = Byte << 30
};

namespace MemoryAlignment
{
enum
{
    MinAlignment     = sizeof(std::max_align_t),
    DefaultAlignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__,
};
} // namespace MemoryAlignment

class Memory
{
  public:
    Memory()  = delete;
    ~Memory() = delete;

    template<typename T, typename U>
    static ptrdiff GetDistance(const T* first_, const U* second_)
    {
        return reinterpret_cast<intptr>(first_) - reinterpret_cast<intptr>(second_);
    }

    template<typename T>
    static T* AddOffset(T* address_, sizet offset_)
    {
        return reinterpret_cast<T*>((uintptr) (address_) + (uintptr) offset_);
    }

    template<typename T>
    static T* SubOffset(T* address_, sizet offset_)
    {
        uintptr address = (uintptr) address_;
        uintptr offset  = (uintptr) offset_;
        if (offset > address)
        {
            return nullptr;
        }
        return reinterpret_cast<T*>(address - offset);
    }

    template<typename T>
    static constexpr T Align(T bytes_, sizet alignment_ = MemoryAlignment::MinAlignment)
    {
        static_assert(std::is_integral_v<T> && (!std::is_same<T, void>::value) || sizeof(T) <= sizeof(sizet));

        const sizet mask = alignment_ == 0 ? alignment_ : alignment_ - 1;
        return T(((sizet) bytes_ + mask) & ~mask);
    }

    template<typename T>
    static constexpr T* Align(T* address_, sizet alignment_ = MemoryAlignment::MinAlignment)
    {
        return (T*) (Align(reinterpret_cast<uintptr>(address_), alignment_));
    }

    template<typename T>
    static constexpr T* AlignSafe(T* address_, sizet& bufferSize_, sizet alignment_ = MemoryAlignment::MinAlignment)
    {
        T* alignedAddress = Align(address_, alignment_);

        constexpr sizet typeSize = sizeof(std::conditional<std::is_same_v<T, void>, uint8, T>::type);

        const sizet padding = GetDistance(alignedAddress, address_);

        if (padding + typeSize <= bufferSize_)
        {
            bufferSize_ -= padding + typeSize;
            return alignedAddress;
        }

        return nullptr;
    }

    template<typename T>
    static constexpr bool IsAligned(const T* address_, sizet alignment_ = MemoryAlignment::MinAlignment)
    {
        return ((uintptr) (address_)) % alignment_ == 0;
    }

    template<typename T>
    static void Memset(T* begin_, T* end_, uint8 pattern_)
    {
        memset(begin_, pattern_, (sizet) GetDistance(end_, begin_));
    }

    template<typename T>
    static void Memset(T* begin_, sizet size_, uint8 pattern_)
    {
        memset(begin_, pattern_, size_);
    }

    template<typename T>
    static void Memzero(T* begin_, T* end_)
    {
        memset(begin_, 0, (sizet) GetDistance(end_, begin_));
    }

    template<typename T>
    static void Memzero(T* begin_, sizet size_)
    {
        memset(begin_, 0, size_);
    }

    template<typename T>
    static void Memzero(T& data_)
    {
        Memzero(std::addressof(data_), sizeof(T));
    }

    template<typename T, sizet N>
    static void Memzero(T (&data_)[N])
    {
        Memzero(DataPtr(data_), sizeof(T) * N);
    }

    static constexpr uint64 Convert(uint64 size_, StorageUnit convertTo_, StorageUnit convertFrom_)
    {
        return size_ * (uint64) (convertFrom_) / (uint64) convertTo_;
    }
};

class MemoryDebug
{
  public:
    MemoryDebug()  = delete;
    ~MemoryDebug() = delete;

    template<typename T>
    static void MarkUninitialized(T* begin_, T* end_)
    {
#if (_DEBUG)
        Memory::Memset(begin_, end_, kUninitializedMemoryPattern);
#endif
    }
    template<typename T>
    static void MarkUninitialized(T* begin_, sizet size_)
    {
#if (_DEBUG)
        Memory::Memset(begin_, size_, kUninitializedMemoryPattern);
#endif
    }

    template<typename T>
    static void MarkFree(T* begin_, T* end_)
    {
#if (_DEBUG)
        Memory::Memset(begin_, end_, kFreeMemoryPattern);
#endif
    }

    template<typename T>
    static void MarkFree(T* begin_, sizet size_)
    {
#if (_DEBUG)
        Memory::Memset(begin_, size_, kFreeMemoryPattern);
#endif
    }

    static constexpr uint8 kUninitializedMemoryPattern = 0xDD; // DeaD

    static constexpr uint8 kFreeMemoryPattern = 0xFE; // FrEe
};

struct MemoryRange
{
  public:
    MemoryRange() = default;

    template<typename T>
    MemoryRange(T* begin_, T* end_)
        : begin((void*) begin_)
        , end((void*) end_) {};

    template<typename T>
    MemoryRange(T* begin_, sizet size_)
        : begin((void*) begin_)
        , end(Memory::AddOffset((void*) begin_, size_))
    {
    }

    explicit MemoryRange(const MemoryRange& other_, sizet alignment_)
        : begin(Memory::Align(other_.begin, alignment_))
        , end(other_.end)
    {
    }

    MemoryRange(MemoryRange&& other_) noexcept
        : begin(other_.begin)
        , end(other_.end)
    {
        Memory::Memzero(&other_, sizeof(MemoryRange));
    }

    MemoryRange(const MemoryRange& other_)
        : begin(other_.begin)
        , end(other_.end)
    {
    }

    MemoryRange& operator=(const MemoryRange& other_)
    {
        begin = other_.begin;
        end   = other_.end;
        return *this;
    }

    bool operator==(const MemoryRange& other_) const
    {
        return begin == other_.begin && end == other_.end;
    }

    template<typename T>
    bool Contains(const T* address_) const
    {
        return Memory::GetDistance(address_, begin) >= 0 && Memory::GetDistance(address_, end) < 0;
    }

    bool Contains(const MemoryRange& other_) const
    {
        if (*this == other_)
            return true;

        const auto distanceFromStart = Memory::GetDistance(other_.begin, begin);

        const auto distanceFromEnd = Memory::GetDistance(end, other_.end);

        return (distanceFromStart >= 0 && distanceFromEnd > 0) || (distanceFromStart > 0 && distanceFromEnd >= 0);
    }

    template<typename T, sizet N>
    static bool Contains(const T (&arr_)[N], T* ptr_)
    {
        return MemoryRange(DataPtr(arr_), sizeof(T) * N).Contains(ptr_);
    }

    sizet Size() const
    {
        return static_cast<sizet>(Memory::GetDistance(end, begin));
    }

    void* begin = nullptr;

    void* end = nullptr;
};
} // namespace Zn
#endif // _ZN_CORE_TYPES_H_
