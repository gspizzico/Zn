#pragma once

#include <ankerl_map/unordered_dense.h>

namespace Zn
{
template<typename K, typename V>
using Map = ankerl::unordered_dense::map<K, V>;

template<typename K, typename V>
using SegmentedMap = ankerl::unordered_dense::segmented_map<K, V>;
} // namespace Zn
