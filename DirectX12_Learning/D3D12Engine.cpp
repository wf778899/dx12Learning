#include "stdafx.h"
#include "D3D12Engine.h"

const UINT g_numFrameResources = 3;

//! ============================================================  Конструктор  ============================================================
D3D12Engine::D3D12Engine(HINSTANCE hInstance) : D3D12Base(hInstance) {}


//! ===========================================================    Деструктор   ===========================================================
D3D12Engine::~D3D12Engine() {}


//! ==========================================================   Иинициализация   =========================================================
bool D3D12Engine::Initialize()
{
	if (!D3D12Base::Initialize())
		return false;

	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
	BuildRootSignature();
	BuildShadersAndInputLayout();
	//BuildBoxGeometry();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	CreateCbvDescriptorHeaps();
	CreateCBVdescriptorHeaps();
	Build_ConstantBuffers();
	BuildConstantBuffers();
	BuildPSO();
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList *cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	FlushCommandQueue();
	return true;
}


//! =====================================================   Обработчик Resize Event   =====================================================
void D3D12Engine::OnResize()
{
	D3D12Base::OnResize();
	XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}


//! =============================================================   Update   ==============================================================
/*
   Здесь каждый кадр меняем матрицы объектов и пишем их в константные буферы.															 */
void D3D12Engine::Update(const GameTimer & timer)
{
	OnKeyboardInput(timer);
	UpdateCamera(timer);

	m_currFrameIndex = (m_currFrameIndex + 1) % g_numFrameResources;
	m_currFrame = m_frameResources[m_currFrameIndex].get();

	if (m_currFrame->fenceVal != 0 && m_fence->GetCompletedValue() < m_currFrame->fenceVal)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_currFrame->fenceVal, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//Constants constants;

	//XMMATRIX view = XMLoadFloat4x4(&m_view);

	///* Получаем Projection-матрицу (получена ранее, в инициализирующем вызове OnResize()).												 */
	//XMMATRIX proj = XMLoadFloat4x4(&m_proj);

	///* Строим World-матрицу для Куба, вычисляем WVP-матрицу и пишем её в буфер пообъектных констант [0].								 */
	//XMMATRIX world = XMMatrixTranslation(-1.0f, -1.0f, -1.0f);
	//XMMATRIX worldViewProj = world * view * proj;
	//XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	//constants.gPulseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//m_constant_po_buf->CopyData(0, constants);

	///* Строим World-матрицу для Пирамиды, вычисляем WVP-матрицу и пишем её в буфер пообъектных констант [1].							 */
	//world = XMMatrixTranslation(1.0f, -1.0f, -1.0f);
	//worldViewProj = world * view * proj;
	//XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	//constants.gPulseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	//m_constant_po_buf->CopyData(1, constants);

	///* Заполняем буфер покадровых констант (он нужен только в иллюстративных целях)														 */
	//m_constant_pf_buf->CopyData(0, timer.TotalTime());
	////m_constant_pf_buf->CopyData(1, 5.0f);

	UpdateObjectCB(timer);
	UpdatePassCB(timer);
}


//! ============================================================   Отрисовка   ============================================================
/*
   Рисуем. */
