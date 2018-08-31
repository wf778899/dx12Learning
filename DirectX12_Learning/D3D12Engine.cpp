#include "stdafx.h"
#include "D3D12Engine.h"

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
	BuildBoxGeometry();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	CreateCbvDescriptorHeaps();
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
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}


//! =============================================================   Update   ==============================================================
/*
   ����� ������ ���� ������ ������� �������� � ����� �� � ����������� ������.															 */
void D3D12Engine::Update(const GameTimer & timer)
{
	Constants constants;

	float x = m_radius * sinf(m_phi) * cos(m_theta);
	float z = m_radius * sinf(m_phi) * sin(m_theta);
	float y = m_radius * cosf(m_phi);

	/* ������ View-������� �� ���������� ������, ��������� �.																			 */
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);

	/* �������� Projection-������� (�������� �����, � ���������������� ������ OnResize()).												 */
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);

	/* ������ World-������� ��� ����, ��������� WVP-������� � ����� � � ����� ����������� �������� [0].								 */
	XMMATRIX world = XMMatrixTranslation(-1.0f, -1.0f, -1.0f);
	XMMATRIX worldViewProj = world * view * proj;
	XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	constants.gPulseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_constant_po_buf->CopyData(0, constants);

	/* ������ World-������� ��� ��������, ��������� WVP-������� � ����� � � ����� ����������� �������� [1].							 */
	world = XMMatrixTranslation(1.0f, -1.0f, -1.0f);
	worldViewProj = world * view * proj;
	XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	constants.gPulseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_constant_po_buf->CopyData(1, constants);

	/* ��������� ����� ���������� �������� (�� ����� ������ � �������������� �����)														 */
	m_constant_pf_buf->CopyData(0, timer.TotalTime());
	m_constant_pf_buf->CopyData(1, 5.0f);
}


//! ============================================================   ���������   ============================================================
/*
   ������. */
void D3D12Engine::Draw(const GameTimer &timer)
{
	/* ������� ��������� � ������ ������ ��� ���������� �������������																	 */
	ThrowIfFailed(m_cmdAllocator->Reset());
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), m_pipelineState.Get()));

	/* �������� �������� � �������																										 */
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);

	/* STATE_PRESENT  ==>  STATE_RENDER_TARGET																							 */
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	/* ������� ������� back-����� � ����� ������. ���������� �� � Output Merger'�.														 */
	m_cmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::DarkViolet, 0, nullptr);
	m_cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	/*												_____________________________________________________________
	  ��������� ����������� ������������ � ����:	| ���������� | ���������� |	  ����������   |   ����������   |
													| 1 �������	 | 2 �������  |	����� �������� | ����� �������� |
													|____(���)___|_(��������)_|_______b1_______|_______b2_______|						 */

	/* ������������� ���� ������������ � ��������� �� ����������� � ������ ��������														 */
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_heap.Get() };
	m_cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	/* ������������� ������ ������ � ��������� �����, �������� ��������� ��������� �����.												 */
	m_cmdList->IASetVertexBuffers(0, 2, m_boxGeometry->VertexBufferViews());
	m_cmdList->IASetIndexBuffer(&m_boxGeometry->IndexBufferView());
	m_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	/* ���������� ��� b1 (������ � ����). ������������� ��� � ������������ � ���������� ������� root-��������� � ������ �� ������.		 */
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle2(m_CBV_heap->GetGPUDescriptorHandleForHeapStart(), 2, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(1, cbvHandle2);

	/* ���������� ��� ���� (������ � ����). ������������� ��� � ������������ � ���������� ������� root-��������� � ������ ���.			 */
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_CBV_heap->GetGPUDescriptorHandleForHeapStart());
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["box"].IndexCount, 1, m_boxGeometry->DrawArgs["box"].StartIndexLocation, m_boxGeometry->DrawArgs["box"].BaseVertexLocation, 0);

	/* ���������� ��� �������� (������ � ����, ������� ��������� �������). ����������������� ��� ������� ��������� � ������ ��������.	 */
	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	m_cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	m_cmdList->DrawIndexedInstanced(m_boxGeometry->DrawArgs["pyramide"].IndexCount, 1, m_boxGeometry->DrawArgs["pyramide"].StartIndexLocation, m_boxGeometry->DrawArgs["pyramide"].BaseVertexLocation, 0);

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

	FlushCommandQueue();
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


