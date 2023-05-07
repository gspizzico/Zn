#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/RHI/RHI.h>
#include <Rendering/RHI/RHITypes.h>
#include <Rendering/RHI/RHIVertex.h>

namespace Zn
{
	struct RHIMesh
	{
		Vector<RHIVertex> vertices;

		RHIBuffer vertexBuffer;
	};
}