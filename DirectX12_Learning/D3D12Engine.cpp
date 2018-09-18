#include "stdafx.h"

#include "D3D12Engine.h"
#include "Definitions/Vertexes_defs.h"
#include "Helpers/GeometryGenerator.h"
#include "Exceptions/DxException.h"
#include "FrameResources.h"


const UINT g_numFrameResources = 3;


//! ============================================================  �����������  ============================================================
D3D12Engine::D3D12Engine(HINSTANCE hInstance) : D3D12Base(hInstance) {}


//! ===========================================================    ����������   ===========================================================
D3D12Engine::~D3D12Engine() {}


//! ==========================================================   ��������������   =========================================================
bool D3D12Engine::Initialize()
{
	if (!D3D12Base::Initialize())
		return false;

	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	CreateCBVdescriptorHeaps();
	BuildConstantBuffers();
	BuildPSO();
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList *cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	FlushCommandQueue();
	return true;
}


//! =====================================================   ���������� Resize Event   =====================================================
void D3D12Engine::OnResize()
{
	D3D12Base::OnResize();
	XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}


//! =================================================   ���������� �������  ������ ����   =================================================
void D3D12Engine::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
	SetCapture(m_hWindow);
}


//! =================================================   ���������� �������  ������ ����   =================================================
void D3D12Engine::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}


//! ====================================================   ���������� ��������  ����   ====================================================
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


//! ======================================================   ���������� ����������   ======================================================
void D3D12Engine::OnKeyboardInput(const GameTimer & timer)
{
	if (GetAsyncKeyState('1') & 0x8000)
		m_wireFrame = true;
	else
		m_wireFrame = false;
}


//! =============================================================   Update   ==============================================================
/*
   ����� ������ ���� ������ ������� �������� � ����� �� � ����������� ������.															 */
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

	UpdateObjectCB(timer);
	UpdatePassCB(timer);
}


//! ========================================================   ���������  ������   ========================================================
void D3D12Engine::UpdateCamera(const GameTimer & timer)
{
	m_eyePos.x = m_radius * sinf(m_phi) * cos(m_theta);
	m_eyePos.z = m_radius * sinf(m_phi) * sin(m_theta);
	m_eyePos.y = m_radius * cosf(m_phi);

	/* ������ View-������� �� ���������� ������, ��������� �. */
	XMVECTOR pos = XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);
}


//! ==================================================   ���������� ��������� ��������   ==================================================
/*
   ��������� ��������� ����������� ������ ���� � ������ ���������� �����. ���������. � ���� ������ ����������� ������ ��� ��������  �  ����
����������� �����-��������.																												 */
void D3D12Engine::UpdateObjectCB(const GameTimer & timer)
{
	auto objectCB = m_currFrame->ObjectCB.get();
	for (auto &e : m_allRenderItems)
	{
		// ��������� ��������� ��������� (��� ���� ��������� �������) ������ ���� ����
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


//! =================================================   ����������  ���������� ��������   =================================================
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


//! ============================================================   ���������   ============================================================
/*
   ������. */
void D3D12Engine::Draw(const GameTimer &timer)
{
	auto cmdAllocator = m_currFrame->cmdAllocator;

	/* ������� ��������� �������� �����-�������																							 */
	ThrowIfFailed(cmdAllocator->Reset());

	if (m_wireFrame) {
		ThrowIfFailed(m_cmdList->Reset(cmdAllocator.Get(), m_pipelineState["opaque_wireframe"].Get()));
	}
	else {
		ThrowIfFailed(m_cmdList->Reset(cmdAllocator.Get(), m_pipelineState["opaque"].Get()));
	}

	/* �������� �������� � �������																										 */
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	/* STATE_PRESENT  ==>  STATE_RENDER_TARGET																							 */
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	/* ������� ������� back-����� � ����� ������. ���������� �� � Output Merger'�.														 */
	m_cmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, nullptr);
	m_cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());


	/* ������������� ���� ������������ � ��������� �� ����������� � ������ ��������														 */
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
	m_cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	/* ������������� ������ ������ � ��������� �����, �������� ��������� ��������� �����.												 */
	m_cmdList->IASetVertexBuffers(0, 1, &m_geometries["primitives"]->VertexBufferViews());
	m_cmdList->IASetIndexBuffer(&m_geometries["primitives"]->IndexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/* ���������� ��� b1. ������������� ��� � ������������ � ���������� ������� root-��������� � ������ �� ������.		 */
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle2(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), m_passCBVoffset + m_currFrameIndex, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(1, cbvHandle2);

	/* ���������� ��� ����. ������������� ��� � ������������ � ���������� ������� root-��������� � ������ ���.			 */
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), m_allRenderItems.size() * m_currFrameIndex, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	m_cmdList->DrawIndexedInstanced(m_opaqueRenderItems[0]->indexCount, 1, m_opaqueRenderItems[0]->startIndex, m_opaqueRenderItems[0]->baseVertex, 0);

	/* ���������� ��� ��������. ����������������� ��� ������� ��������� � ������ ��������.	 */
	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	m_cmdList->DrawIndexedInstanced(m_opaqueRenderItems[1]->indexCount, 1, m_opaqueRenderItems[1]->startIndex, m_opaqueRenderItems[1]->baseVertex, 0);

	/* STATE_RENDER_TARGET  ==>  STATE_PRESENT																							 */
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	/* ��������� ������ ������ � ���������� � ������� �� ����������.																	 */
	ThrowIfFailed(m_cmdList->Close());
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	/* ������ back-������ � ��� ��������� ���������� ������.																			 */
	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBuffersCount;


	/* ����������� �������� Fence �������� ������, ������ � ������� ������ ������ ����� ��������										 */
	m_currFrame->fenceVal = ++m_currentFence;
	m_cmdQueue->Signal(m_fence.Get(), m_currFrame->fenceVal);
}