//! ======================================================   ���� ������������ CBV   ======================================================
/*
   ������  ����  ��  4  ������������ ��� ����������� �������.  ���  ���  �����������  ��������  (� ����� ������) � 2 ��� ����� (� ������).
����������� ����������� ������������ ��������� ��� ��������� �����. �������. ����� - ���� ��� � �� ����.				(���. 337 - 339) */
void D3D12Engine::CreateCbvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 4;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBV_heap)));
}


//! ======================================================   ���� ������������ CBV   ======================================================
void D3D12Engine::CreateCBVdescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;

	m_passCBVoffset = m_allRenderItems.size() * g_numFrameResources;	// ��������� �������� ���������� ������������

	cbvHeapDesc.NumDescriptors = (m_allRenderItems.size() + 1) * g_numFrameResources;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}


//! =============================================   ����������� ����� � ����������� �� ����   =============================================
/*
   � ��� 2 ����������� ������. � ����� - ���������, ����������� ��� ������� ������� �����. � ������ - ���������, �����  ���  ���� ��������.
� �� � ������ ���������������� ������ �����. �� ������ ��������� ������������� ���� ����������. ����������� ����������� ������������ � Draw
���������, � ������ � ���� �� ����� � �������, ��� �������� ������� �������. ����� - ���� ��� �� ������ ��������� ���� ��������.
   ����� ������� ������ ������ ��������. ����� - ����������� �� �� ��������.											(���. 337 - 339) */
void D3D12Engine::BuildConstantBuffers()
{
	/* ������ ����������� ������. ��� ����� ���������� UploadBuffer - �� ������ ������������  � ������ GPU  �����  �  �������������  �����
	CopyData( ������, ������ ). �.�. ����� ����� �������� ����� � ����� �����.															 */
	m_constant_po_buf = std::make_unique<UploadBuffer<Constants>>(m_device.Get(), 2, true);	// ����������� ��������� (�� ����� ��������)
	m_constant_pf_buf = std::make_unique<UploadBuffer<float>>(m_device.Get(), 2, true);		// �����

	/* ������� ������ �������� �������. */
	const UINT cb_po_ElemByteSize = m_constant_po_buf->elementByteSize();
	const UINT cb_pf_ElemByteSize = m_constant_pf_buf->elementByteSize();

	/* �������� ����� ����� ������� � ������ GPU. */
	D3D12_GPU_VIRTUAL_ADDRESS cb_po_Address = m_constant_po_buf->Resource()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS cb_pf_Address = m_constant_pf_buf->Resource()->GetGPUVirtualAddress();

	/* ������, ���� ������ ������� � ������� �� ���������, �������������� ����������� ��� ���. */
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_CBV_heap->GetCPUDescriptorHandleForHeapStart());	// �������� ����� �� ������ ���������� ����

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cb_po_Address;
	cbvDesc.SizeInBytes = cb_po_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// ��������� ������ ���������� (��� �����������)

	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	cbvDesc.BufferLocation += cb_po_ElemByteSize;
	cbvDesc.SizeInBytes = cb_po_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// ��������� ������ ���������� (��� �����������)

	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	cbvDesc.BufferLocation = cb_pf_Address;
	cbvDesc.SizeInBytes = cb_pf_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// ��������� ������ ���������� (��� �����)

	cbvHandle.Offset(1, m_CBV_SRV_UAV_descriptorSize);
	cbvDesc.BufferLocation += cb_pf_ElemByteSize;
	cbvDesc.SizeInBytes = cb_pf_ElemByteSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);	// ��������� �������� ���������� (��� �����)
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
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 1);	// �������� �� 2 ������������ ������������ � ��������, ������� � b1
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


