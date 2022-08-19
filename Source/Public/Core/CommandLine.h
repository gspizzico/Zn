#pragma once

#include <Core/Containers/Vector.h>
#include <Core/HAL/BasicTypes.h>

namespace Zn
{
	class CommandLine
	{
	public:

		static CommandLine& Get();

		void Initialize(char* arguments[], size_t count);

		bool Param(const char* param) const;

		String GetExeArgument() const;

		//bool Value(const char* param, String& out_value) const;

	private:

		String ToLower(const char* param) const; // #todo implement in common library

		Vector<String> arguments_;
	};
}
