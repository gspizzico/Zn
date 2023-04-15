#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Core/Containers/Vector.h>

template<typename OutputType, typename VkFunction, typename ...Args>
Zn::Vector<OutputType> VkEnumerate(VkFunction&& function, Args&&... args)
{
	uint32 count = 0;
	function(std::forward<Args>(args)..., &count, nullptr);
	Zn::Vector<OutputType> output(count);
	function(std::forward<Args>(args)..., &count, output.data());
	return output;
}