//! ===========================================   �������� ��������,  ����������� InputLayout   ===========================================
/*
   ��������� ����-��� �� .cso-������������������ ������ ��������. ������� ����������� ���������� ����� 2 �����. � ������  �����  ����������
��������� ����� �� ���������� "POSITION" � "TEXCOORD", �� ������� - "NORMAL", "TANGENT" � "COLOR".			 (���. 321 - 325, 344 - 353) */
void D3D12Engine::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;
	m_vsByteCode = Util::LoadBinary(L"cso/VertexShader.cso");
	m_psByteCode = Util::LoadBinary(L"cso/PixelShader.cso");

	m_inputLayout =
	{
		/* "���������" "������"      "������ ��������"      "����" "������"				     "?"                       "?" */
		{  "POSITION",    0,    DXGI_FORMAT_R32G32B32_FLOAT,  0,      0,    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{  "TEXCOORD",    0,    DXGI_FORMAT_R32G32_FLOAT,     0,      12,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{  "NORMAL",      0,    DXGI_FORMAT_R32G32B32_FLOAT,  1,      0,    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{  "TANGENT",     0,    DXGI_FORMAT_R32G32B32_FLOAT,  1,      12,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{  "COLOR",       0,    DXGI_FORMAT_B8G8R8A8_UNORM,   1,      24,   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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


//! ========================================================   ������  ���������   ========================================================
/*
   � ����� � ��� �� ������ ������ ��������� ������� ��� ������ ��������. ������ �������� ������ ������������ � 2 ��������. � 1-� ����������
��������: ������� �������, ���������� ��������. �� 2-� ������ �������, ��������, ���� �������. ��� ������ ����������� � ���������  �����  2
�����. ��� �� ����������� ����� ��������.																								 */
void D3D12Engine::BuildBoxGeometry()
{
	/* � ������������� ����� ����� ������ ��������� ��� ����� ������ (������ ������� ���������), ������� ����� ��������� � ������ �����. */
	const UINT64 numVertices = 13;
	std::array<SeparatedVertex, numVertices> verticesSplittedAttr =
	{
		/*=========1 ���� (������� �������, ��������� ��������)=========*/  /*================2 ���� (������ �������, ��������, ����)================*/
		// ���
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::White)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,+1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Black)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,+1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Red)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Green)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Blue)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,+1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Yellow)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,+1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Cyan)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Magenta)}}),
		// ��������
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::White)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,-1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Black)}}),
		SeparatedVertex({{XMFLOAT3(+1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Red)}}),
		SeparatedVertex({{XMFLOAT3(-1.0f,-1.0f,+1.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Green)}}),
		SeparatedVertex({{XMFLOAT3( 0.0f,+2.0f, 0.0f),XMFLOAT2(0.0f,0.0f)}, {XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f),XMCOLOR(Colors::Blue)}}),
	};
	std::array<std::uint16_t, 54> indices =
	{
		// ���
		0, 1, 2, 0, 2, 3,	// Front
		4, 6, 5, 4, 7, 6,	// Back
		4, 5, 1, 4, 1, 0,	// Left
		3, 2, 6, 3, 6, 7,	// Right
		1, 5, 6, 1, 6, 2,	// Top
		4, 0, 3, 4, 3, 7,	// Bottom
		// ��������
		1, 3, 0, 1, 2, 3,	// Bottom
		0, 4, 1,			// Front
		1, 4, 2,			// Right
		2, 4, 3,			// Back
		3, 4, 0				// Left
	};

	/* �� ������� ������� ��������� ��������� 2 ������� � ����������. � 1 - ������� �������, ���������� ��������.  �� 2-� - ������ �������,
	��������, ���� �������.																												 */
	std::array<SeparatedVertex::PosTex, numVertices> verticesPT;
	std::array<SeparatedVertex::NorTanCol, numVertices> verticesNTC;

	for (int i = 0; i < numVertices; ++i)
	{
		verticesPT[i] = verticesSplittedAttr[i].pos_tex;
		verticesNTC[i] = verticesSplittedAttr[i].nor_tan_col;
	}
	const UINT iByteSize = (UINT)indices.size() * sizeof(std::uint16_t);			// ������ ���������� ������
	const UINT vByteSizePT = numVertices * sizeof(SeparatedVertex::PosTex);			// ������ �������, ������������� � ����� 1
	const UINT vByteSizeNTC = numVertices * sizeof(SeparatedVertex::NorTanCol);		// ������ �������, ������������� � ����� 2

	// ��������� ��������� �������
	m_boxGeometry = std::make_unique<MeshGeometry<2>>();
	m_boxGeometry->Name = "Box";

	/* ������������ ��� ���������� VertexBufferView */
	m_boxGeometry->vbMetrics[0].VertexBufferByteSize = vByteSizePT;					// ByteSize � Stride ������ ������ ��� 1 �����
	m_boxGeometry->vbMetrics[0].VertexByteStride = sizeof(SeparatedVertex::PosTex);
	m_boxGeometry->vbMetrics[1].VertexBufferByteSize = vByteSizeNTC;				// ByteSize � Stride ������ ������ ��� 2 �����
	m_boxGeometry->vbMetrics[1].VertexByteStride = sizeof(SeparatedVertex::NorTanCol);

	m_boxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_boxGeometry->IndexBufferByteSize = iByteSize;									// ByteSize ���������� ������

	// ��������� ������ ������ � �������� � ������ �� ������� CPU
	ThrowIfFailed(D3DCreateBlob(vByteSizePT, &m_boxGeometry->VBufferCPU[0]));		// ����������� ����� � ������
	ThrowIfFailed(D3DCreateBlob(vByteSizeNTC, &m_boxGeometry->VBufferCPU[1]));
	ThrowIfFailed(D3DCreateBlob(iByteSize, &m_boxGeometry->IndexBufferCPU));

	CopyMemory(m_boxGeometry->VBufferCPU[0]->GetBufferPointer(), verticesPT.data(), vByteSizePT);	// ��������� ��� �������
	CopyMemory(m_boxGeometry->VBufferCPU[1]->GetBufferPointer(), verticesNTC.data(), vByteSizeNTC);
	CopyMemory(m_boxGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);

	/* ������ � GPU ������ ������ (�� ���������� ������) � ��������� �����. �������� ���� ������ �� ��������������� ��������.			 */
	m_boxGeometry->VBufferGPU[0] = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), verticesPT.data(), vByteSizePT, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->VBufferGPU[1] = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), verticesNTC.data(), vByteSizeNTC, m_boxGeometry->VertexBufferUploader);
	m_boxGeometry->IndexBufferGPU =
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), indices.data(), iByteSize, m_boxGeometry->IndexBufferUploader);

	/* ��������� ��������� �������� � ������� ������ � ��������� ������ */
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


