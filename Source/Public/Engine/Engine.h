#pragma once
#include <Core/HAL/BasicTypes.h>
#include <Engine/Window.h>

namespace Zn
{
	struct Camera;

	class Engine
	{
	public:
		
		void Start();
		void Shutdown();

	private:

		bool PumpMessages();

		bool m_IsRequestingExit{ false };

		float m_DeltaTime{ 0.f };

		SharedPtr<Window> m_Window;

		SharedPtr<Camera> m_Camera;
	};
}