void D3D12Engine::Draw(const GameTimer &timer)
{
	//Sleep(5);
	auto cmdAllocator = m_currFrame->cmdAllocator;

	/* Ресетим аллокатор текущего фрейм-ресурса																							 */
	ThrowIfFailed(cmdAllocator->Reset());

	if (m_wireFrame) {
		ThrowIfFailed(m_cmdList->Reset(cmdAllocator.Get(), m_pipelineState["opaque_wireframe"].Get()));
	}
	else {
		ThrowIfFailed(m_cmdList->Reset(cmdAllocator.Get(), m_pipelineState["opaque"].Get()));
	}

	/* Устанока вьюпорта и обрезки																										 */
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	/* STATE_PRESENT  ==>  STATE_RENDER_TARGET																							 */
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	/* Очищаем текущий back-буфер и буфер глубин. Подключаем их к Output Merger'у.														 */
	m_cmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::DarkViolet, 0, nullptr);
	m_cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	/*												_____________________________________________________________
	  Топология константных дескрипторов в куче:	| Дескриптор | Дескриптор |	  Дескриптор   |   Дескриптор   |
													| 1 объекта	 | 2 объекта  |	общих констант | общих констант |
													|____(Куб)___|_(Пирамида)_|_______b1_______|_______b2_______|						 */

	/* Устанавливаем кучи дескрипторов и сигнатуру их подключения к слотам шейдеров														 */
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
	m_cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	/* Устанавливаем буферы вершин и индексный буфер, сообщаем топологию отрисовки точек.												 */
	m_cmdList->IASetVertexBuffers(0, 1, m_geometries["primitives"]->VertexBufferViews());
	m_cmdList->IASetIndexBuffer(&m_geometries["primitives"]->IndexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/* Дескриптор для b1 (третий в куче). Устанавливаем его в соответствии с сигнатурой второму root-параметру и больше не меняем.		 */
	/*CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle2(m_CBV_heap->GetGPUDescriptorHandleForHeapStart(), 2, m_CBV_SRV_UAV_descriptorSize);*/
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle2(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), m_passCBVoffset + m_currFrameIndex, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(1, cbvHandle2);

	/* Дескриптор для Куба (первый в куче). Устанавливаем его в соответствии с сигнатурой первому root-параметру и рисуем Куб.			 */
	/*CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_CBV_heap->GetGPUDescriptorHandleForHeapStart());*/
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), m_allRenderItems.size() * m_currFrameIndex, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	
	m_cmdList->DrawIndexedInstanced(m_opaqueRenderItems[0]->indexCount, 1, m_opaqueRenderItems[0]->startIndex, m_opaqueRenderItems[0]->baseVertex, 0);
	//m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["box"].IndexCount, 1, m_boxGeometry->DrawArgs["box"].StartIndexLocation, m_boxGeometry->DrawArgs["box"].BaseVertexLocation, 0);

	/* Дескриптор для Пирамиды (второй в куче, получен смещением первого). Переустанавливаем его первому параметру и рисуем Пирамиду.	 */
	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	m_cmdList->DrawIndexedInstanced(m_opaqueRenderItems[1]->indexCount, 1, m_opaqueRenderItems[1]->startIndex, m_opaqueRenderItems[1]->baseVertex, 0);
	//m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["pyramide"].IndexCount, 1, m_boxGeometry->DrawArgs["pyramide"].StartIndexLocation, m_boxGeometry->DrawArgs["pyramide"].BaseVertexLocation, 0);

	/* STATE_RENDER_TARGET  ==>  STATE_PRESENT																							 */
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	/* Закрываем список команд и отправляем в очередь на выполнение.																	 */
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	/* Свапим back-буферы и ждём окончания выполнения команд.																			 */
	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBuffersCount;


	/* Увеличиваем значение Fence текущего фрейма, кидаем в очередь команд маркер этого значения										 */
	m_currFrame->fenceVal = ++m_currentFence;
	m_cmdQueue->Signal(m_fence.Get(), m_currFrame->fenceVal);
	//FlushCommandQueue();
}




//! =================================================   Обработчик нажатия  кнопки мыши   =================================================
void D3D12Engine::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
	SetCapture(m_hWindow);
}


//! =================================================   Обработчик отжатия  кнопки мыши   =================================================
void D3D12Engine::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}


//! ====================================================   Обработчик движения  мыши   ====================================================
void D3D12Engine::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.5f * static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.5f * static_cast<float>(y - m_lastMousePos.y));

		m_theta += dx;
		m_phi += dy;
		m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.05f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - m_lastMousePos.y);

		m_radius += dx - dy;
		m_radius = MathHelper::Clamp(m_radius, 5.0f, 150.0f);
	}
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}


//! ======================================================   Обработчик клавиатуры   ======================================================
void D3D12Engine::OnKeyboardInput(const GameTimer & timer)
{
	if (GetAsyncKeyState('1') & 0x8000)
		m_wireFrame = true;
	else
		m_wireFrame = false;
}


//! ========================================================   Обновляем  камеру   ========================================================
void D3D12Engine::UpdateCamera(const GameTimer & timer)
{
	m_eyePos.x = m_radius * sinf(m_phi) * cos(m_theta);
	m_eyePos.z = m_radius * sinf(m_phi) * sin(m_theta);
	m_eyePos.y = m_radius * cosf(m_phi);

	/* Строим View-матрицу по полученным данным, сохраняем её. */
	XMVECTOR pos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);
}


