#include "stdafx.h"
#include "D3D12Engine.h"


D3D12Engine::D3D12Engine(HINSTANCE hInstance) : D3D12Base(hInstance)
{
}


D3D12Engine::~D3D12Engine()
{
}


bool D3D12Engine::Initialize()
{
	if (!D3D12Base::Initialize())
		return false;

	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildPSO();
	BuildBoxGeometry();
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList *cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	FlushCommandQueue();
	return true;
}

void D3D12Engine::OnResize()
{
	D3D12Base::OnResize();
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

void D3D12Engine::Update(const GameTimer & timer)
{
	float x = m_radius * sinf(m_phi) * cos(m_theta);
	float z = m_radius * sinf(m_phi) * sin(m_theta);
	float y = m_radius * cosf(m_phi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);
	XMMATRIX world = XMMatrixTranslation(-1.0f, -1.0f, -1.0f);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	XMMATRIX worldViewProj = world * view * proj;

	// Заполняем элементы константного буфера константами для каждого объекта.
	Constants constants;
	XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	constants.gPulseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	constants.gTime = timer.TotalTime()*0;
	m_constantBuffer->CopyData(0, constants);

	world = XMMatrixTranslation(1.0f, -1.0f, -1.0f);
	worldViewProj = world * view * proj;
	XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	constants.gPulseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	constants.gTime = timer.TotalTime()*0;
	m_constantBuffer->CopyData(1, constants);
}

void D3D12Engine::Draw(const GameTimer & timer)
{
	ThrowIfFailed(m_cmdAllocator->Reset());
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), m_pipelineState.Get()));

	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_cmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::DarkViolet, 0, nullptr);
	m_cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_heap.Get() };		// Сейчас в куче 2 дескриптора для куба и пирамиды
	m_cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	m_cmdList->IASetVertexBuffers(0, 2, m_boxGeometry->VertexBufferViews());

	m_cmdList->IASetIndexBuffer(&m_boxGeometry->IndexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Получаем Handle для работы с кучей дескрипторов константного буфера (число дескрипторов = число элементов буфера). Сейчас он
	// указывает на первый элемент буфера констант. Каждый раз при смене дескриптора нужно вызывать SetGraphicsRootDescriptorTable().
	// Сами дескрипторы инициализируются (получают адрес/размер области буфера, на которую указывают) в BuildConstantBuffers().
	// Константный буфер создаётся там же (просто выделяется место в памяти GPU), но элементы инитятся в Update().
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_CBV_heap->GetGPUDescriptorHandleForHeapStart());
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

	// Рисуем первый меш, используя первый дескриптор
	m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["box"].IndexCount, 1, m_boxGeometry->DrawArgs["box"].StartIndexLocation, m_boxGeometry->DrawArgs["box"].BaseVertexLocation, 0);

	// Получаем второй дескриптор из кучи, сдвигая Handle. Он указывает на второй элемент буфера констант. Рисуем второй меш, установив
	// новый дескриптор (SetGraphicsRootDescriptorTable()).
	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["pyramide"].IndexCount, 1, m_boxGeometry->DrawArgs["pyramide"].StartIndexLocation, m_boxGeometry->DrawArgs["pyramide"].BaseVertexLocation, 0);

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(m_cmdList->Close());

	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBuffersCount;

	FlushCommandQueue();
}

void D3D12Engine::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
	SetCapture(m_hWindow);
}

void D3D12Engine::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3D12Engine::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

		m_theta += dx;
		m_phi += dy;
		m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.005f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_lastMousePos.y);

		m_radius += dx - dy;
		m_radius = MathHelper::Clamp(m_radius, 3.0f, 15.0f);
	}
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void D3D12Engine::BuildDescriptorHeaps()
{
	// Создаём кучу из 2 дескрипторов для константного буфера. Каждый дескриптор будет использован для отрисовки соотв. объекта сцены.
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 2;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBV_heap)));
}

