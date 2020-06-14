#pragma once

#include "Core/HAL/BasicTypes.h"

namespace Zn
{
	struct Guid
	{
		uint32 A = 0;
		uint32 B = 0;
		uint32 C = 0;
		uint32 D = 0;

		bool operator==(const Guid& other) const
		{
			return A == other.A &&
				B == other.B &&
				C == other.C &&
				D == other.D;
		}

		String ToString() const;

		static Guid Generate();

		//static Guid FromString();

		static const Guid kNone;
	};
}

namespace std
{
	template<> struct hash<Zn::Guid>
	{
		std::size_t operator()(Zn::Guid const& guid) const noexcept
		{
			return hash<Zn::String>{}(guid.ToString());
		}
	};
}