//! ========================================================   ������  ���������   ========================================================
/*
   ��� ��������� �������� MeshGeometry.  ��� ������ ������ � ��������, ����������� � �� ������� CPU, � GPU;  ��� ����������� �� ��� ������;
��� ��������� ��������� ������� � ���� �������. */
void D3D12Engine::BuildGeometry()
{
	const UINT64 numVertices = 13;

	std::array<Vertex, numVertices> vertices =
	{
		/*===========(������� �������)==============(����)==========*/
		// ���
		Vertex(XMFLOAT3(-1.0f,-1.0f,-1.0f), XMCOLOR(Colors::White)), Vertex(XMFLOAT3(-1.0f,+1.0f,-1.0f), XMCOLOR(Colors::Black)),
		Vertex(XMFLOAT3(+1.0f,+1.0f,-1.0f), XMCOLOR(Colors::Red)), Vertex(XMFLOAT3(+1.0f,-1.0f,-1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(-1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Blue)), Vertex(XMFLOAT3(-1.0f,+1.0f,+1.0f), XMCOLOR(Colors::Yellow)),
		Vertex(XMFLOAT3(+1.0f,+1.0f,+1.0f), XMCOLOR(Colors::Cyan)), Vertex(XMFLOAT3(+1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Magenta)),
		// ��������
		Vertex(XMFLOAT3(-1.0f,-1.0f,-1.0f), XMCOLOR(Colors::White)),
		Vertex(XMFLOAT3(+1.0f,-1.0f,-1.0f), XMCOLOR(Colors::Black)),
		Vertex(XMFLOAT3(+1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Red)),
		Vertex(XMFLOAT3(-1.0f,-1.0f,+1.0f), XMCOLOR(Colors::Green)),
		Vertex(XMFLOAT3(0.0f,+2.0f, 0.0f), XMCOLOR(Colors::Blue)),
	};

	std::array<std::uint16_t, 54> indices =
	{
		// ���
		/*=== Front ======|====== Back =======|====== Left =======|====== Right ======|======= Top =======|==== Bottom ====*/
		0, 1, 2, 0, 2, 3,   4, 6, 5, 4, 7, 6,   4, 5, 1, 4, 1, 0,   3, 2, 6, 3, 6, 7,   1, 5, 6, 1, 6, 2,   4, 0, 3, 4, 3, 7,
		// ��������
		/*=== Bottom =====|= Front ==|= Right ==|= Back ===|= Left*/
		1, 3, 0, 1, 2, 3,	0, 4, 1,   1, 4, 2,   2, 4, 3,   3, 4, 0
	};

	UINT vByteSize = vertices.size() * sizeof(Vertex);
	UINT iByteSize = indices.size() * sizeof(std::uint16_t);

	/* ������ ��������� */
	std::unique_ptr<MeshGeometry<1>> shapes = std::make_unique<MeshGeometry<1>>();
	shapes->Name = "Primitives";
	
	shapes->vbMetrics->VertexBufferByteSize = vByteSize;
	shapes->vbMetrics->VertexByteStride = sizeof(Vertex);
	shapes->IndexBufferByteSize = iByteSize;
	shapes->IndexFormat = DXGI_FORMAT_R16_UINT;

	/* ��������� ��������� �� ������� CPU */
	ThrowIfFailed(D3DCreateBlob(vByteSize, &shapes->VBufferCPU[0]));
	ThrowIfFailed(D3DCreateBlob(iByteSize, &shapes->IndexBufferCPU));
	CopyMemory(shapes->VBufferCPU[0]->GetBufferPointer(), vertices.data(), vByteSize);
	CopyMemory(shapes->IndexBufferCPU->GetBufferPointer(), indices.data(), iByteSize);

	/* ������ ������ � GPU, ��������� � ��� ��������� */
	shapes->VBufferGPU[0] = 
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), vertices.data(), vByteSize, shapes->VertexBufferUploader);
	shapes->IndexBufferGPU =
		Util::CreateDefaultBuffer(m_device.Get(), m_cmdList.Get(), indices.data(), iByteSize, shapes->IndexBufferUploader);

	/* ������������ ������� � ������� */
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


