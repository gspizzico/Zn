#pragma once

struct SDL_Window;
struct SDL_Surface;
union SDL_Event;

namespace Zn
{
	class D3D11Device;
	class VulkanDevice;
	class ImGuiWrapper;

	class Window
	{
	public:

		Window(const int width, const int height, const String& title);

		~Window();

		void PollEvents(float delta_time);

		void NewFrame(float delta_time);

		void EndFrame();

		bool IsRequestingExit() const;

		// void* GetHandle() const;

	private:

		SDL_Window* m_Window{ nullptr };
		SDL_Surface* m_Canvas{ nullptr };

		UniquePtr<VulkanDevice> m_VulkanDevice{ nullptr };

		UniquePtr<ImGuiWrapper> m_ImGui{ nullptr };

		uint64 m_FrameNumber{ 0 };

		void* m_NativeHandle{nullptr};

		bool m_IsRequestingExit{ false };
		bool m_HasPolledEventsThisFrame{ false };
		bool m_IsMinimized{ false };

		// Handle Input

		void HandleInput(const SDL_Event& InEvent, float delta_time);

		bool m_RightMouseDown = false;
	};
}
