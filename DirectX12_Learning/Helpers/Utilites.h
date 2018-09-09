#pragma once
#include "d3dx12.h"
#define DEBUG

using Microsoft::WRL::ComPtr;

UINT8 D3D12GetFormatPlaneCount(_In_ ID3D12Device *pDevice, DXGI_FORMAT format);
UINT D3D12CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize);


//! ==================================================   Класс вспомогательных  утилит   ==================================================
class Util
{
public:
	static ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, const void *initData, UINT64 byteSize, ComPtr<ID3D12Resource>&uploadBuffer);

	static UINT CalcConstantBufferByteSize(UINT byteSize) { return (byteSize + 255) & ~255; }

	static ComPtr<ID3DBlob> LoadBinary(const std::wstring &fileName);

	static ComPtr<ID3DBlob> CompileShader(const std::wstring &file, const D3D_SHADER_MACRO *defs, const std::string &entry, const std::string &target);
};


//! =======================================================   Update Subresources   =======================================================

template <UINT MaxSubresources> inline UINT64 UpdateSubresources(_In_ ID3D12GraphicsCommandList *cmdList,
																 _In_ ID3D12Resource *destinationResource,
																 _In_ ID3D12Resource *sourceResource,
																 UINT64 sourceOffset,
																 _In_range_(0, MaxSubresources) UINT firstSubresource,
																 _In_range_(1, MaxSubresources - firstSubresource) UINT numSubresources,
																 _In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA *sourceData)
{
	UINT64 requiredSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MaxSubresources];
	UINT numRows[MaxSubresources];
	UINT64 rowSizesInBytes[MaxSubresources];

	D3D12_RESOURCE_DESC desc = destinationResource->GetDesc();
	ID3D12Device *device;
	destinationResource->GetDevice(__uuidof(*device), reinterpret_cast<void**>(&device));
	device->GetCopyableFootprints(&desc, firstSubresource, numSubresources, sourceOffset, layouts, numRows, rowSizesInBytes, &requiredSize);
	device->Release();

	D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
	D3D12_RESOURCE_DESC destinDesc = destinationResource->GetDesc();

	if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || sourceDesc.Width < requiredSize + layouts[0].Offset || requiredSize >(SIZE_T) - 1 ||
		(destinDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && (firstSubresource != 0 || numSubresources != 1))) {
		return 0;
	}

	BYTE *data;
	if (FAILED(sourceResource->Map(0, NULL, reinterpret_cast<void**>(&data))))
		return 0;

	for (UINT i = 0; i < numSubresources; ++i) {
		if (rowSizesInBytes[i] >(SIZE_T)-1)
			return 0;
		D3D12_MEMCPY_DEST destData = { data + layouts[i].Offset, layouts[i].Footprint.RowPitch, layouts[i].Footprint.RowPitch * numRows[i] };
		MemcpySubresource(&destData, &sourceData[i], (SIZE_T)rowSizesInBytes[i], numRows[i], layouts[i].Footprint.Depth);
	}
	sourceResource->Unmap(0, NULL);
	if (destinDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
		CD3DX12_BOX srcBox(UINT(layouts[0].Offset), UINT(layouts[0].Offset + layouts[0].Footprint.Width));
		cmdList->CopyBufferRegion(destinationResource, 0, sourceResource, layouts[0].Offset, layouts[0].Footprint.Width);
	}
	else {
		for (UINT i = 0; i < numSubresources; ++i) {
			CD3DX12_TEXTURE_COPY_LOCATION Dst(destinationResource, i + firstSubresource);
			CD3DX12_TEXTURE_COPY_LOCATION Src(sourceResource, layouts[i]);
			cmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return requiredSize;
}


inline void MemcpySubresource(_In_ const D3D12_MEMCPY_DEST *dest, 
							  _In_ const D3D12_SUBRESOURCE_DATA *srcData, 
							  SIZE_T rowSizeInBytes, 
							  UINT numRows, 
							  UINT numSlices) {
	for (UINT i = 0; i < numSlices; ++i) {
		BYTE *destSlice = reinterpret_cast<BYTE*>(dest->pData) + dest->SlicePitch * i;
		const BYTE *srcSlice = reinterpret_cast<const BYTE*>(srcData->pData) + srcData->SlicePitch * i;
		for (UINT k = 0; k < numRows; ++k) {
			memcpy(destSlice + dest->RowPitch * k, srcSlice + srcData->RowPitch * k, rowSizeInBytes);
		}
	}
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ВСПОМОГАТЕЛЬНЫЕ КЛАССЫ, СТРУКТУРЫ   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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
template<UINT8 numSlots> struct MeshGeometry
{
	struct VBufferMetrics {
		UINT VertexByteStride = 0;
		UINT VertexBufferByteSize = 0;
	};

	std::string Name;
	ComPtr<ID3DBlob> VBufferCPU[numSlots];						// Вершинные буферы, доступные CPU (для вычисления коллизий и т.п.)
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
	ComPtr<ID3D12Resource> VBufferGPU[numSlots];				// Вершинные буферы, доступные GPU (собственно для отрисовки)
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	VBufferMetrics vbMetrics[numSlots];							// Stride и ByteSize соответствующего вершинного буфера
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;				// Формат индексного буфера
	UINT IndexBufferByteSize = 0;

	D3D12_VERTEX_BUFFER_VIEW VBufferViews[numSlots];  // Дескрипторы вершинных буферов (подключаются в Draw() методом IASetVertexBuffers())
	D3D12_INDEX_BUFFER_VIEW IBufferView;			  // Дескриптор индексного буфера (подключаются в Draw() методом IASetIndexBuffer())
	bool vbViewInited = false;
	bool ibViewInited = false;
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;	// Расположение моделей в буферах (поимённое)

	//! Выдаёт массив дескрипторов вершинных буферов, единожды его инициализируя
	D3D12_VERTEX_BUFFER_VIEW* VertexBufferViews() 
	{
		if (!vbViewInited) {
			for (UINT8 i = 0; i < numSlots; ++i) {
				VBufferViews[i].BufferLocation = VBufferGPU[i]->GetGPUVirtualAddress();
				VBufferViews[i].StrideInBytes = vbMetrics[i].VertexByteStride;
				VBufferViews[i].SizeInBytes = vbMetrics[i].VertexBufferByteSize;
			}
		}
		return VBufferViews;
	}

	//! Выдаёт дескриптор индексного буфера, единожды его инициализируя
	D3D12_INDEX_BUFFER_VIEW IndexBufferView()
	{
		if (!ibViewInited) {
			IBufferView.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
			IBufferView.Format = IndexFormat;
			IBufferView.SizeInBytes = IndexBufferByteSize;
		}
		return IBufferView;
	}

	void DisposeUploaders() {
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
	MeshGeometry<1> *geometry = nullptr;
};


//! ========================================================   Класс  исключения   ========================================================
class DxException
{
public:
	DxException() = default;

	DxException(HRESULT hr_, const std::wstring &functionName_, const std::wstring &fileName_, int lineNumber_)
		: errorCode(hr_)
		, functionName(functionName_)
		, fileName(fileName_)
		, lineNumber(lineNumber_)
	{}

	std::wstring toString() const
	{
		_com_error error(errorCode);
		std::wstring message = error.ErrorMessage();
		return functionName + L" failed in " + fileName + L"; line " + std::to_wstring(lineNumber) + L"; error: " + message;
	}

	HRESULT errorCode = S_OK;
	std::wstring functionName;
	std::wstring fileName;
	int lineNumber = -1;
};


#define SC_ALLOW_MODE_SWITCH DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   МАКРОСЫ   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
	HRESULT hr = (x);													\
	std::wstring fileName = AnsiToWString(__FILE__);					\
	if(FAILED(hr)) { throw DxException(hr, L#x, fileName, __LINE__); }	\
}
#endif


#ifndef ReleaseCom
#define ReleaseCom(x)				\
{									\
	if(x) {							\
		x->Release();				\
		x = 0;						\
	}								\
}
#endif


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

inline std::wstring ToString(const DXGI_ADAPTER_DESC &desc, const std::wstring &extra = L"", bool prepend = false)
{
	std::wstring description = L"";
	description += desc.Description;
	if (!extra.empty()) {
		if (prepend)
			description = extra + L" " + description + L"\n";
		else
			description += L" " + extra + L"\n";
	}
	return description;
}

inline std::wstring ToString(const DXGI_OUTPUT_DESC &desc, const std::wstring &extra = L"", bool prepend = false)
{
	std::wstring description = L"";
	description += desc.DeviceName;
	if (!extra.empty()) {
		if (prepend)
			description = extra + L" " + description + L"\n";
		else
			description += L" " + extra + L"\n";
	}
	return description;
}

inline std::wstring AnsiToWString(const std::string &str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}