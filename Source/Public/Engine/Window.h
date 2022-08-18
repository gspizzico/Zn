#pragma once

struct SDL_Window;
struct SDL_Surface;

namespace Zn
{
	class D3D11Device;
	class ImGuiWrapper;

	class Window
	{
	public:

		Window(const int width, const int height, const String& title);

		~Window();

		void PollEvents();

		void NewFrame();

		void EndFrame();

		bool IsRequestingExit() const;

	private:

		SDL_Window* m_Window{ nullptr };
		SDL_Surface* m_Canvas{ nullptr };

		UniquePtr<D3D11Device> m_D3DDevice{ nullptr };

		UniquePtr<ImGuiWrapper> m_ImGui{ nullptr };

		uint64 m_FrameNumber{ 0 };

		bool m_IsRequestingExit{ false };
		bool m_HasPolledEventsThisFrame{ false };
	};
}
