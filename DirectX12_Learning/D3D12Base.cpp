#include "stdafx.h"

#include "D3D12Base.h"
#include "Helpers/Utilites.h"
#include "Helpers/d3dx12.h"
#include "Exceptions/DxException.h"

D3D12Base* D3D12Base::m_directXApplication = nullptr;	// Инстанс синглтона приложения.


//! ===================================================  Call-back  для окна приложения  ==================================================
/*
   Здесь вызывается обработчик событий. Будучи виртуальным его можно дополнять в наследниках.											 */
LRESULT CALLBACK MainWindowProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lparam)
{
	return D3D12Base::GetDirectXApplication()->MsgProc(hWindow, msg, wParam, lparam);
}


//! =====================================================  Обработка событий  Windows  ====================================================
/*
																																		 */
LRESULT D3D12Base::MsgProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lparam)
{
	switch (msg) {
	/* WM_ACTIVATE приходит, когда окно становится активным/неактивным. При этом приложение запускается/приостанавливается соответств.   */
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			m_appPaused = true;
			m_timer.Stop();
		} else {
			m_appPaused = false;
			m_timer.Start();
		}
		return 0;
	/* WM_SIZE приходит, когда юзер меняет размеры окна. */
	case WM_SIZE:
		/* Сохраняем новые размеры. */
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
					/* Когда юзер тянет за ресайз-бары и меняет размеры - приходит целая очередь сообщений WM_SIZE.  Ничего  делать  при */
					/* этом не надо - накладно слишком. Лучше дождаться WM_EXITSIZEMOVE и тогда обработать изменения.					 */
				} else {
					OnResize();
				}
			}
		}
		return 0;
	/* WM_ENTERSIZEMOVE приходит, когда юзер захватывает ресайз-бары. */
	case WM_ENTERSIZEMOVE:
		m_appPaused = true;
		m_appResizing = true;
		m_timer.Stop();
		return 0;
	/* WM_ENTERSIZEMOVE приходит, когда юзер отпускает ресайз-бары. */
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
	/* Перехватываем WM_GETMINMAXINFO для предотвращения изменения размеров окна меньше указанных ниже. */
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 200;
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;
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


//! ===========================================================  Цикл  приложения  =========================================================
/*
																																		  */
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
				CalculateFrameStats();
				Update(m_timer);
				Draw(m_timer);
			} else {
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
}


//! ========================================================  Общая  инициализация  =======================================================
/*
   Сначала создаёт окно, в которое будем рисовать, потом инициализирует Direct3D, в конце обязательно вызывает OnResize() - там он в первый
раз задаёт DepthStencil буферу и буферам SwapChain'a требуемые размеры, создаёт на них вьюхи, задаёт размеры Viewport'a и ScissorRect'a. */
bool D3D12Base::Initialize()
{	
	if (!InitMainWindow())
		return false;
	if (!InitDirect3D())
		return false;
	OnResize();
	return true;
}


//! ============================================================  Конструктор  ============================================================
/* 
   Синглтон, при попытке создать второй объект D3D12Base выпадает в assert.																 */
D3D12Base::D3D12Base(HINSTANCE hInstance)
	: m_hInstance(hInstance)
{
	assert(m_directXApplication == nullptr);
	m_directXApplication = this;
}


//! ======================================================  Инициализация  Direct3D  ======================================================
/*
   Создание и инициализация основных объектов для работы DirectX-приложения:  создание фабрики, девайса, Fence,  настройка мультисэмплинга,
объекты для работы с командами (очередь команд, список команд, аллокатор), цепь связи, кучи дескрипторов (RTV, DCV, CBV).				 */
bool D3D12Base::InitDirect3D()
{
#if defined(DEBUG)
{
	/* Включаем вывод отладочной информации																					  (стр. 187) */
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif
	/* Создаём   IDXGIFactory4  - с её помощью можно перечислять адаптеры и т.п. А ещё создать SwapChain.					  (стр. 187) */
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory4)));
	ComPtr<IDXGIAdapter> adapter = nullptr;

#if defined(DEBUG)
	LogAdapters();
