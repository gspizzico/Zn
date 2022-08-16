#pragma once

#include <set>
#include <unordered_set>

namespace Zn
{
	template<typename T>
	using Set = std::pmr::set<T>;

	template<typename T>
	using UnorderedSet = std::pmr::unordered_set<T>;
}