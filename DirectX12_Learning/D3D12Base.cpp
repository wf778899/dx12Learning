#include "stdafx.h"
#include "D3D12Base.h"

LRESULT CALLBACK MainWindowProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lparam)
{
	return D3D12Base::GetDirectXApplication()->MsgProc(hWindow, msg, wParam, lparam);
}


LRESULT D3D12Base::MsgProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lparam)
{
	switch (msg) {
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			m_appPaused = true;
			m_timer.Stop();
		} else {
			m_appPaused = false;
			m_timer.Start();
		}
		return 0;
	case WM_SIZE:
		m_windowWidth = LOWORD(lparam);
		m_windowHeight = HIWORD(lparam);
		if (m_device) {
			if (wParam == SIZE_MINIMIZED) {
				m_appPaused = true;
				m_appMinimized = true;
				m_appMaximized = false;
			} else if (wParam == SIZE_MAXIMIZED) {
				m_appPaused = false;
				m_appMinimized = false;
				m_appMaximized = true;
				OnResize();
			} else if (wParam == SIZE_RESTORED) {
				if (m_appMinimized) {
					m_appPaused = false;
					m_appMinimized = false;
					OnResize();
				} else if (m_appMaximized) {
					m_appPaused = false;
					m_appMaximized = false;
					OnResize();
				} else if (m_appResizing) {

				} else {
					OnResize();
				}
			}
		}
		return 0;
	case WM_ENTERSIZEMOVE:
		m_appPaused = true;
		m_appResizing = true;
		m_timer.Stop();
		return 0;
	case WM_EXITSIZEMOVE:
		m_appPaused = false;
		m_appResizing = false;
		m_timer.Start();
		OnResize();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 200;
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		//TODO: реализовать onMouseDown()
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		//TODO: реализовать onMouseUp()
		return 0;
	case WM_MOUSEMOVE:
		//TODO: реализовать onMouseMove()
	case WM_KEYUP:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		} else if ((int)wParam == VK_F2) {
			m_multisamplingEnabled = !m_multisamplingEnabled;
		}
		return 0;
	}
	return DefWindowProc(hWindow, msg, wParam, lparam);
}


int D3D12Base::Run()
{
	MSG msg = { 0 };
	m_timer.Reset();

	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			m_timer.Tick();
			if (!m_appPaused) {
				//TODO: реализовать CalculateFrameStats(), Update(), Draw()
			} else {
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
}


D3D12Base* D3D12Base::m_directXApplication = nullptr;


D3D12Base* D3D12Base::GetDirectXApplication()
{
	return m_directXApplication;
}


bool D3D12Base::Initialize()
{	
	if (!InitMainWindow())
		return false;
	if (!InitDirect3D())
		return false;

	OnResize();

	return true;
}




D3D12Base::D3D12Base(HINSTANCE hInstance)
	: m_hInstance(hInstance)
{
	assert(m_directXApplication == nullptr);
	m_directXApplication = this;
}


bool D3D12Base::InitDirect3D()
{
#if defined(DEBUG)
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory4)));
	ComPtr<IDXGIAdapter> adapter = nullptr;

#if defined(DEBUG)
	LogAdapters();
#endif

	HRESULT hr = S_FALSE;
	DXGI_ADAPTER_DESC desc = { 0 };

	for (int i = 0; m_factory4->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapter->GetDesc(&desc);
		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
		OutputDebugString( ToString(desc, L"Trying to create device with adapter:", true).c_str() );
		if (SUCCEEDED(hr)) {
			OutputDebugString(L"... Success.\n");
			break;
		}
		OutputDebugString(L"... Failed.\n");
	}
	if (FAILED(hr)) {
		return false;
	}

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_RTV_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DSV_descriptrSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CBV_SRV_UAV_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = m_backBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
	m_msQualityLevels = msQualityLevels.NumQualityLevels;
	assert(m_msQualityLevels > 0 && "Unexpected MSAA quality level.");

	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeaps();

	return true;
}

bool D3D12Base::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWindow";

	if (!RegisterClass(&wc)) {
		MessageBox(0, L"Register class failed.", 0, 0);
		return false;
	}

	RECT rect = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	m_hWindow = CreateWindow(L"MainWindow", m_mainWindowTitle.c_str(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hInstance, 0);

	if (!m_hWindow) {
		MessageBox(0, L"Create window failed.", 0, 0);
		return false;
	}

	ShowWindow(m_hWindow, SW_SHOW);
	UpdateWindow(m_hWindow);
	return true;
}

D3D12Base::~D3D12Base()
{
}

void D3D12Base::LogAdapters()
{
	IDXGIAdapter1 *adapter1 = nullptr;

	for (int i = 0; m_factory4->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC adapterDesc = { 0 };
		adapter1->GetDesc(&adapterDesc);
		OutputDebugString( ToString(adapterDesc, L"***Adapter:", true).c_str() );
		LogAdapterOutputs( adapter1 );
		ReleaseCom( adapter1 );
	}

}