#endif

	HRESULT hr = S_FALSE;
	DXGI_ADAPTER_DESC desc = { 0 };

	/* Перечисляем адаптеры. С каждым из них пытаемся создать девайс. При первой успешной попытке заканчиваем перечисление. Если с хардовым
	адаптером не получилось - по-любому получится с эмулятором Microsoft Basic Render Driver.								  (стр. 188) */
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

	/* Создаём ID3D12Fence - с её помощью можно  синхронизировать  выполнение списка  команд  на GPU  с работой  CPU. Также  узнаём размеры
	дескрипторов всех типов. Т.к. они лежат в куче, и известен только адрес первого дескриптора, то для  обращения  к  ним  по  порядковому
	номеру или абсолютному смещению нужно знать их размер, варьирующийся от железа к железу.								  (стр. 181) */
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_RTV_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DSV_descriptrSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CBV_SRV_UAV_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	/* Запрос   NumQualityLevels   для заданного формата заднего буфера ( m_backBufferFormat ) и количества выборок ( 4 ). 	  (стр. 173) */
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = m_backBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
	m_msQualityLevels = msQualityLevels.NumQualityLevels;
	assert(m_msQualityLevels > 0 && "Unexpected MSAA quality level.");

	CreateCommandObjects();		//	Создание объектов  для работы с командами
	CreateSwapChain();			//	Создание цепи связи
	CreateRtvDsvDescriptorHeaps();	//	Создание куч дескрипторов

	return true;
}


//! ==================================================   Инициализация окна приложения   ==================================================
/*
																																		 */
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

	m_hWindow = CreateWindow(
		L"MainWindow", m_mainWindowTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hInstance, 0);

	if (!m_hWindow) {
		MessageBox(0, L"Create window failed.", 0, 0);
		return false;
	}

	ShowWindow(m_hWindow, SW_SHOW);
	UpdateWindow(m_hWindow);
	return true;
}


//! ===========================================================    Деструктор   ===========================================================
/*
   Перед разрушением ожидает выполнения всех команд в очереди GPU																		 */
D3D12Base::~D3D12Base()
{
	if (m_device != nullptr)
		FlushCommandQueue();
}


//! =================================================   Вывод в лог доступных адаптеров   =================================================
/*
																																		 */
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


//! =====================================   Вывод в лог доступных дисплеев для конкретного адаптера   =====================================
/*
																																		 */
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


//! ======================================   Вывод в лог доступных режимов для конкретного дисплея   ======================================
/*
																																		 */
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


//! ============================================   Создание объектов  для работы с командами   ============================================
/*
   Команды в GPU даются в виде списков, размещающихся  в очереди  команд. С объектом  списка связана  область памяти, в которую пишется тот
или иной набор команд. И вот таких наборов может быть много, все они ставятся в очередь и постепенно выполняются GPU. Очередь  представлена
ID3D12CommandQueue,  с  помощью метода  ExecuteCommandLists( число списков, массив списков )  она  добавляет  списки команд  в  очередь  на
выполнение. Список команд представлен базовым  ID3D12CommandList,  он наследуется, н-р,  ID3D12GraphicsCommandList. Тот или иной тип списка
позволяет добавлять определённые команды в список. Эти команды размещаются в памяти с помощью связанного  со  списком  аллокатора,  который
представлен ID3D12CommandAllocator. После добавления команд в список его надо закрыть методом  Close(). В этот момент команды размещаются в
памяти, выделяемой аллокатором. После закрытия (завершения записи команд) список м.б. добавлен в очередь (ExecuteCommandLists()).  Записать
в него ещё какие-то команды уже нельзя. После закрытия списка можно повторить с ним работу уже только  с чистого листа,  вызвав  его  метод
Reset( аллокатор, стэйт конвейера ). Аллокатор при этом можно использовать старый, но его надо тоже ресетнуть Reset().  (стр. 176 - 181) */
void D3D12Base::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue)));
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_cmdList)));
	m_cmdList->Close();
}


//! =======================================================   Создание цепи связи   =======================================================
/*
   SwapChain отвечает за создание и использование back-буферов. Они должны точно соответствовать окну, в которое  происходит  отрисовка, по
   размерам и формату. Она же задаёт частоту перемены буферов, параметры семплирования.							   (стр. 157, 192 - 195) */
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


