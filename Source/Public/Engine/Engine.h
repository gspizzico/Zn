#pragma once
#include <Core/HAL/BasicTypes.h>
#include <Engine/Window.h>

namespace Zn
{
	class Engine
	{
	public:

		void Initialize();
		void Start();
		void Shutdown();

	private:

		bool m_IsRequestingExit{ false };

		float m_DeltaTime{ 0.f };

		UniquePtr<Window> m_Window;
	};
}
