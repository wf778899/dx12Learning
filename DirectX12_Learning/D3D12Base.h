#pragma once

#include "Timers/GameTimer.h"

using Microsoft::WRL::ComPtr;


class D3D12Base
{
public:
	static D3D12Base* GetDirectXApplication() { return m_directXApplication; }

	HINSTANCE HInstance() const { return m_hInstance; }
	HWND HWindow() const { return m_hWindow; }
	float AspectRatio() const { return static_cast<float>(m_windowWidth) / m_windowHeight; }

	bool MultisamplingEnabled() const { return m_multisamplingEnabled; }
	void EnableMultisampling() { m_multisamplingEnabled = true; CreateSwapChain(); OnResize(); }
	void DisableMultisampling() { m_multisamplingEnabled = false; CreateSwapChain(); OnResize(); }

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lparam);

	int Run();

protected:
	D3D12Base(HINSTANCE hInstance);
	virtual ~D3D12Base();

	virtual void CreateRtvDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const GameTimer &timer) = 0;
	virtual void Draw(const GameTimer &timer) = 0;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

	bool InitMainWindow();
	bool InitDirect3D();

	void CreateCommandObjects();
	void CreateSwapChain();
	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter1 *adapter1);
	void LogOutputDisplayModes(IDXGIOutput *output);
	void FlushCommandQueue();
	void CalculateFrameStats();

	ID3D12Resource *CurrentBackBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView();

	static D3D12Base *m_directXApplication;
	static const int m_swapChainBuffersCount = 2;

	bool m_appPaused = false;
	bool m_appMinimized = false;
	bool m_appMaximized = false;
	bool m_appResizing = false;
	bool m_fullScreenState = false;
	bool m_multisamplingEnabled = false;

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWindow = nullptr;

	ComPtr<IDXGIFactory4> m_factory4;
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<ID3D12DescriptorHeap> m_RTV_heap;
	ComPtr<ID3D12DescriptorHeap> m_DSV_heap;
	ComPtr<ID3D12Resource> m_swapChainBuffers[m_swapChainBuffersCount];	// ������ Com-���������� �� back-������
	ComPtr<ID3D12Resource> m_depthStencilBuffer;						// Com-��������� �� ����� ������/���������

	D3D12_VIEWPORT m_viewPort;
	D3D12_RECT m_scissorRect;
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;		// ������ ������� ������ (������� = �������)
	DXGI_FORMAT m_DS_bufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;		// ������ ������ ������� (24 ���� Depth, 8 ��� Stencil)
	UINT64 m_currentFence = 0;
	UINT64 m_RTV_descriptorSize = 0;
	UINT64 m_DSV_descriptrSize = 0;
	UINT64 m_CBV_SRV_UAV_descriptorSize = 0;
	INT32 m_windowWidth = 1024;
	INT32 m_windowHeight = 768;
	UINT8 m_msQualityLevels = 0;
	UINT8 m_currentBackBuffer = 0;

	GameTimer m_timer;

	std::wstring m_mainWindowTitle = L"DirectX 12 window";

	D3D12Base(const D3D12Base &copy) = delete;
	D3D12Base& operator=(const D3D12Base &rhs) = delete;
};