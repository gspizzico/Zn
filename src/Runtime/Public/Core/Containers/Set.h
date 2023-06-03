#pragma once

#include <ankerl_map/unordered_dense.h>

namespace Zn
{
template<typename T>
using Set = ankerl::unordered_dense::set<T>;
} // namespace Zn
