#pragma once
#include <memory>
#include <string>
#include <functional>
#include "Core/Containers/Vector.h"

using int8 = char;
using int16 = short;
using int32 = int;
using int64 = __int64;
using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned __int64;

// Testing alternative format

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = __int64;
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned __int64;

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
}