//! ==================================================   Обновление объектных констант   ==================================================
/*
   Объектные константы обновляются только если у модели поменялась соотв. константа. В этом случае обновляются буферы для текущего  и  всех
последующих фрэйм-ресурсов.																												 */
void D3D12Engine::UpdateObjectCB(const GameTimer & timer)
{
	auto objectCB = m_currFrame->ObjectCB.get();
	for (auto &e : m_allRenderItems)
	{
		// Обновляем объектные константы (для всех следующих фреймов) только если надо
		if (e->durtyFrames > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->worldMatrix);
			ObjectConstants constant;
			XMStoreFloat4x4(&constant.world, DirectX::XMMatrixTranspose(world));
			objectCB->CopyData(e->objectCB_index, constant);
			e->durtyFrames--;
		}
	}
}


//! =================================================   Обновление  покадровых констант   =================================================
void D3D12Engine::UpdatePassCB(const GameTimer & timer)
{
	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
	XMMATRIX inv_view = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
	XMMATRIX inv_proj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
	XMMATRIX inv_viewProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&m_passConstant.view, DirectX::XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_passConstant.proj, DirectX::XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_passConstant.viewProj, DirectX::XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_passConstant.inv_view, DirectX::XMMatrixTranspose(inv_view));
	XMStoreFloat4x4(&m_passConstant.inv_proj, DirectX::XMMatrixTranspose(inv_proj));
	XMStoreFloat4x4(&m_passConstant.inv_viewProj, DirectX::XMMatrixTranspose(inv_viewProj));

	m_passConstant.renderTargetSize = { (float)m_windowWidth, (float)m_windowHeight };
	m_passConstant.inv_renderTargetSize = { 1.0f / m_windowWidth, 1.0f / m_windowHeight };

	m_passConstant.eyePos_w = m_eyePos;
	m_passConstant.nearZ = 1.0f;
	m_passConstant.farZ = 1000.0f;
	m_passConstant.totalTime = timer.TotalTime();
	m_passConstant.deltaTime = timer.DeltaTime();

	m_currFrame->PassCB->CopyData(0, m_passConstant);
}


//! ======================================================   Куча дескрипторов CBV   ======================================================
/*
   Создаём  кучу  из  4  дескрипторов для константных буферов.  Два  для  пообъектных  констант  (в одном буфере) и 2 для общих (в другом).
Пообъектные дескрипторы подключаются поочерёдно при отрисовке соотв. объекта. Общие - один раз в за кадр.				(стр. 337 - 339) */
void D3D12Engine::CreateCbvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 3;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBV_heap)));
}


//! ======================================================   Куча дескрипторов CBV   ======================================================
void D3D12Engine::CreateCBVdescriptorHeaps()
{
	m_passCBVoffset = m_allRenderItems.size() * g_numFrameResources;	// Сохраняем смещение покадровых дескрипторов

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = (m_allRenderItems.size() + 1) * g_numFrameResources;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}


//! =============================================   Константный буфер и дескрипторы на него   =============================================
/*
   У нас 2 константных буфера. В одном - константы, характерные для каждого объекта сцены. В другом - константы, общие  для  всех объектов.
И те и другие перезаписываются каждый фрейм. На каждую константу привязывается свой дескриптор. Пообъектные дескрипторы подключаются в Draw
поочерёдно, к одному и тому же слоту в шейдере, при отрисоке каждого объекта. Общие - один раз до начала отрисовки всех объектов.
   Здесь вначале создаём буферы констант. Затем - дескрипторы на их элементы.											(стр. 337 - 339) */
void D3D12Engine::BuildConstantBuffers()
{
	/* Создаём константные буферы. Для этого используем UploadBuffer - он просто инстанцирует  в памяти GPU  буфер  и  предоставляет  метод
	CopyData( индекс, данные ). Т.о. можно легко обновить буфер в любое время.															 */
	m_constant_po_buf = std::make_unique<UploadBuffer<Constants>>(m_device.Get(), 2, true);	// Пообъектные константы (по числу объектов)
	m_constant_pf_buf = std::make_unique<UploadBuffer<float>>(m_device.Get(), 1, true);		// Общие

	/* Размеры одного элемента буферов. */
	const UINT cb_po_ElemByteSize = m_constant_po_buf->elementByteSize();
	const UINT cb_pf_ElemByteSize = m_constant_pf_buf->elementByteSize();

	/* Получаем адрес начал буферов в памяти GPU. */
	D3D12_GPU_VIRTUAL_ADDRESS cb_po_Address = m_constant_po_buf->Resource()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS cb_pf_Address = m_constant_pf_buf->Resource()->GetGPUVirtualAddress();

	/* Теперь, зная адреса буферов и размеры их элементов, инициализируем дескрипторы для них. */
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_CBV_heap->GetCPUDescriptorHandleForHeapStart());	// Получаем хандл на первый дескриптор кучи

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cb_po_Address;
	cbvDesc.SizeInBytes = cb_po_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// Заполняем первый дескриптор (это пообъектный)

	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	cbvDesc.BufferLocation += cb_po_ElemByteSize;
	cbvDesc.SizeInBytes = cb_po_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// Заполняем второй дескриптор (это пообъектный)

	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	cbvDesc.BufferLocation = cb_pf_Address;
	cbvDesc.SizeInBytes = cb_pf_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// Заполняем третий дескриптор (это общий)

	//cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	//cbvDesc.BufferLocation += cb_pf_ElemByteSize;
	//cbvDesc.SizeInBytes = cb_pf_ElemByteSize;
	//m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// Заполняем четвёртый дескриптор (это общий)
}


