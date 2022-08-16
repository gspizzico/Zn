#include <Znpch.h>
#include <Core/CommandLine.h>
#include <algorithm>

using namespace Zn;

CommandLine& CommandLine::Get()
{
	static CommandLine Instance;
	return Instance;
}

void CommandLine::Initialize(char* arguments[], size_t count)
{
	arguments_.reserve(count);

	for (size_t index = 0; index < count; ++index)
	{
		arguments_.emplace_back(ToLower(arguments[index]));
	}
}

bool CommandLine::Param(const char* param) const
{
	return std::any_of(arguments_.begin(), arguments_.end(), [lower = ToLower(param)](const auto& arg)
	{
		return arg.compare(lower) == 0;
	});
}

String CommandLine::ToLower(const char* param) const
{
	const auto size = strlen(param);
	String lower;
	lower.resize(size + 1);

	for (size_t index = 0; index < strlen(param); ++index)
	{
		lower[index] = std::tolower(param[index]);
	}
	lower[size] = '\0';

	return lower;
}