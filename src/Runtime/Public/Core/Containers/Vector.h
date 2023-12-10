#pragma once

#include <vector>
#include <ankerl_map/unordered_dense.h>

namespace Zn
{
template<typename T, typename A = std::allocator<T>>
using Vector = std::vector<T, A>;

template<typename T>
using ChunkedVector = ankerl::unordered_dense::segmented_vector<T>;
} // namespace Zn