//! =============================================   Мапим  дескрипторы  на  буфера констант   =============================================
/*
   Привязываем дескрипторы кучи к константам для каждого фрейм-ресурса.											(стр. 337 - 339)		 */
void D3D12Engine::Build_ConstantBuffers()
{
	UINT objectCB_elemSize = Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT passCB_elemSize = Util::CalcConstantBufferByteSize(sizeof(PassConstants));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvObjectDesc;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvPassDesc;

	cbvObjectDesc.SizeInBytes = objectCB_elemSize;
	cbvPassDesc.SizeInBytes = passCB_elemSize;

	int i = 0;
	for (; i < g_numFrameResources; ++i)
	{
		D3D12_GPU_VIRTUAL_ADDRESS objectCB_va = m_frameResources[i]->ObjectCB->Resource()->GetGPUVirtualAddress();
		D3D12_GPU_VIRTUAL_ADDRESS passCB_va = m_frameResources[i]->PassCB->Resource()->GetGPUVirtualAddress();
		for (int k = 0; k < m_allRenderItems.size(); ++k)
		{
			auto cbvObjectDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			cbvObjectDescriptorHandle.Offset(k + i * m_allRenderItems.size(), m_CBV_SRV_UAV_descriptorSize);
			cbvObjectDesc.BufferLocation = objectCB_va + k * objectCB_elemSize;
			m_device->CreateConstantBufferView(&cbvObjectDesc, cbvObjectDescriptorHandle);			
		}
		auto cbvPassDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
		cbvPassDescriptorHandle.Offset(m_passCBVoffset + i, m_CBV_SRV_UAV_descriptorSize);
		cbvPassDesc.BufferLocation = passCB_va;
		m_device->CreateConstantBufferView(&cbvPassDesc, cbvPassDescriptorHandle);
	}
}


