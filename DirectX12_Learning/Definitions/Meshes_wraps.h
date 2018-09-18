#pragma once

#include <DirectXCollision.h>
#include "../Helpers/MathHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


//! =========================================   ������������ ������ � ������� �������� � ������   =========================================
/*
������������ ������ � ������� ������ � ��������.																						 */
struct SubmeshGeometry
{
	UINT IndexCount = 0;			// ����� �������� ������
	UINT StartIndexLocation = 0;	// ��������� ������ � ��������� ������
	INT BaseVertexLocation = 0;		// ������� ������ ������� � ��������� ������
	BoundingBox Bounds;
};


//! ========================================================   ���������  �������   ========================================================
/*
������ ��, ��� ���������� ��� ��������� ������� �� ����� Draw():
- ������ ������ ������� (������� ������ - ������� ������� � ������� �������� ���������):
- � ����� ID3D12Resource ��� ������ � GPU;
- � ����� ID3DBlob ��� ������ � CPU.
- ��������� ����� ������� - ��� �� � 2 ������, ��� � ����� ������;
- ����������� ��������� ������� (� ��������� ���������� ������ � ������ GPU, ��� Stride � ByteSize);
- ���������� ���������� ������ (� ��������� ���������� ������ � ������ GPU, ��� ������ � ByteSize);
- ������������ ������� � �������. � ����� �������� ������ ������ ����� ������� ������� ��� ��������� �������. ��� ��� �������������� ���
������ ���������� ������  DrawIndexedInstanced( IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation ).
��� ������ ������ ������������, ��� ��� ������������ � ������ �������� � � ������� ������. ���� ������ - ����� ��������, ��������� ������ �
��������� ������; � ������� ������ ������� � ��������� ������ (� ������� ����� ������������ ������� ��� ��������� ���������). (���. 316). */
struct MeshGeometry
{
	struct VBufferMetrics
	{
		UINT VertexByteStride = 0;
		UINT VertexBufferByteSize = 0;
	};

	std::string Name;
	ComPtr<ID3DBlob> VBufferCPU;						// ��������� ������, ��������� CPU (��� ���������� �������� � �.�.)
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
	ComPtr<ID3D12Resource> VBufferGPU;				// ��������� ������, ��������� GPU (���������� ��� ���������)
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	VBufferMetrics vbMetrics;							// Stride � ByteSize ���������������� ���������� ������
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;				// ������ ���������� ������
	UINT IndexBufferByteSize = 0;

	D3D12_VERTEX_BUFFER_VIEW VBufferViews;  // ����������� ��������� ������� (������������ � Draw() ������� IASetVertexBuffers())
	D3D12_INDEX_BUFFER_VIEW IBufferView;			  // ���������� ���������� ������ (������������ � Draw() ������� IASetIndexBuffer())
	bool vbViewInited = false;
	bool ibViewInited = false;
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;	// ������������ ������� � ������� (��������)

																//! ����� ������ ������������ ��������� �������, �������� ��� �������������
	D3D12_VERTEX_BUFFER_VIEW VertexBufferViews()
	{
		if (!vbViewInited)
		{
			VBufferViews.BufferLocation = VBufferGPU->GetGPUVirtualAddress();
			VBufferViews.StrideInBytes = vbMetrics.VertexByteStride;
			VBufferViews.SizeInBytes = vbMetrics.VertexBufferByteSize;
		}
		return VBufferViews;
	}

	//! ����� ���������� ���������� ������, �������� ��� �������������
	D3D12_INDEX_BUFFER_VIEW IndexBufferView()
	{
		if (!ibViewInited)
		{
			IBufferView.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
			IBufferView.Format = IndexFormat;
			IBufferView.SizeInBytes = IndexBufferByteSize;
		}
		return IBufferView;
	}

	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};


//! ====================================================   ��������� ���������  ������   ==================================================
/*
������������� ������ ��� ��������� �������� ������ ������.																			 */
struct RenderItem
{
	RenderItem(UINT8 durty) : durtyFrames(durty) {};

	XMFLOAT4X4 worldMatrix = MathHelper::Identity4x4();

	UINT objectCB_index = -1;	// ������ ��������� ��������� � ������

								/* ������������ ������ � ������� ������ � ��������.																					 */
	UINT indexCount = 0;
	UINT startIndex = 0;
	INT baseVertex = 0;
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	/* ���� ��������� � ������ ���������� (�-�, worldMatrix), �� ����� �������� � ������ �������� ��� ���� �����-��������. ���� durtyFrames
	����������, ������� ������� ������� ������ �������� ����������� ������ ��� ������ ������.											 */
	UINT8 durtyFrames = 0;
	MeshGeometry *geometry = nullptr;
};