//! ======================================================   ���������  ����������   ======================================================
/*
��������� ������������ ��� ����������� ����������� ������������ � ��������. ���������� - ������� �  ������  �����  �������  ������������
������������ � ������ ��� ����. ��� ���������� ���� � ���� �������� D3D12_ROOT_PARAMETER. ����� ���������� ����� ���� ����� (����� - 2), �
������ �� ��� ��������, ����� ����������� � ����� ������ ����������. D3D12_ROOT_PARAMETER ������ 3 ����� - ���������, ���������� �  �������
������������. ������ �� ���� �������������� 2 root-���������, ���  2  �������  ������������.  �������  ���������  �����������  ������������
���������, ������ L, ������� �� ����� N.
�� ������ ������ �� ����� ���������� � ����� b0 ���� ����������� ��������� �� ������ *_po_buf, � � ������ b1,b2 - ��������� �� *_pf_buf.
����������� ��������� ������������ �� ������� � ����� b0 ��� ��������� �����. �������, �  ������� �����. �����������. �� ���� ��  �  �����,
��������� ����������� �������� ������: ������� �� ����� b0 ������������ �������� �� 1 �����������. ��� ��������� ��������� 1 root-��������.
������ root-�������� ��������� ����������� ��������� �� ���� ����� �������� � ������, ������� � b1.
����������  2  root-���������  �  1  �����  �� ���������,  �.�.  ���  ���������  ����������� ����������� ���������� ������������. � �������
����������� ����������� (�� ������� ������ ���� ����� ���� ���������) � ������ � ���� �������� �� ���������.
���������� ����� ���������� ��� ���:
|-------------------------------|---------------|---------------|
|              b0				|	   b1		|	   b2		|
|--------------/\---------------|------/\-------|------/\-------|
_______||________			   ||			   ||
|			    |			   ||              ||
|-------|-------|-------|-------|------||-------|------||-------|
|  ���������� 1 |  ���������� 2 |  ���������� 3 |  ���������� 4 |
|---------------|---------------|---------------|---------------|
||              ||		       ||              ||
||			   ||		       ||			   ||
|------\/-------|------\/-------|------\/-------|------\/-------|
| ��������� ��� | ��������� ��� |     �����     |     �����     |
|   1 �������   |   2 �������   |  ��������� 1  |  ��������� 2  |
|---------------|---------------|---------------|---------------|
|	   ����������� ����� 1      |	   ����������� ����� 2      |
|-------------------------------|-------------------------------|

����� �� ���� ���������� �������� root-���������, ������������ � Draw(). ���� �������, ��� ��� ������ �� ���������� - ������ ���������.
���� ����������� ���������� � ������� SetGraphicsRootDescriptorTable( ����� root-���������, ��������� ���������� ).		(���. 339 - 344) */
void D3D12Engine::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// �������� �� 1 ����������� ������������ � ��������, ������� � b0
	rootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);	// �������� �� 1 ������������ ������������ � ��������, ������� � b1
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