//! ======================================================   Сигнатура  параметров   ======================================================
/*
   Сигнатура используется для определения подключения дескрипторов к шейдерам. Конкретнее - начиная с  какого  слота  сколько  дескрипторов
подключается и какого они типа. Эту информацию несёт в себе параметр D3D12_ROOT_PARAMETER. Таких параметров может быть много (здесь - 2), и
каждый из них сообщает, какие дескрипторы к каким слотам подключать. D3D12_ROOT_PARAMETER бывает 3 типов - константа, дескриптор и  таблица
дескрипторов. Сейчас мы пока инициализируем 2 root-параметра, как  2  таблицы  дескрипторов.  Таблица  объявляет  подключение  непрерывного
диапазона, длиной L, начиная со слота N.
   На данный момент мы хотим подключить к слоту b0 одну пообъектную константу из буфера *_po_buf, а к слотам b1,b2 - константы из *_pf_buf.
Пообъектные константы подключаются по очереди к слоту b0 при отрисовке соотв. объекта, с  помощью соотв. дескриптора. Но хоть их  и  много,
сигнатура подключения выглядит просто: начиная со слота b0 подключается диапазон из 1 дескриптора. Эту сигнатуру описывает 1 root-параметр.
   Второй root-параметр описывает подключение диапазона из двух общих констант к слотам, начиная с b1.
Объединить  2  root-параметра  в  1  здесь  не получится,  т.к.  они  описывают  подключение непрерывных диапазонов дескрипторов. А собрать
пообъектные дескрипторы (из которых только один может быть подключен) с общими в один диапазон не получится.
   Схематично можно нарисовать это так:
				|-------------------------------|---------------|---------------|
				|              b0				|	   b1		|	   b2		|
				|--------------/\---------------|------/\-------|------/\-------|
						_______||________			   ||			   ||
 						|			    |			   ||              ||        
				|-------|-------|-------|-------|------||-------|------||-------|
				|  Дескриптор 1 |  Дескриптор 2 |  Дескриптор 3 |  Дескриптор 4 |
				|---------------|---------------|---------------|---------------|
					   ||              ||		       ||              ||		 
					   ||			   ||		       ||			   ||		 
				|------\/-------|------\/-------|------\/-------|------\/-------|
				| Константа для | Константа для |     Общая     |     Общая     |
				|   1 объекта   |   2 объекта   |  константа 1  |  константа 2  |
				|---------------|---------------|---------------|---------------|
				|	   Константный буфер 1      |	   Константный буфер 2      |
				|-------------------------------|-------------------------------|

   Далее из этих параметров создаётся root-сигнатура, подключаемая в Draw(). Надо помнить, что она ничего не подключает - только описывает.
Само подключение происходит с помощью SetGraphicsRootDescriptorTable( номер root-параметра, начальный дескриптор ).		(стр. 339 - 344) */
void D3D12Engine::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// Диапазон из 1 дескриптора подключается к шейдерам, начиная с b0
	rootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);	// Диапазон из 1 дескрипторов подключается к шейдерам, начиная с b1
	rootParameter[1].InitAsDescriptorTable(1, &cbvTable2);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializeRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializeRootSig, &errorBlob));

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(m_device->CreateRootSignature(
		0, serializeRootSig->GetBufferPointer(), serializeRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}


//! ===========================================   Загрузка шейдеров,  определение InputLayout   ===========================================
/*
   Загружает байт-код из .cso-прекомпилированных файлов шейдеров. Вершины описываются атрибутами через 2 слота. К одному  будем  подключать
вершинный буфер со значениями "POSITION" и "TEXCOORD", ко второму - "NORMAL", "TANGENT" и "COLOR".			 (стр. 321 - 325, 344 - 353) */
void D3D12Engine::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;
	m_shaders_Vertex["simpleVS_01"] = Util::LoadBinary(L"cso/simpleVS_01.cso");
	m_shaders_Pixel["simplePS_01"] = Util::LoadBinary(L"cso/simplePS_01.cso");
	//m_vsByteCode = Util::LoadBinary(L"cso/VertexShader.cso");
	//m_psByteCode = Util::LoadBinary(L"cso/PixelShader.cso");

	m_inputLayout =
	{
		/* "Семантика" "Индекс"      "Формат атрибута"      "Слот" "Оффсет"				     "?"                       "?" */
		{  "POSITION",    0,    DXGI_FORMAT_R32G32B32_FLOAT,  0,      0,    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{  "TEXCOORD",    0,    DXGI_FORMAT_R32G32_FLOAT,     0,      12,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{  "NORMAL",      0,    DXGI_FORMAT_R32G32B32_FLOAT,  1,      0,    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{  "TANGENT",     0,    DXGI_FORMAT_R32G32B32_FLOAT,  1,      12,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{  "COLOR",       0,    DXGI_FORMAT_B8G8R8A8_UNORM,   0,      12,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}


//! ===================================================   Pipeline State Object (PSO)   ===================================================
/*
	PSO задаёт конвейеру условия работы:
	1. Интерпретация данных из буфера(ов) вершин (InputLayout);
	2. Сигнатура параметров (какие дескрипторы к каким слотам шейдеров подключаются);
	3. Используемые шейдеры (VS, DS, HS, GS, PS);
	4. Настройки растеризации (FILL_MODE, CULL_MODE и т.д.);
	5. Настройки блендера;
	6. Настройки буфера глубин/трафарета;
	7. Маска сэмплирования;
	8. Топология отрисовки точек;
	9. Количество render targets, отрисовываемых одновременно;
	10. Формат RTV;
	11. Формат DSV;
	12. Настройки мультисэмлинга;																						(стр. 353 - 359) */
void D3D12Engine::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc_opaque;
	ZeroMemory(&psoDesc_opaque, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc_opaque.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc_opaque.pRootSignature = m_rootSignature.Get();
	psoDesc_opaque.VS =
	{ 
		reinterpret_cast<BYTE*>(m_shaders_Vertex["simpleVS_01"]->GetBufferPointer()),
		m_shaders_Vertex["simpleVS_01"]->GetBufferSize()
	};
	psoDesc_opaque.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders_Pixel["simplePS_01"]->GetBufferPointer()),
		m_shaders_Pixel["simplePS_01"]->GetBufferSize()
	};
	psoDesc_opaque.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc_opaque.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc_opaque.BlendState = CD3XD12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc_opaque.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc_opaque.SampleMask = UINT_MAX;
	psoDesc_opaque.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc_opaque.NumRenderTargets = 1;
	psoDesc_opaque.RTVFormats[0] = m_backBufferFormat;
	psoDesc_opaque.SampleDesc.Count = m_multisamplingEnabled ? 4 : 1;
	psoDesc_opaque.SampleDesc.Quality = m_multisamplingEnabled ? (m_msQualityLevels - 1) : 0;
	psoDesc_opaque.DSVFormat = m_DS_bufferFormat;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc_opaque, IID_PPV_ARGS(&m_pipelineState["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc_opaque_wireframe = psoDesc_opaque;
	psoDesc_opaque_wireframe.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc_opaque_wireframe, IID_PPV_ARGS(&m_pipelineState["opaque_wireframe"])));
}


//! ========================================================   Строим  геометрию   ========================================================
/*
   В одном и том же буфере вершин находятся вершины для разных объектов. Сейчас атрибуты вершин захардкожены в 2 массивах. В 1-м содержатся
атрибуты: позиция вершины, координата текстуры. Во 2-м вектор нормали, тангента, цвет вершины. Эти буферы подрубаются к конвейеру  через  2
слота. Так же захардкожен буфер индексов.																								 */
void D3D12Engine::BuildBoxGeometry()
{
	/* В показательных целях имеем массив атрибутов для обоих слотов (массив цельных атрибутов), который можно подрубить к одному слоту. */
	const UINT64 numVertices = 13;
	std::array<SeparatedVertex, numVertices> verticesSplittedAttr =
	{
		/*=========1 слот (позиция вершины, кордината текстуры)=========*/  /*================2 слот (вектор нормали, тангента, цвет)================*/
		// Куб
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::White)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,+1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Black)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,+1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Red)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Green)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Blue)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,+1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Yellow)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,+1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Cyan)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Magenta)}}),
		// Пирамида
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::White)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Black)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Red)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Green)}}),
		SeparatedVertex({{XMFLOAT3( 0.0f,+2.0f, 0.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Blue)}}),
	};
	std::array<std::uint16_t, 54> indices =
	{
		// Куб
		0, 1, 2, 0, 2, 3,	// Front
		4, 6, 5, 4, 7, 6,	// Back
		4, 5, 1, 4, 1, 0,	// Left
		3, 2, 6, 3, 6, 7,	// Right
		1, 5, 6, 1, 6, 2,	// Top
		4, 0, 3, 4, 3, 7,	// Bottom
		// Пирамида
		1, 3, 0, 1, 2, 3,	// Bottom
		0, 4, 1,			// Front
		1, 4, 2,			// Right
		2, 4, 3,			// Back
		3, 4, 0				// Left
	};

	/* Из массива цельных атрибутов формируем 2 массива с атрибутами. В 1 - позиция вершины, координата текстуры.  Во 2-м - вектор нормали,
	тангента, цвет вершины.																												 */
	std::array<SeparatedVertex::PosTex, numVertices> verticesPT;
	std::array<SeparatedVertex::NorTanCol, numVertices> verticesNTC;

	for (int i = 0; i < numVertices; ++i)
	{
		verticesPT[i] = verticesSplittedAttr[i].pos_tex;
		verticesNTC[i] = verticesSplittedAttr[i].nor_tan_col;
	}
	const UINT iByteSize = (UINT)indices.size() * sizeof(std::uint16_t);			// Размер индексного буфера
	const UINT vByteSizePT = numVertices * sizeof(SeparatedVertex::PosTex);			// Размер буффера, подключаемого к слоту 1
	const UINT vByteSizeNTC = numVertices * sizeof(SeparatedVertex::NorTanCol);		// Размер буффера, подключаемого к слоту 2

	// Описываем геометрию меделей
	m_boxGeometry = std::make_unique<MeshGeometry<2>>();
	m_boxGeometry->Name = "Box";

	/* Используется при вычислении VertexBufferView */
	m_boxGeometry->vbMetrics[0].VertexBufferByteSize = vByteSizePT;					// ByteSize и Stride буфера вершин для 1 слота
	m_boxGeometry->vbMetrics[0].VertexByteStride = sizeof(SeparatedVertex::PosTex);
	m_boxGeometry->vbMetrics[1].VertexBufferByteSize = vByteSizeNTC;				// ByteSize и Stride буфера вершин для 2 слота
	m_boxGeometry->vbMetrics[1].VertexByteStride = sizeof(SeparatedVertex::NorTanCol);

	m_boxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_boxGeometry->IndexBufferByteSize = iByteSize;									// ByteSize индексного буфера

	// Сохраняем буферы вершин и индексов в памяти на стороне CPU
	ThrowIfFailed(D3DCreateBlob(vByteSizePT, &m_boxGeometry->VBufferCPU[0]));		// Резервируем место в памяти
	ThrowIfFailed(D3DCreateBlob(vByteSizeNTC, &m_boxGeometry->VBufferCPU[1]));
	ThrowIfFailed(D3DCreateBlob(iByteSize, &m_boxGeometry->IndexBufferCPU));

	CopyMemory(m_boxGeometry->VBufferCPU[0]->GetBufferPointer(), verticesPT.data(), vByteSizePT);	// Заполняем его данными
	CopyMemory(m_boxGeometry->VBufferCPU[1]->GetBufferPointer(), verticesNTC.data(), vByteSizeNTC);
	CopyMemory(m_boxGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);

	/* Создаём в GPU буферы вершин (по количеству слотов) и индексный буфер. Копируем туда данные из соответствующих массивов.			 */
	m_boxGeometry->VBufferGPU[0] = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), verticesPT.data(), vByteSizePT, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->VBufferGPU[1] = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), verticesNTC.data(), vByteSizeNTC, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->IndexBufferGPU =
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), indices.data(), iByteSize, m_boxGeometry->IndexBufferUploader);

	/* Описываем топологию объектов в буферах вершин и индексном буфере */
	SubmeshGeometry submesh;
	submesh.IndexCount = 36;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	m_boxGeometry->DrawArgs["box"] = submesh;

	submesh.IndexCount = 18;
	submesh.StartIndexLocation = 36;
	submesh.BaseVertexLocation = 8;
	m_boxGeometry->DrawArgs["pyramide"] = submesh;
}


