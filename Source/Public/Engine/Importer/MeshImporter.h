#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
	struct RHIMesh;

	class MeshImporter
	{
	public:

		static bool Import(const String& fileName, RHIMesh& mesh);
	};
}
