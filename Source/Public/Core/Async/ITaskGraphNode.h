#pragma once
#include "Core/Name.h"

namespace Zn
{
	class ITaskGraphNode abstract
	{
	public:
		virtual void Execute() = 0;

		virtual Name GetName() const = 0;

		virtual void DumpNode() const = 0;
	};
}