//! ========================================================   ���������  ������   ========================================================
/*
   ��� ��������� ���� ��� ������������ � BuildGeometry(). ������ ������ ����� ����� ��������������� ���������, ��������� �� ����� ���������
����������� ��� ������� ������� ������� World. � ��� ������ ��������� ������ ��������, ���� ��� ��� �������  ������� ��� ���������.  ������
�������� (����������� � ����������) �������� � ����������� ������� FrameResources (��� 3 �������).						(���. 394 - 396) */
void D3D12Engine::BuildRenderItems()
{
	auto boxItem = std::make_unique<RenderItem>(g_numFrameResources);
	auto pyramideItem = std::make_unique<RenderItem>(g_numFrameResources);

	XMStoreFloat4x4(&boxItem->worldMatrix, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxItem->geometry = m_geometries["Primitives"].get();
	boxItem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxItem->indexCount = boxItem->geometry->DrawArgs["box"].IndexCount;
	boxItem->startIndex = boxItem->geometry->DrawArgs["box"].StartIndexLocation;
	boxItem->baseVertex = boxItem->geometry->DrawArgs["box"].BaseVertexLocation;
	boxItem->objectCB_index = 0;
	m_allRenderItems.push_back(std::move(boxItem));

	pyramideItem->worldMatrix = MathHelper::Identity4x4();
	pyramideItem->geometry = m_geometries["Primitives"].get();
	pyramideItem->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	pyramideItem->indexCount = pyramideItem->geometry->DrawArgs["pyramide"].IndexCount;
	pyramideItem->startIndex = pyramideItem->geometry->DrawArgs["pyramide"].StartIndexLocation;
	pyramideItem->baseVertex = pyramideItem->geometry->DrawArgs["pyramide"].BaseVertexLocation;
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