//! ======================================================   ���� ������������ CBV   ======================================================
/*
   ������ ���� ������������ ��� ����������� �������. ����������� ����������� ������������ ��������� ��� ��������� �����. �������. ����� -
���� ��� � �� ����.																										(���. 337 - 339) */
void D3D12Engine::CreateCBVdescriptorHeaps()
{
	m_passCBVoffset = m_allRenderItems.size() * g_numFrameResources;	// ��������� �������� ���������� ������������

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = (m_allRenderItems.size() + 1) * g_numFrameResources;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}


//! =============================================   �����  �����������  ��  ������ ��������   =============================================
/*
   ����������� ����������� ���� � ���������� ��� ������� �����-�������.											(���. 337 - 339)		 */
void D3D12Engine::BuildConstantBuffers()
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


//! ===========================================   �������� ��������,  ����������� InputLayout   ===========================================
/*
   ��������� ����-��� �� .cso-������������������ ������ ��������. ������� ����������� ���������� ����� 2 �����. � ������  �����  ����������
��������� ����� �� ���������� "POSITION" � "TEXCOORD", �� ������� - "NORMAL", "TANGENT" � "COLOR".			 (���. 321 - 325, 344 - 353) */
void D3D12Engine::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;
	m_shaders_Vertex["simpleVS_01"] = Util::LoadBinary(L"cso/simpleVS_01.cso");
	m_shaders_Pixel["simplePS_01"] = Util::LoadBinary(L"cso/simplePS_01.cso");

	m_inputLayout =
	{
		/* "���������" "������"      "������ ��������"      "����" "������"				     "?"                       "?" */
		{  "POSITION",    0,    DXGI_FORMAT_R32G32B32_FLOAT,  0,      0,    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{  "COLOR",       0,    DXGI_FORMAT_B8G8R8A8_UNORM,   0,      12,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}


//! ===================================================   Pipeline State Object (PSO)   ===================================================
/*
	PSO ����� ��������� ������� ������:
	1. ������������� ������ �� ������(��) ������ (InputLayout);
	2. ��������� ���������� (����� ����������� � ����� ������ �������� ������������);
	3. ������������ ������� (VS, DS, HS, GS, PS);
	4. ��������� ������������ (FILL_MODE, CULL_MODE � �.�.);
	5. ��������� ��������;
	6. ��������� ������ ������/���������;
	7. ����� �������������;
	8. ��������� ��������� �����;
	9. ���������� render targets, �������������� ������������;
	10. ������ RTV;
	11. ������ DSV;
	12. ��������� ��������������;																						(���. 353 - 359) */
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


//! ========================================================   ������  ���������   ========================================================
/*
   ��� ��������� �������� MeshGeometry.  ��� ������ ������ � ��������, ����������� � �� ������� CPU, � GPU;  ��� ����������� �� ��� ������;
��� ��������� ��������� ������� � ���� �������. */
void D3D12Engine::BuildGeometry()
{
	// ������� ����
	GeometryGenerator shapeCreator;
	GeometryGenerator::MeshData cylinder = shapeCreator.CreateCylinder(0.3f, 0.3f, 1.0f, 40, 25);

	UINT cylinderVertexOffset = 0;

	UINT cylinderIndexOffset = 0;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT32)cylinder.indices32.size();
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;

	auto totalVerticesCount = cylinder.vertices.size();

	std::vector<Vertex> Vertices(totalVerticesCount);
	std::vector<std::uint16_t> Indices;

	UINT32 k = 0;
	for (size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
	{
		Vertices[k].Position = cylinder.vertices[i].position;
		Vertices[k].Color = XMCOLOR(DirectX::Colors::DarkGreen);
	}

	Indices.insert(Indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	const UINT vByteSize = (UINT)Vertices.size() * sizeof(Vertex);
	const UINT iByteSize = (UINT)Indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> shapes = std::make_unique<MeshGeometry>();
	shapes->Name = "primitives";
	shapes->vbMetrics.VertexBufferByteSize = vByteSize;
	shapes->vbMetrics.VertexByteStride = sizeof(Vertex);
	shapes->IndexBufferByteSize = iByteSize;
	shapes->IndexFormat = DXGI_FORMAT_R16_UINT;

	/* ��������� ��������� �� ������� CPU */
	ThrowIfFailed(D3DCreateBlob(vByteSize, &shapes->VBufferCPU));
	ThrowIfFailed(D3DCreateBlob(iByteSize, &shapes->IndexBufferCPU));
	CopyMemory(shapes->VBufferCPU->GetBufferPointer(), Vertices.data(), vByteSize);
	CopyMemory(shapes->IndexBufferCPU->GetBufferPointer(), Indices.data(), iByteSize);

	/* ������ ������ � GPU, ��������� � ��� ��������� */
	shapes->VBufferGPU = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), Vertices.data(), vByteSize, shapes->VertexBufferUploader);
	shapes->IndexBufferGPU =
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), Indices.data(), iByteSize, shapes->IndexBufferUploader);

	shapes->DrawArgs["cylinder"] = cylinderSubmesh;

	m_geometries[shapes->Name] = std::move(shapes);
}