void D3D12Engine::BuildConstantBuffers()
{
	// Создаём буфер констант, состоящий из 2 элементов
	m_constantBuffer = std::make_unique<UploadBuffer<Constants>>(m_device.Get(), 2, true);
	const UINT cbElemByteSize = Util::CalcConstantBufferByteSize(sizeof(Constants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_constantBuffer->Resource()->GetGPUVirtualAddress();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = cbElemByteSize;

	// Создаём Handle, указывающий на первый дескриптор кучи CBV. Обратить внимание - это "CPU"_DESCRIPTOR_HANDLE. С его помощью пишем в
	// первый дескриптор инфу, на какую область ему указывать и какого она размера. Эта инфа берётся из D3D12_CONSTANT_BUFFER_VIEW_DESC.
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_CBV_heap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	// Заполняем второй дескриптор. Для этого пишем в D3D12_CONSTANT_BUFFER_VIEW_DESC новый адрес, куда указывать, и получаем Handle на
	// второй дескриптор в куче (просто сдвигая вперёд ранее полученный на размер дескриптора)
	cbvDesc.BufferLocation += cbElemByteSize;
	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);
}

void D3D12Engine::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[1];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	rootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializeRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializeRootSig.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(m_device->CreateRootSignature(0, serializeRootSig->GetBufferPointer(), serializeRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void D3D12Engine::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;
	m_vsByteCode = Util::LoadBinary(L"cso/VertexShader.cso");
	m_psByteCode = Util::LoadBinary(L"cso/PixelShader.cso");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
	};
}

void D3D12Engine::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(m_vsByteCode->GetBufferPointer()), 
		m_vsByteCode->GetBufferSize() 
	};
	psoDesc.PS = 
	{
		reinterpret_cast<BYTE*>(m_psByteCode->GetBufferPointer()),
		m_psByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3XD12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_backBufferFormat;
	psoDesc.SampleDesc.Count = m_multisamplingEnabled ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_multisamplingEnabled ? (m_msQualityLevels - 1) : 0;
	psoDesc.DSVFormat = m_DS_bufferFormat;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void D3D12Engine::BuildBoxGeometry()
{
	const UINT32 vertexNumber = 8;
	std::array<SeparatedVertex, 13> verticesSplittedAttr =
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

	std::array<SeparatedVertex::PosTex, 13> verticesPosTex;
	std::array<SeparatedVertex::NorTanCol, 13> verticesNorTanCol;

	for (int i = 0; i < verticesSplittedAttr.size(); ++i) {
		verticesPosTex[i] = verticesSplittedAttr[i].pos_tex;
		verticesNorTanCol[i] = verticesSplittedAttr[i].nor_tan_col;
	}

	const UINT iByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_boxGeometry = std::make_unique<MeshGeometry<2>>();
	m_boxGeometry->Name = "Box";

	const UINT vByteSizeSlot1 = verticesPosTex.size() * sizeof(SeparatedVertex::PosTex);		// Размер буффера, подключаемого к слоту 1
	const UINT vByteSizeSlot2 = verticesNorTanCol.size() * sizeof(SeparatedVertex::NorTanCol);	// Размер буффера, подключаемого к слоту 2

	// Описываем буферы вершин (Stride, Size). Эти данные используются для генерации вьюх на буферы при подключении их к GPU-pipeline.
	m_boxGeometry->vbMetrics[0].VertexBufferByteSize = vByteSizeSlot1;
	m_boxGeometry->vbMetrics[0].VertexByteStride = sizeof(SeparatedVertex::PosTex);
	m_boxGeometry->vbMetrics[1].VertexBufferByteSize = vByteSizeSlot2;
	m_boxGeometry->vbMetrics[1].VertexByteStride = sizeof(SeparatedVertex::NorTanCol);

	// Сохраняем буферы вершин в памяти на стороне CPU
	ThrowIfFailed(D3DCreateBlob(vByteSizeSlot1, &m_boxGeometry->VBufferCPU[0]));
	ThrowIfFailed(D3DCreateBlob(vByteSizeSlot2, &m_boxGeometry->VBufferCPU[1]));
	CopyMemory(m_boxGeometry->VBufferCPU[0]->GetBufferPointer(), verticesPosTex.data(), vByteSizeSlot1);
	CopyMemory(m_boxGeometry->VBufferCPU[1]->GetBufferPointer(), verticesNorTanCol.data(), vByteSizeSlot2);

	ThrowIfFailed(D3DCreateBlob(iByteSize, &m_boxGeometry->IndexBufferCPU));
	CopyMemory(m_boxGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);

	// Создаём в GPU буферы вершин (сколько слотов - столько буферов) и индексный буфер. Копируем туда данные из соответствующих массивов
	m_boxGeometry->VBufferGPU[0] = Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), verticesPosTex.data(), vByteSizeSlot1, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->VBufferGPU[1] = Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), verticesNorTanCol.data(), vByteSizeSlot2, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->IndexBufferGPU = Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), indices.data(), iByteSize, m_boxGeometry->IndexBufferUploader);

	// Описываем буфер индексов. Эти данные используются для генерации вьюх на индексный буфер при подключении их к GPU-pipeline.
	m_boxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_boxGeometry->IndexBufferByteSize = iByteSize;

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
