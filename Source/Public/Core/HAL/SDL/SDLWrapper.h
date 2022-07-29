#pragma once

namespace Zn
{
	class SDLWrapper
	{
	public:

		static bool Initialize();

		static void Shutdown();

		static bool IsInitialized();

	private:

		SDLWrapper() = default;

		static SDLWrapper& Get();

		bool m_Initialized{ false };
	};
}
