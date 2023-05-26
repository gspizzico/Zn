#pragma once

#include <map>
#include <unordered_map>

namespace Zn
{
template<typename K, typename V> using Map = std::pmr::map<K, V>;

template<typename K, typename V> using UnorderedMap = std::pmr::unordered_map<K, V>;
} // namespace Zn