void D3D12Base::LogAdapterOutputs(IDXGIAdapter1 *adapter1)
{
	IDXGIOutput *output = nullptr;
	for (int i = 0; adapter1->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_OUTPUT_DESC outputDesc = { 0 };
		output->GetDesc(&outputDesc);
		OutputDebugString( ToString(outputDesc, L"\t***Output:", true).c_str() );
		LogOutputDisplayModes(output);
		ReleaseCom(output);
	}
}

void D3D12Base::LogOutputDisplayModes(IDXGIOutput *output)
{
	UINT count = 0;
	UINT flags = 0;
	output->GetDisplayModeList(m_backBufferFormat, flags, &count, nullptr);
	std::vector<DXGI_MODE_DESC> modes(count);
	output->GetDisplayModeList(m_backBufferFormat, flags, &count, &modes[0]);
	for (DXGI_MODE_DESC mode : modes) {
		UINT n = mode.RefreshRate.Numerator;
		UINT d = mode.RefreshRate.Denominator;
		std::wstring text = L"\t\tDimentions: " + std::to_wstring(mode.Width) + L" x " + std::to_wstring(mode.Height) +
			L"Refresh: " + std::to_wstring(n) + L" / " + std::to_wstring(d) + L"\n";
		OutputDebugString(text.c_str());
	}
}

void D3D12Base::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue)));
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmdAllocator.GetAddressOf())));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), nullptr, IID_PPV_ARGS(m_cmdList.GetAddressOf())));
	m_cmdList->Close();
}

void D3D12Base::CreateSwapChain()
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = m_windowWidth;
	sd.BufferDesc.Height = m_windowHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = m_backBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = m_multisamplingEnabled ? 4 : 1;
	sd.SampleDesc.Quality = m_multisamplingEnabled ? (m_msQualityLevels - 1) : 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = m_swapChainBuffersCount;
	sd.OutputWindow = m_hWindow;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ThrowIfFailed(m_factory4->CreateSwapChain(m_cmdQueue.Get(), &sd, m_swapChain.GetAddressOf()));
}

void D3D12Base::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC RTV_heap_desc;
	RTV_heap_desc.NumDescriptors = m_swapChainBuffersCount;
	RTV_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTV_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RTV_heap_desc.NodeMask = 0;

	D3D12_DESCRIPTOR_HEAP_DESC DSV_heap_desc;
	DSV_heap_desc.NumDescriptors = 1;
	DSV_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSV_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DSV_heap_desc.NodeMask = 0;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&RTV_heap_desc, IID_PPV_ARGS(m_RTV_heap.GetAddressOf())));
	ThrowIfFailed(m_device->CreateDescriptorHeap(&DSV_heap_desc, IID_PPV_ARGS(m_DSV_heap.GetAddressOf())));
}

void D3D12Base::FlushCommandQueue()
{
	m_currentFence++;
	ThrowIfFailed(m_cmdQueue->Signal(m_fence.Get(), m_currentFence));
	if (m_fence->GetCompletedValue() < m_currentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3D12Base::OnResize()
{
	assert(m_device);
	assert(m_swapChain);
	assert(m_cmdAllocator);

	FlushCommandQueue();

	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
	for (int i = 0; i < m_swapChainBuffersCount; ++i) {
		m_swapChainBuffers[i].Reset();
	}
	m_depthStencilBuffer.Reset();
	ThrowIfFailed(m_swapChain->ResizeBuffers(m_swapChainBuffersCount, m_windowWidth, m_windowHeight, m_backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	m_currentBackBuffer = 0;


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RTV_heap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < m_swapChainBuffersCount; ++i) {
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i])));
		m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_RTV_descriptorSize);
	}

	// Формат ресурса DXGI_FORMAT_R24G8_TYPELESS, т.к.он будет использоваться двумя вьюхами: SRV (формат DXGI_FORMAT_R24_UNORM_X8_TYPELESS) и
	// DSV (формат DXGI_FORMAT_D24_UNORM_S8_UINT).
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_windowWidth;
	depthStencilDesc.Height = m_windowHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = m_multisamplingEnabled ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m_multisamplingEnabled ? (m_msQualityLevels - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimalClear;
	optimalClear.Format = m_DS_bufferFormat;
	optimalClear.DepthStencil.Depth = 1.0f;
	optimalClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optimalClear,
		IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_DS_bufferFormat;
	dsvDesc.Texture2D.MipSlice = 0;

	m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, DepthStencilView());
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width = static_cast<float>(m_windowWidth);
	m_viewPort.Height = static_cast<float>(m_windowHeight);
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	m_scissorRect.top = 0;
	m_scissorRect.left = 0;
	m_scissorRect.bottom = m_windowHeight;
	m_scissorRect.right = m_windowWidth;

}

ID3D12Resource* D3D12Base::CurrentBackBuffer()
{
	return m_swapChainBuffers[m_currentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Base::CurrentBackBufferView()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTV_heap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBuffer, m_RTV_descriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Base::DepthStencilView()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DSV_heap->GetCPUDescriptorHandleForHeapStart());
}
