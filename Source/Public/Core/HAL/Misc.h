#pragma once
#include "Core/HAL/BasicTypes.h"

namespace Zn
{
	struct Guid;

	enum class ProcessorArchitecture
	{
		x64,
		x86,
		ARM,
		IA64,
		ARM64,
		Unknown
	};

	struct SystemInfo
	{
		uint64 m_PageSize;
		uint64 m_AllocationGranularity;
		uint8 m_NumOfProcessors;
		ProcessorArchitecture m_Architecture;
	};

	class Misc
	{
	public:

		static SystemInfo GetSystemInfo();

		static void Exit(bool with_errors = false);

		static Guid GenerateGuid();
	};
}
