#pragma once

#define DEBUG

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	BoundingBox Bounds;
};

struct MeshGeometry
{
	std::string Name;
	ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const {
		D3D12_VERTEX_BUFFER_VIEW vb_view;
		vb_view.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vb_view.StrideInBytes = VertexByteStride;
		vb_view.SizeInBytes = VertexBufferByteSize;
		return vb_view;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const {
		D3D12_INDEX_BUFFER_VIEW ib_view;
		ib_view.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ib_view.Format = IndexFormat;
		ib_view.SizeInBytes = IndexBufferByteSize;
		return ib_view;
	}

	void DisposeUploaders() {
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};


inline UINT64 UpdateSubresources(_In_ ID3D12GraphicsCommandList *cmdList,
								 _In_ ID3D12Resource *destinationResource,
								 _In_ ID3D12Resource *sourceResource,
								 _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT firstSubresource,
								 _In_range_(0, D3D12_REQ_SUBRESOURCES - firstSubresource) UINT numSubresources,
								 UINT64 requiredSize,
								 _In_reads_(numSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts,
								 _In_reads_(numSubresources) const UINT *numRows,
								 _In_reads_(numSubresources) const UINT64 *rowSizesInBytes,
								 _In_reads_(numSubresources) const D3D12_SUBRESOURCE_DATA *sourceData) {
	D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
	D3D12_RESOURCE_DESC destinDesc = destinationResource->GetDesc();
	if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || sourceDesc.Width < requiredSize + layouts[0].Offset || requiredSize >(SIZE_T) - 1 ||
		(destinDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && (firstSubresource != 0 || numSubresources != 1))) {
		return 0;
	}
	BYTE *data;
	if(FAILED(sourceResource->Map(0, NULL, reinterpret_cast<void**>(&data))))
		return 0;
	for (UINT i = 0; i < numSubresources; ++i) {
		if (rowSizesInBytes[i] > (SIZE_T)-1)
			return 0;
		D3D12_MEMCPY_DEST destData = { data + layouts[i].Offset, layouts[i].Footprint.RowPitch, layouts[i].Footprint.RowPitch * numRows[i] };
	}

}

template <UINT MaxSubresources> inline UINT64 UpdateSubresources(_In_ ID3D12GraphicsCommandList *cmdList,
																 _In_ ID3D12Resource *destinationResource,
																 _In_ ID3D12Resource *sourceResource,
																 UINT64 sourceOffset,
																 _In_range_(0, MaxSubresources) UINT firstSubresource,
																 _In_range_(1, MaxSubresources - firstSubresource) UINT numSubresources,
																 _In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA *sourceData) {
	UINT64 requiredSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MaxSubresources];
	UINT numRows[MaxSubresources];
	UINT64 rowSizesInBytes[MaxSubresources];

	D3D12_RESOURCE_DESC desc = destinationResource->GetDesc();
	ID3D12Device *device;
	destinationResource->GetDevice(__uuidof(*device), reinterpret_cast<void**>(&device));
	device->GetCopyableFootprints(&desc, firstSubresource, numSubresources, sourceOffset, layouts, numRows, rowSizesInBytes, requiredSize);
	device->Release();
}


struct CD3DX12_DEFAULT {};

struct CD3DX12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE() {}
	explicit CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE &o) : D3D12_CPU_DESCRIPTOR_HANDLE(o) {}
	CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT) { ptr = 0; }

	CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &other, INT offsetScaledByIncrementSize) {
		InitOffsetted(other, offsetScaledByIncrementSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &other, INT offsetInDesriptors, UINT descriptorIncrementSize) {
		InitOffsetted(other, offsetInDesriptors, descriptorIncrementSize);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize) {
		ptr += offsetScaledByIncrementSize;
		return *this;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDesriptors, UINT descriptorIncrementSize) {
		ptr += offsetInDesriptors * descriptorIncrementSize;
		return *this;
	}
	bool operator==(_In_  const D3D12_CPU_DESCRIPTOR_HANDLE &other) {
		return (this->ptr == other.ptr);
	}
	bool operator !=(_In_  const D3D12_CPU_DESCRIPTOR_HANDLE &other) {
		return (this->ptr != other.ptr);
	}

	inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetScaledByIncrementSize) {
		InitOffsetted(*this, base, offsetScaledByIncrementSize);
	}
	static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE &handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetScaledByIncrementSize) {
		handle.ptr = base.ptr + offsetScaledByIncrementSize;
	}
	inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetInDesriptors, UINT descriptorIncrementSize) {
		InitOffsetted(*this, base, offsetInDesriptors, descriptorIncrementSize);
	}
	static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE &handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE &base, INT offsetInDesriptors, UINT descriptorIncrementSize) {
		handle.ptr = base.ptr + offsetInDesriptors * descriptorIncrementSize;
	}
};

struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
	CD3DX12_RESOURCE_BARRIER() {}
	explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER &o) : D3D12_RESOURCE_BARRIER(o) {}

	static inline CD3DX12_RESOURCE_BARRIER Transition(_In_ ID3D12Resource *resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
		CD3DX12_RESOURCE_BARRIER result;
		ZeroMemory(&result, sizeof(result));
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		result.Flags = flags;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;
		barrier.Transition.Subresource = subresource;
		return result;
	}

	static inline CD3DX12_RESOURCE_BARRIER Aliasing(_In_ ID3D12Resource *resourceBefore, _In_ ID3D12Resource *resourceAfter) {
		CD3DX12_RESOURCE_BARRIER result;
		ZeroMemory(&result, sizeof(result));
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Aliasing.pResourceBefore = resourceBefore;
		barrier.Aliasing.pResourceAfter = resourceAfter;
		return result;
	}

	static inline CD3DX12_RESOURCE_BARRIER UAV(_In_ ID3D12Resource *resource) {
		CD3DX12_RESOURCE_BARRIER result;
		ZeroMemory(&result, sizeof(result));
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = resource;
		return result;
	}
};

struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
{
	CD3DX12_HEAP_PROPERTIES() {}
	explicit CD3DX12_HEAP_PROPERTIES(const CD3DX12_HEAP_PROPERTIES &o) : D3D12_HEAP_PROPERTIES(o) {}

	explicit CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE type, UINT creationNodeMask = 1, UINT nodeMask = 1) {
		Type = type;
		CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		CreationNodeMask = creationNodeMask;
		VisibleNodeMask = nodeMask;
	}
	CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPool, UINT creationNodeMask = 1, UINT nodeMask = 1) {
		Type = D3D12_HEAP_TYPE_CUSTOM;
		CPUPageProperty = cpuPageProperty;
		MemoryPoolPreference = memoryPool;
		CreationNodeMask = creationNodeMask;
		VisibleNodeMask = nodeMask;
	}
	bool IsCPUAccessible() const {
		return (Type == D3D12_HEAP_TYPE_UPLOAD || Type == D3D12_HEAP_TYPE_READBACK) ||
			(Type == D3D12_HEAP_TYPE_CUSTOM && (CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE || CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
	}
	operator const D3D12_HEAP_PROPERTIES&() const { return *this; }
};
inline bool operator==(const D3D12_HEAP_PROPERTIES &l, const D3D12_HEAP_PROPERTIES &r) {
	return l.Type == r.Type &&
		l.CPUPageProperty == r.CPUPageProperty &&
		l.MemoryPoolPreference == r.MemoryPoolPreference &&
		l.CreationNodeMask == r.CreationNodeMask &&
		l.VisibleNodeMask == r.VisibleNodeMask;
}
inline bool operator!=(const D3D12_HEAP_PROPERTIES &l, const D3D12_HEAP_PROPERTIES &r) { return !(r == l); }

inline UINT D3D12CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize) {
	return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
}
inline UINT8 D3D12GetFormatPlaneCount(_In_ ID3D12Device *pDevice, DXGI_FORMAT format) {
	D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = { format };
	if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo))))
		return 0;
	return formatInfo.PlaneCount;
}
struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC {
	CD3DX12_RESOURCE_DESC() {}
	explicit CD3DX12_RESOURCE_DESC(const D3D12_RESOURCE_DESC &other) : D3D12_RESOURCE_DESC(other) {}
	CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION dimension,
		UINT64 alignment,
		UINT64 width,
		UINT height,
		UINT16 depthOrArraySize,
		UINT16 mipLevels,
		DXGI_FORMAT format,
		UINT sampleCount,
		UINT sampleQuality,
		D3D12_TEXTURE_LAYOUT layout,
		D3D12_RESOURCE_FLAGS flags) {
		Dimension = dimension;
		Alignment = alignment;
		Width = width;
		Height = height;
		DepthOrArraySize = depthOrArraySize;
		MipLevels = mipLevels;
		Format = format;
		SampleDesc.Count = sampleCount;
		SampleDesc.Quality = sampleQuality;
		Layout = layout;
		Flags = flags;
	}
	static inline CD3DX12_RESOURCE_DESC Buffer(const D3D12_RESOURCE_ALLOCATION_INFO &resAllocInfo, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, resAllocInfo.Alignment, resAllocInfo.SizeInBytes, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT64 alignment = 0) {
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, alignment, width, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Tex1D(
		DXGI_FORMAT format,
		UINT64 width,
		UINT16 arraySize = 1,
		UINT16 mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0) {
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE1D, alignment, width, 1, arraySize, mipLevels, format, 1, 0, layout, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Tex2D(
		DXGI_FORMAT format,
		UINT64 width,
		UINT height,
		UINT16 arraySize = 1,
		UINT16 mipLevels = 0,
		UINT sampleCount = 1,
		UINT sampleQuality = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0) {
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE2D, alignment, width, height, arraySize, mipLevels, format, sampleCount, sampleQuality, layout, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Tex3D(
		DXGI_FORMAT format,
		UINT64 width,
		UINT height,
		UINT16 depth,
		UINT16 mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		UINT64 alignment = 0) {
		return CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_TEXTURE3D, alignment, width, height, depth, mipLevels, format, 1, 0, layout, flags);
	}
	inline UINT16 Depth() const { return (Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1); }
	inline UINT16 ArraySize() const { return (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1); }
	inline UINT8 PlaneCount(_In_ ID3D12Device *pDevice) const { return D3D12GetFormatPlaneCount(pDevice, Format); }
	inline UINT Subresources(_In_ ID3D12Device *pDevice) const { return MipLevels * ArraySize() * PlaneCount(pDevice); }
	inline UINT CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice) { return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, MipLevels, ArraySize()); }
	operator const D3D12_RESOURCE_DESC& () const { return *this; }
};
inline bool operator==(const D3D12_RESOURCE_DESC &l, const D3D12_RESOURCE_DESC &r) {
	return
		l.Dimension == r.Dimension &&
		l.Alignment == r.Alignment &&
		l.Width == r.Width &&
		l.Height == r.Height &&
		l.DepthOrArraySize == r.DepthOrArraySize &&
		l.MipLevels == r.MipLevels &&
		l.Format == r.Format &&
		l.SampleDesc.Count == r.SampleDesc.Count &&
		l.SampleDesc.Quality == r.SampleDesc.Quality &&
		l.Layout == r.Layout &&
		l.Flags == r.Flags;
}
inline bool operator!=(const D3D12_RESOURCE_DESC &l, const D3D12_RESOURCE_DESC &r) { return l != r; }

class Util
{
public:
	static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, const void *initData, UINT64 byteSize, ComPtr<ID3D12Resource>&uploadBuffer);
};


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


inline std::wstring AnsiToWString(const std::string &str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}


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