//! ========================================================   Строим  геометрию   ========================================================
/*
   Вся геометрия хранится MeshGeometry.  Это буфера вершин и индексов, сохраняемые и со стороны CPU, и GPU;  это дескрипторы на эти буфера;
это топология отдельных моделей в этих буферах. */
void D3D12Engine::BuildGeometry()
{
	const UINT64 numVertices = 13;

	std::array<Vertex, numVertices> vertices =
	{
		/*===========(позиция вершины)==============(цвет)==========*/
		// Куб
		Vertex(XMFLOAT3(-1.0f,-1.0f,-1.0f), XMCOLOR(Colors::Red)),
		Vertex(XMFLOAT3(-1.0f,+1.0f,-1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(+1.0f,+1.0f,-1.0f), XMCOLOR(Colors::White)),
		Vertex(XMFLOAT3(+1.0f,-1.0f,-1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(-1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Red)),
		Vertex(XMFLOAT3(-1.0f,+1.0f,+1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(+1.0f,+1.0f,+1.0f), XMCOLOR(Colors::Red)),
		Vertex(XMFLOAT3(+1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Green)),
		// Пирамида
		Vertex(XMFLOAT3(-1.0f,-1.0f,-1.0f), XMCOLOR(Colors::Red)),
		Vertex(XMFLOAT3(+1.0f,-1.0f,-1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(+1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Red)),
		Vertex(XMFLOAT3(-1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(0.0f,+2.0f, 0.0f), XMCOLOR(Colors::White)),
	};

	std::array<std::uint16_t, 54> indices =
	{
		// Куб
		/*=== Front ======|====== Back =======|====== Left =======|====== Right ======|======= Top =======|==== Bottom ====*/
		0, 1, 2, 0, 2, 3,   4, 6, 5, 4, 7, 6,   4, 5, 1, 4, 1, 0,   3, 2, 6, 3, 6, 7,   1, 5, 6, 1, 6, 2,   4, 0, 3, 4, 3, 7,
		// Пирамида
		/*=== Bottom =====|= Front ==|= Right ==|= Back ===|= Left*/
		1, 3, 0, 1, 2, 3,	0, 4, 1,   1, 4, 2,   2, 4, 3,   3, 4, 0
	};

	UINT vByteSize = vertices.size() * sizeof(Vertex);
	UINT iByteSize = indices.size() * sizeof(std::uint16_t);

	/* Строим геометрию */
	std::unique_ptr<MeshGeometry<1>> shapes = std::make_unique<MeshGeometry<1>>();
	shapes->Name = "primitives";
	
	shapes->vbMetrics->VertexBufferByteSize = vByteSize;
	shapes->vbMetrics->VertexByteStride = sizeof(Vertex);
	shapes->IndexBufferByteSize = iByteSize;
	shapes->IndexFormat = DXGI_FORMAT_R16_UINT;

	/* Сохраняем геометрию на стороне CPU */
	ThrowIfFailed(D3DCreateBlob(vByteSize, &shapes->VBufferCPU[0]));
	ThrowIfFailed(D3DCreateBlob(iByteSize, &shapes->IndexBufferCPU));
	CopyMemory(shapes->VBufferCPU[0]->GetBufferPointer(), vertices.data(), vByteSize);
	CopyMemory(shapes->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);

	/* Строим буферы в GPU, сохраняем в них геометрию */
	shapes->VBufferGPU[0] = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), vertices.data(), vByteSize, shapes->VertexBufferUploader);
	shapes->IndexBufferGPU =
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), indices.data(), iByteSize, shapes->IndexBufferUploader);

	/* Расположение моделей в буферах */
	SubmeshGeometry submesh;
	submesh.IndexCount = 36;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	shapes->DrawArgs["box"] = submesh;

	submesh.IndexCount = 18;
	submesh.StartIndexLocation = 36;
	submesh.BaseVertexLocation = 8;
	shapes->DrawArgs["pyramide"] = submesh;

	m_geometries[shapes->Name] = std::move(shapes);
	return;
}


//! ========================================================   Формируем  модели   ========================================================
/*
   Вся геометрия была уже сформирована в BuildGeometry(). Модель хранит адрес куска соответствующей геометрии, указатель на общую геометрию
и характерную для каждого объекта матрицу World. И ещё индекс константы буфера констант, куда она эту матрицу запишет при изменении. Буфера
констант (пообъектных и покадровых) хранятся в циклическом массиве FrameResources (для 3 фреймов).						(стр. 394 - 396) */
void D3D12Engine::BuildRenderItems()
{
	auto boxItem = std::make_unique<RenderItem>(g_numFrameResources);
	auto pyramideItem = std::make_unique<RenderItem>(g_numFrameResources);

	XMStoreFloat4x4(&boxItem->worldMatrix, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(3.0f, 0.0f, 0.0f));
	boxItem->geometry = m_geometries["primitives"].get();
	boxItem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxItem->indexCount = boxItem->geometry->DrawArgs["box"].IndexCount;
	boxItem->startIndex = boxItem->geometry->DrawArgs["box"].StartIndexLocation;
	boxItem->baseVertex = boxItem->geometry->DrawArgs["box"].BaseVertexLocation;
	boxItem->objectCB_index = 0;
	m_allRenderItems.push_back(std::move(boxItem));

	XMStoreFloat4x4(&pyramideItem->worldMatrix, DirectX::XMMatrixTranslation(-3.0f, 2.0f, 0.0f));
	pyramideItem->geometry = m_geometries["primitives"].get();
	pyramideItem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	pyramideItem->indexCount = pyramideItem->geometry->DrawArgs["pyramide"].IndexCount;
	pyramideItem->startIndex = pyramideItem->geometry->DrawArgs["pyramide"].StartIndexLocation;
	pyramideItem->baseVertex = pyramideItem->geometry->DrawArgs["pyramide"].BaseVertexLocation;
	pyramideItem->objectCB_index = 1;
	m_allRenderItems.push_back(std::move(pyramideItem));

	/* Все модели непрозрачные */
	for (auto &item : m_allRenderItems)
		m_opaqueRenderItems.push_back(item.get());

}


//! ==========================================================   Фрэйм-ресурсы   ==========================================================
/*
   Формируем циклический массив фрэйм-ресурсов, используемый для эффективной загрузки GPU. В то время, как  GPU выполняет список команд для
кадра N, используя при этом константы для этого кадра, CPU может не ждать и готовить константы и список команд для N+1 кадра.  Массив  на 3
элемента обеспечивает GPU данными на 3 кадра вперёд, если CPU работает быстрее.	
   Здесь каждый фрэйм ресурс имеет 2 буфера констант: пообъектных и покадровых. И те и другие валидны в течение  только одного  кадра, т.о.
имеем в памяти GPU 6 константных буферов.																				(стр. 389 - 394) */
void D3D12Engine::BuildFrameResources()
{
	for (int i = 0; i < g_numFrameResources; ++i)
	{
		m_frameResources.push_back(std::make_unique<FrameResources>(m_device.Get(), 1, m_allRenderItems.size()));
	}
}
