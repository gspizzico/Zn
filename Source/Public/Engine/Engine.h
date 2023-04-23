#pragma once
#include <Core/HAL/BasicTypes.h>

namespace Zn
{
	struct Camera;

	class EngineFrontend;
	class Window;

	class Engine
	{
	public:

		void Initialize();

		void Update(float deltaTime);
		
		void Start();
		void Shutdown();

	private:

		// Render editor ImGui 
		void RenderUI(float deltaTime);

		void ProcessInput();

		bool PumpMessages();

		float m_DeltaTime{ 0.f };

		SharedPtr<Window> m_Window;

		SharedPtr<Camera> m_Camera;

		SharedPtr<EngineFrontend> m_FrontEnd;
	};
}
