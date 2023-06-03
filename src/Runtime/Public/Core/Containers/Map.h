#pragma once

#include <ankerl_map/unordered_dense.h>

#define MAP_HASH_NAMESPACE ankerl::unordered_dense

namespace Zn
{
template<typename K, typename V, typename H = MAP_HASH_NAMESPACE::hash<K>>
using Map = ankerl::unordered_dense::map<K, V, H>;

template<typename K, typename V, typename H = MAP_HASH_NAMESPACE::hash<K>>
using SegmentedMap = ankerl::unordered_dense::segmented_map<K, V, H>;
} // namespace Zn
