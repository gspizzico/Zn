#pragma once
#include <d3d11.h>

namespace Zn
{
	class D3D11Device
	{
	public:

		static D3D11Device* CreateDevice(HWND window_handle);

		void ResizeWindow();

		void Cleanup();

		void ClearRenderTarget();

		void Present(bool vSync);

		ID3D11Device* GetDevice() const;
		ID3D11DeviceContext* GetDeviceContext() const;
		
		~D3D11Device();

	private:

		ID3D11Device* m_D3DDevice{ nullptr };
		ID3D11DeviceContext* m_D3DDeviceContext{ nullptr };
		IDXGISwapChain* m_SwapChain{ nullptr };
		ID3D11RenderTargetView* m_RenderTargetView{ nullptr };
	};
}