//! ===============================================   Создание куч дескрипторов RTV и DSV   ===============================================
/*
   В кучах дескрипторов живут дескрипторы (внезапно). С их помощью различные этапы конвейера GPU обращаются к ресурсам, буферам, текстурам.
RTV-дескриптор указывает на буфер, в который попадают результаты рендеринга (OM-state). Им может быть, например, back-буфер из SwapChain'a.
Поскольку в данном случае у нас 2 буфера в SwapChain, то для каждого из них нужно создать дескриптор. В OnResize() этим дескрипторам  будет
сказано указывать на back-буферы SwapChain'a, а в Draw() через них вывод рендеринга будет направляться в back-буферы, каждый раз - в другой
буфер. DSV-дескриптор указывает на бувер теста глубины и трафарета. Этт буфер создаётся далее, в OnResize().  Дескриптор  используется  для
доступа Output Merger'a к буферу глубин (как RTV - к буферу отрисовки).													 (стр. 195, 196) */
void D3D12Base::CreateRtvDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC RTV_heap_desc;				// В куче RenderTargetView-дескрипторов их будет 2 (для каждого back-буфера)
	RTV_heap_desc.NumDescriptors = m_swapChainBuffersCount;
	RTV_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTV_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RTV_heap_desc.NodeMask = 0;

	D3D12_DESCRIPTOR_HEAP_DESC DSV_heap_desc;				// В куче DepthStencilView-дескрипторов будет 1 дескриптор (для 1 depth-буфера)
	DSV_heap_desc.NumDescriptors = 1;
	DSV_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSV_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DSV_heap_desc.NodeMask = 0;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&RTV_heap_desc, IID_PPV_ARGS(m_RTV_heap.GetAddressOf())));
	ThrowIfFailed(m_device->CreateDescriptorHeap(&DSV_heap_desc, IID_PPV_ARGS(m_DSV_heap.GetAddressOf())));
}


