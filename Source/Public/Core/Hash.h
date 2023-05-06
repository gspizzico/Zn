#pragma once

#include <Core/HAL/BasicTypes.h>
#include <wyhash.h>

namespace Zn
{
	template<typename T>
	inline u64 HashCalculate(const T& value, sizet seed = 0)
	{
		return wyhash(&value, sizeof(T), seed, _wyp);
	}

	template<size_t N>
	inline u64 HashCalculate(const char(&value)[N], sizet seed = 0)
	{
		return wyhash(value, strlen(value), seed, _wyp);
	}

	template<>
	inline u64 HashCalculate(const cstring& value, sizet seed)
	{
		return wyhash(value, strlen(value), seed, _wyp);
	}

	inline u64 HashBytes(const void* data, sizet length, sizet seed = 0)
	{
		return wyhash(data, length, seed, _wyp);
	}

	inline u64 HashCombine(u64 first, u64 second)
	{
		return _wymix(first, second);
	}

	template<typename ...H>
	u64 HashCombine(u64 first, u64 second, H... hashes)
	{
		u64 combinedHash = HashCombine(first, second);
		return HashCombine(combinedHash, hashes...);
	}
}
