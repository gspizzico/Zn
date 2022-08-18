#pragma once

union SDL_Event;
struct SDL_Window;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Zn
{
	class ImGuiWrapper
	{
	public:

		void Initialize();

		void ProcessEvent(SDL_Event& event);

		void NewFrame();

		void EndFrame();

		void Shutdown();
	};
}