//! ========================================================   ���������  ������   ========================================================
/*
   ��� ��������� ���� ��� ������������ � BuildGeometry(). ������ ������ ����� ����� ��������������� ���������, ��������� �� ����� ���������
� ����������� ��� ������� ������� ������� World. � ��� ������ ��������� ������ ��������, ���� ��� ��� ������� ������� ��� ���������. ������
�������� (����������� � ����������) �������� � ����������� ������� FrameResources (��� 3 �������).						(���. 394 - 396) */
void D3D12Engine::BuildRenderItems()
{
	auto boxItem = std::make_unique<RenderItem>(g_numFrameResources);
	auto pyramideItem = std::make_unique<RenderItem>(g_numFrameResources);

	XMStoreFloat4x4(&boxItem->worldMatrix, DirectX::XMMatrixTranslation(1.0f, 0.0f, 0.0f));
	boxItem->geometry = m_geometries["primitives"].get();
	boxItem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxItem->indexCount = boxItem->geometry->DrawArgs["cylinder"].IndexCount;
	boxItem->startIndex = boxItem->geometry->DrawArgs["cylinder"].StartIndexLocation;
	boxItem->baseVertex = boxItem->geometry->DrawArgs["cylinder"].BaseVertexLocation;
	boxItem->objectCB_index = 0;
	m_allRenderItems.push_back(std::move(boxItem));

	XMStoreFloat4x4(&pyramideItem->worldMatrix, DirectX::XMMatrixTranslation(-1.0f, 0.0f, 0.0f));
	pyramideItem->geometry = m_geometries["primitives"].get();
	pyramideItem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	pyramideItem->indexCount = pyramideItem->geometry->DrawArgs["cylinder"].IndexCount;
	pyramideItem->startIndex = pyramideItem->geometry->DrawArgs["cylinder"].StartIndexLocation;
	pyramideItem->baseVertex = pyramideItem->geometry->DrawArgs["cylinder"].BaseVertexLocation;
	pyramideItem->objectCB_index = 1;
	m_allRenderItems.push_back(std::move(pyramideItem));

	/* ��� ������ ������������ */
	for (auto &item : m_allRenderItems)
		m_opaqueRenderItems.push_back(item.get());

}


//! ==========================================================   �����-�������   ==========================================================
/*
   ��������� ����������� ������ �����-��������, ������������ ��� ����������� �������� GPU. � �� �����, ���  GPU ��������� ������ ������ ���
����� N, ��������� ��� ���� ��������� ��� ����� �����, CPU ����� �� ����� � �������� ��������� � ������ ������ ��� N+1 �����.  ������  �� 3
�������� ������������ GPU ������� �� 3 ����� �����, ���� CPU �������� �������.	
   ����� ������ ����� ������ ����� 2 ������ ��������: ����������� � ����������. � �� � ������ ������� � �������  ������ ������  �����, �.�.
����� � ������ GPU 6 ����������� �������.																				(���. 389 - 394) */
void D3D12Engine::BuildFrameResources()
{
	for (int i = 0; i < g_numFrameResources; ++i)
	{
		m_frameResources.push_back(std::make_unique<FrameResources>(m_device.Get(), 1, m_allRenderItems.size()));
	}
}
