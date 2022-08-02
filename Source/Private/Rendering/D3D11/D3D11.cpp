#include <Rendering/D3D11/D3D11.h>
#include <Core/Memory/Memory.h>
#include "crtdbg.h" //#todo needed for _ASSERT

using namespace Zn;

#pragma warning(push)
// CreateRenderTargetView
#pragma warning(disable : 6387)

D3D11Device* D3D11Device::CreateDevice(HWND window_handle)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    Memory::Memzero(swapChainDesc);
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = window_handle;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D11Device* device = new D3D11Device();

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &swapChainDesc, &device->m_SwapChain, &device->m_D3DDevice, &featureLevel, &device->m_D3DDeviceContext) != S_OK)
    {
        device->Cleanup();

        delete device;

        return nullptr;
    }

    // Create Render Target

    ID3D11Texture2D* backBuffer{ nullptr };
    device->m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device->m_D3DDevice->CreateRenderTargetView(backBuffer, NULL, &device->m_RenderTargetView);
    backBuffer->Release();

    // 

    return device;
}

void D3D11Device::ResizeWindow()
{
    if (m_SwapChain)
    {
        // Create Render Target

        if (m_RenderTargetView)
        {
            m_RenderTargetView->Release();
            m_RenderTargetView = nullptr;
        }
        
        //////////
        m_SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        //////////

        // Clean render target

        ID3D11Texture2D* backBuffer{ nullptr };
        m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        m_D3DDevice->CreateRenderTargetView(backBuffer, NULL, &m_RenderTargetView);
        backBuffer->Release();
    }
}

#pragma warning(pop)

void D3D11Device::Cleanup()
{
    // Clean render target

    if (m_RenderTargetView)
    {
        m_RenderTargetView->Release();
        m_RenderTargetView = nullptr;
    }

    //

    if (m_SwapChain)
    {
        m_SwapChain->Release();
        m_SwapChain = nullptr;
    }

    if (m_D3DDeviceContext)
    {
        m_D3DDeviceContext->Release();
        m_D3DDeviceContext = nullptr;
    }

    if (m_D3DDevice)
    {
        m_D3DDevice->Release();
        m_D3DDevice = nullptr;
    }
}

void D3D11Device::ClearRenderTarget()
{
    static const float kClearColorWithAlpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
    m_D3DDeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, NULL);
    m_D3DDeviceContext->ClearRenderTargetView(m_RenderTargetView, kClearColorWithAlpha);
}

void D3D11Device::Present(bool vSync)
{
    uint8 vSyncFlag = vSync ? 1 : 0;
    m_SwapChain->Present(vSyncFlag, 0);
}

ID3D11Device* D3D11Device::GetDevice() const
{
    return m_D3DDevice;
}

ID3D11DeviceContext* D3D11Device::GetDeviceContext() const
{
    return m_D3DDeviceContext;
}

D3D11Device::~D3D11Device()
{
	_ASSERT(m_D3DDevice == nullptr);
	_ASSERT(m_D3DDeviceContext == nullptr);
	_ASSERT(m_SwapChain == nullptr);
	_ASSERT(m_RenderTargetView == nullptr);
}