//! =======================================   Дождаться окончания выполнения команд в очереди GPU   =======================================
void D3D12Base::FlushCommandQueue()
{
	/* Инкрементим значение Fence */
	m_currentFence++;

	/* Толкаем в очередь команду-маркер "Установить значение Fence = m_currentFence". До  выполнения  этой  команды  значение  Fence  будет
	меньше, что проверяется далее. К моменту выполнения маркера нужно привязаться событием и ждать его наступления.		(стр. 181 - 184) */
	ThrowIfFailed(m_cmdQueue->Signal(m_fence.Get(), m_currentFence));
	if (m_fence->GetCompletedValue() < m_currentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}


//! =======================================================   Подсчёт  статистики   =======================================================
/*
   Таймер выдаёт время в секундах, т.о. каждую секунду подсчитываем FPS и обратную величину. Результаты пишем в название окна.			 */
void D3D12Base::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;
	frameCnt++;
	if ((m_timer.TotalTime() - timeElapsed) >= 1.0f) {
		float fps = (float)frameCnt;
		float msPerFrame = 1000.0f / fps;
		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring msPerFrameStr = std::to_wstring(msPerFrame);
		std::wstring windowText = m_mainWindowTitle + L"        FPS: " + fpsStr + L" msPerFrame: " + msPerFrameStr;
		SetWindowText(m_hWindow, windowText.c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


//! =====================================================   Обработчик Resize Event   =====================================================
/*
   Вызывается сразу после инициализации  окна и  Direct3D - для начальной инициализации back-буферов и depthStencil-буфера -, а  также  при
изменении размеров окна, т.к. нужно пересоздать вышеупомянутые буферы в соответствии с новыми размерами окна и привязать к ним  дескрипторы
соответственно. Viewport и ScissorRect так же преписываются  (но устанавливаются в Draw() для RasteriserState),  т.к.  зависят  от размеров
окна.																													(стр. 196 - 206) */
void D3D12Base::OnResize()
{
	/* К моменту вызова OnResize() девайс, цепь связи и аллокатор должны быть уже созданы. */
	assert(m_device);				
	assert(m_swapChain);
	assert(m_cmdAllocator);

	/* Ждём, когда GPU выполнит команды в очереди (если они там есть) и ресетим список команд, чтоб можно было в него добавлять команды. */
	FlushCommandQueue();			
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));

	/* Ресетим укзатели на ресурсы буферов SwapChain и буфера глубин/трафарета. */
	for (int i = 0; i < m_swapChainBuffersCount; ++i)
		m_swapChainBuffers[i].Reset();	
	m_depthStencilBuffer.Reset();

	/* Задаём всем back-буферам новые размеры и формат, указываем текущий номер back-буфера */
	ThrowIfFailed(m_swapChain->ResizeBuffers(m_swapChainBuffersCount, m_windowWidth, m_windowHeight, m_backBufferFormat, SC_ALLOW_MODE_SWITCH));
	m_currentBackBuffer = 0;

	/* Теперь получаем первый дескриптор из кучи RTV-дескрипторов (их там 2). Потом заполняем массив ресурсов back-буферами.  (стр. 196) */
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RTV_heap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < m_swapChainBuffersCount; ++i)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i])));
		/* Создаём дескриптор на back-буфер. При этом описание, формат дескриптору не задаём (nullptr), т.к.  формат  буфера  при  создании
		Swapchain был определён, т.е. != TYPELESS. */
		m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_RTV_descriptorSize);
	}

	/* Для DSV-дескриптора ресурс ещё не создан (в отличие от ситуации с RTV, т.к. ресурсы back-буферов создаются Swapchain'ом за кулисами)
	Т.о. сначала создаём DepthStencil-буфер. Для этого сперва заполняем дескрипшен с описанием создаваемого ресурса.    (стр. 198 - 203) */
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_windowWidth;
	depthStencilDesc.Height = m_windowHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	/* Формат   ресурса   задаём    неопределённый   -  DXGI_FORMAT_R24G8_TYPELESS,   т.к.   он   будет   использоваться   двумя   вьюхами:
	SRV ( DXGI_FORMAT_R24_UNORM_X8_TYPELESS ) и DSV (формат DXGI_FORMAT_D24_UNORM_S8_UINT). */
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = m_multisamplingEnabled ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m_multisamplingEnabled ? (m_msQualityLevels - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optimalClear;
	optimalClear.Format = m_DS_bufferFormat;
	optimalClear.DepthStencil.Depth = 1.0f;
	optimalClear.DepthStencil.Stencil = 0;

	/* Создаём ресурс */
	ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
													D3D12_HEAP_FLAG_NONE, 
													&depthStencilDesc,
													D3D12_RESOURCE_STATE_COMMON, 
													&optimalClear,
													IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())));

	/* Создаём дескриптор на ресурс DS-буфера. Задаём формат дескриптора DXGI_FORMAT_D24_UNORM_S8_UINT. */
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_DS_bufferFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

	/* Добавляем команду перехода ресурса из общего состояния в состояние записи информации глубины.					 (стр. 185, 186) */
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	ThrowIfFailed(m_cmdList->Close());

	/* Пихаем список в очередь на выполнение, ждём завершение */
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	/* Задаём новое описание для Viewport и ScissorRect */
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width = static_cast<float>(m_windowWidth);
	m_viewPort.Height = static_cast<float>(m_windowHeight);
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	m_scissorRect = { 0, 0, m_windowWidth, m_windowHeight };
}


//! =======================================================   Текущий  Back-буфер   =======================================================
/*
   Возвращает ресурс back-буфера, текущий для данного фрейма. Каждый фрейм индекс текущего буфера переключается (0-1-0-1... для 2 буферов).
Этот индекс отслеживается переменной m_currentBackBuffer. Ресурс соответствующего back-буфера используется в Draw()  для  команд  переходов
этого ресурса из состояния PRESENT в состояние RENDER_TARGET и обратно.																	 */
ID3D12Resource* D3D12Base::CurrentBackBuffer()
{
	return m_swapChainBuffers[m_currentBackBuffer].Get();
}


//! =================================================   Текущий дескриптор  Back-буфера   =================================================
/*
   Возвращает дескриптор, указывающий на текущий back-буфер. Для этого берёт хандлер CD3DX12_CPU_DESCRIPTOR_HANDLE, и смещает его на нужный
дескриптор в соответствии с номером текущего буфера. Этот дескриптор используется в Draw() Output Merger'ом для выбора таргета рисования.*/
D3D12_CPU_DESCRIPTOR_HANDLE D3D12Base::CurrentBackBufferView()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTV_heap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBuffer, m_RTV_descriptorSize);
}


//! =================================================   Дескриптор  DepthStencil-буфера   =================================================
/*
   Этот дескриптор используется в Draw() Output Merger'ом для установки буфера глубин.													 */
D3D12_CPU_DESCRIPTOR_HANDLE D3D12Base::DepthStencilView()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DSV_heap->GetCPUDescriptorHandleForHeapStart());
}
