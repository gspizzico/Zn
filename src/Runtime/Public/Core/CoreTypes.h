#pragma once

#include <Core/Core.hpp>

#include <Core/Containers/Vector.h>
#include <Core/Containers/Map.h>
#include <Core/Containers/Set.h>
// #define DELEGATE_NAMESPACE TDelegate //TODO: If we want to override the cpp namespace, use this.
#include <delegate/delegate.hpp>
// #undef DELEGATE_NAMESPACE

#include <string>
#include <functional>

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

enum class ThreadPriority : u8 // #TODO move to approriate header
{
    Idle,
    Lowest,
    Low,
    Normal,
    High,
    Highest,
    TimeCritical
};
} // namespace Zn
