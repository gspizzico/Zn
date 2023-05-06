#pragma once
#include <memory>
#include <string>
#include <functional>
#include "Core/Containers/Vector.h"

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

//#define DELEGATE_NAMESPACE TDelegate //TODO: If we want to override the cpp namespace, use this.
#include <delegate.hpp>
// #undef DELEGATE_NAMESPACE

using int8 = char;
using int16 = short;
using int32 = int32_t;
using int64 = __int64;
using uint8 = unsigned char;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using cstring = const char*;

// Testing alternative format

using i8 = char;
using i16 = short;
using i32 = int32_t;
using i64 = __int64;
using u8 = unsigned char;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

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

	enum class ThreadPriority //#TODO move to approriate header
	{
		Idle,
		Lowest,
		Low,
		Normal,
		High,
		Highest,
		TimeCritical
	};

	template<typename T, u32 N>
	constexpr u32 ArrayLength(const T(&)[N]) noexcept
	{
		return N;
	}

	template<typename T, u32 N>
	constexpr u32 ArrayElementSize(const T(&)[N]) noexcept
	{
		return sizeof(T);
	}

	template<typename T, u32 N>
	constexpr const T* ArrayData(const T(&arr)[N]) noexcept
	{
		return &arr[0];
	}
}