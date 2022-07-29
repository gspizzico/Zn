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

		void Initialize(SDL_Window* window, ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext);

		void ProcessEvent(SDL_Event& event);

		void NewFrame();

		void EndFrame();

		void Shutdown();
	};
}
