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
	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	XMMATRIX worldViewProj = world * view * proj;

	Constants constants;
	XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	m_constantBuffer->CopyData(0, constants);
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

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_heap.Get() };
	m_cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	m_cmdList->IASetVertexBuffers(0, 1, &m_boxGeometry->VertexBufferView());
	m_cmdList->IASetIndexBuffer(&m_boxGeometry->IndexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_cmdList->SetGraphicsRootDescriptorTable(0, m_CBV_heap->GetGPUDescriptorHandleForHeapStart());

	m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

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
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBV_heap)));
}

void D3D12Engine::BuildConstantBuffers()
{
	m_constantBuffer = std::make_unique<UploadBuffer<Constants>>(m_device.Get(), 1, true);
	const UINT cbElemByteSize = Util::CalcConstantBufferByteSize(sizeof(Constants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_constantBuffer->Resource()->GetGPUVirtualAddress();
	int boxIndexCB = 0;
	cbAddress += boxIndexCB * cbElemByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = Util::CalcConstantBufferByteSize(sizeof(Constants));;

	m_device->CreateConstantBufferView(&cbvDesc, m_CBV_heap->GetCPUDescriptorHandleForHeapStart());
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
	//m_vsByteCode = Util::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	//m_psByteCode = Util::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 }
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
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};
	std::array<std::uint16_t, 36> indices =
	{
		0, 1, 2, 0, 2, 3,	// Front
		4, 6, 5, 4, 7, 6,	// Back
		4, 5, 1, 4, 1, 0,	// Left
		3, 2, 6, 3, 6, 7,	// Right
		1, 5, 6, 1, 6, 2,	// Top
		4, 0, 3, 4, 3, 7	// Bottom
	};
	const UINT vByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT iByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_boxGeometry = std::make_unique<MeshGeometry>();
	m_boxGeometry->Name = "Box";

	ThrowIfFailed(D3DCreateBlob(vByteSize, &m_boxGeometry->VertexBufferCPU));
	CopyMemory(m_boxGeometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vByteSize);

	ThrowIfFailed(D3DCreateBlob(iByteSize, &m_boxGeometry->IndexBufferCPU));
	CopyMemory(m_boxGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);

	m_boxGeometry->VertexBufferGPU = Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), vertices.data(), vByteSize, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->IndexBufferGPU = Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), indices.data(), iByteSize, m_boxGeometry->IndexBufferUploader);

	m_boxGeometry->VertexByteStride = sizeof(Vertex);
	m_boxGeometry->VertexBufferByteSize = vByteSize;
	m_boxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_boxGeometry->IndexBufferByteSize = iByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_boxGeometry->DrawArgs["box"] = submesh;
}
