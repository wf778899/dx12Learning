#pragma once

#include <DirectXCollision.h>
#include "../Helpers/MathHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


//! =========================================   Расположение модели в буферах индексов и вершин   =========================================
/*
Расположение модели в буферах вершин и индексов.																						 */
struct SubmeshGeometry
{
	UINT IndexCount = 0;			// число индексов модели
	UINT StartIndexLocation = 0;	// стартовый индекс в индексном буфере
	INT BaseVertexLocation = 0;		// позиция базовй вершины в вершинном буфере
	BoundingBox Bounds;
};


//! ========================================================   Геометрия  моделей   ========================================================
/*
Хранит всё, что необходимо для отрисовки моделей на этапе Draw():
- Буферы вершин моделей (сколько слотов - столько буферов с разными наборами атрибутов):
- в форме ID3D12Resource для чтения с GPU;
- в форме ID3DBlob для чтения с CPU.
- Индексный буфер моделей - так же в 2 формах, как и буфер вершин;
- Дескрипторы вершинных буферов (с описанием начального адреса в памяти GPU, его Stride и ByteSize);
- Дескриптор индексного буфера (с описанием начального адреса в памяти GPU, его формат и ByteSize);
- Расположение моделей в буферах. В одном огромном буфере вершин можно хранить вершины для отдельных моделей. Все они отрисовываются при
помощи отдельного вызова  DrawIndexedInstanced( IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation ).
Для каждой модели запоминается, где она представлена в буфере индексов и в буферах вершин. Если точнее - число индексов, стартовый индекс в
индексном буфере; и позиция базовй вершины в вершинном буфере (к которой будут прибавляться индексы при индексной отрисовке). (стр. 316). */
struct MeshGeometry
{
	struct VBufferMetrics
	{
		UINT VertexByteStride = 0;
		UINT VertexBufferByteSize = 0;
	};

	std::string Name;
	ComPtr<ID3DBlob> VBufferCPU;						// Вершинные буферы, доступные CPU (для вычисления коллизий и т.п.)
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
	ComPtr<ID3D12Resource> VBufferGPU;				// Вершинные буферы, доступные GPU (собственно для отрисовки)
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	VBufferMetrics vbMetrics;							// Stride и ByteSize соответствующего вершинного буфера
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;				// Формат индексного буфера
	UINT IndexBufferByteSize = 0;

	D3D12_VERTEX_BUFFER_VIEW VBufferViews;  // Дескрипторы вершинных буферов (подключаются в Draw() методом IASetVertexBuffers())
	D3D12_INDEX_BUFFER_VIEW IBufferView;			  // Дескриптор индексного буфера (подключаются в Draw() методом IASetIndexBuffer())
	bool vbViewInited = false;
	bool ibViewInited = false;
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;	// Расположение моделей в буферах (поимённое)

																//! Выдаёт массив дескрипторов вершинных буферов, единожды его инициализируя
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

	//! Выдаёт дескриптор индексного буфера, единожды его инициализируя
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


//! ====================================================   Геометрия отдельной  модели   ==================================================
/*
Инкапсулирует данные для отрисовки отдельно взятой модели.																			 */
struct RenderItem
{
	RenderItem(UINT8 durty) : durtyFrames(durty) {};

	XMFLOAT4X4 worldMatrix = MathHelper::Identity4x4();

	UINT objectCB_index = -1;	// Индекс объектной константы в буфере

								/* Расположение модели в буферах вершин и индексов.																					 */
	UINT indexCount = 0;
	UINT startIndex = 0;
	INT baseVertex = 0;
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	/* Если константы у модели поменяются (н-р, worldMatrix), их нужно записать в буфера констант для всех фрэйм-ресурсов. Поле durtyFrames
	показывает, сколько фреймов впереди должны обновить константные буфера для данной модели.											 */
	UINT8 durtyFrames = 0;
	MeshGeometry *geometry = nullptr;
};