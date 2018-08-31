#pragma once
#include "MathHelper.h"

#define DEBUG

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

UINT8 D3D12GetFormatPlaneCount(_In_ ID3D12Device *pDevice, DXGI_FORMAT format);
UINT D3D12CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize);

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ������� ��  MICROSOFT   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//! ==============================================   ������� ���������������  ��-���������   ==============================================
struct CD3DX12_DEFAULT {};
extern const DECLSPEC_SELECTANY CD3DX12_DEFAULT D3D12_DEFAULT;


//! ===============================================   ������� D3D12_GPU_DESCRIPTOR_HANDLE   ===============================================
/*
   ������������� ������� ��������� ��� ������������� �����������. ���������������� ������ ������ (�������) ������������  ����, ������������
������� ����   GetGPUDescriptorHandleForHeapStart().   ��� ���� ����� ��������� ���� ���������� ������, ���� ������ ����������� � �� �����.
����� ������� ����� ���������� ��������������� ���������� ������������ ��������. ����� ������� �����  ��������  �����  ��������,  ���������
����� �� �������� ��������. */
struct CD3DX12_GPU_DESCRIPTOR_HANDLE : public D3D12_GPU_DESCRIPTOR_HANDLE
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE() {}
	explicit CD3DX12_GPU_DESCRIPTOR_HANDLE(const D3D12_GPU_DESCRIPTOR_HANDLE &o) : D3D12_GPU_DESCRIPTOR_HANDLE(o) {}
	CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT) { ptr = 0; }

	CD3DX12_GPU_DESCRIPTOR_HANDLE(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE &other, INT offsetScaledByIncrementSize) {
		InitOffsetted(other, offsetScaledByIncrementSize);
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE &other, INT offsetInDesriptors, UINT descriptorIncrementSize) {
		InitOffsetted(other, offsetInDesriptors, descriptorIncrementSize);
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize) {
		ptr += offsetScaledByIncrementSize;
		return *this;
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDesriptors, UINT descriptorIncrementSize) {
		ptr += offsetInDesriptors * descriptorIncrementSize;
		return *this;
	}
	bool operator==(_In_  const D3D12_GPU_DESCRIPTOR_HANDLE &other) {
		return (this->ptr == other.ptr);
	}
	bool operator !=(_In_  const D3D12_GPU_DESCRIPTOR_HANDLE &other) {
		return (this->ptr != other.ptr);
	}

	inline void InitOffsetted(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE &base, INT offsetScaledByIncrementSize) {
		InitOffsetted(*this, base, offsetScaledByIncrementSize);
	}
	static inline void InitOffsetted(_Out_ D3D12_GPU_DESCRIPTOR_HANDLE &handle, _In_ const D3D12_GPU_DESCRIPTOR_HANDLE &base, INT offsetScaledByIncrementSize) {
		handle.ptr = base.ptr + offsetScaledByIncrementSize;
	}
	inline void InitOffsetted(_In_ const D3D12_GPU_DESCRIPTOR_HANDLE &base, INT offsetInDesriptors, UINT descriptorIncrementSize) {
		InitOffsetted(*this, base, offsetInDesriptors, descriptorIncrementSize);
	}
	static inline void InitOffsetted(_Out_ D3D12_GPU_DESCRIPTOR_HANDLE &handle, _In_ const D3D12_GPU_DESCRIPTOR_HANDLE &base, INT offsetInDesriptors, UINT descriptorIncrementSize) {
		handle.ptr = base.ptr + offsetInDesriptors * descriptorIncrementSize;
	}
};


//! ===============================================   ������� D3D12_CPU_DESCRIPTOR_HANDLE   ===============================================
/*
������������� ������� ��������� ��� ������������� �����������. ���������������� ������ ������ (�������) ������������  ����, ������������
������� ����   GetCPUDescriptorHandleForHeapStart().   ��� ���� ����� ��������� ���� ���������� ������, ���� ������ ����������� � �� �����.
����� ������� ����� ���������� ��������������� ���������� ������������ ��������. ����� ������� �����  ��������  �����  ��������,  ���������
����� �� �������� ��������.																												 */
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


//! ===============================================   ������ ���  D3D12_RESOURCE_BARRIER   ===============================================
/*
   � ������� ����������� ������� ������ ������� D3D12_RESOURCE_BARRIER, ����������� ���� Transition �������, ����  Aliasing, ���� UAV.  */
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
	CD3DX12_RESOURCE_BARRIER() {}
	explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER &o) : D3D12_RESOURCE_BARRIER(o) {}

	static inline CD3DX12_RESOURCE_BARRIER Transition(
		_In_ ID3D12Resource *resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) 
	{
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

	static inline CD3DX12_RESOURCE_BARRIER Aliasing(_In_ ID3D12Resource *resourceBefore, _In_ ID3D12Resource *resourceAfter)
	{
		CD3DX12_RESOURCE_BARRIER result;
		ZeroMemory(&result, sizeof(result));
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Aliasing.pResourceBefore = resourceBefore;
		barrier.Aliasing.pResourceAfter = resourceAfter;
		return result;
	}

	static inline CD3DX12_RESOURCE_BARRIER UAV(_In_ ID3D12Resource *resource)
	{
		CD3DX12_RESOURCE_BARRIER result;
		ZeroMemory(&result, sizeof(result));
		D3D12_RESOURCE_BARRIER &barrier = result;
		result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = resource;
		return result;
	}
};


//! ================================================   ������ ��� D3D12_HEAP_PROPERTIES   ================================================
/*
   ������ ������� ������ ��� D3D12_HEAP_PROPERTIES, ����������� ����������������� ������������ � ��������� ���������					 */
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


//! =================================================   ������ ��� D3D12_RESOURCE_DESC   =================================================
/*
   ����������� ����������� ���������-���������� �������, �������� ����������� ������, ����������� �������� �������� ��� ������ ��� ��������
1D, 2D, 3D.																																 */
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
	static inline CD3DX12_RESOURCE_DESC Buffer(
		const D3D12_RESOURCE_ALLOCATION_INFO &resAllocInfo, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
	{
		return CD3DX12_RESOURCE_DESC(
			D3D12_RESOURCE_DIMENSION_BUFFER,
			resAllocInfo.Alignment,
			resAllocInfo.SizeInBytes,
			1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, 
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
	}
	static inline CD3DX12_RESOURCE_DESC Buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, UINT64 alignment = 0) 
	{
		return CD3DX12_RESOURCE_DESC(
			D3D12_RESOURCE_DIMENSION_BUFFER, 
			alignment,
			width,
			1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0,
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR, flags);
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
	inline UINT16 Depth() const 
	{
		return (Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1); 
	}
	inline UINT16 ArraySize() const
	{
		return (Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? DepthOrArraySize : 1); 
	}
	inline UINT8 PlaneCount(_In_ ID3D12Device *pDevice) const
	{
		return D3D12GetFormatPlaneCount(pDevice, Format); 
	}
	inline UINT Subresources(_In_ ID3D12Device *pDevice) const 
	{
		return MipLevels * ArraySize() * PlaneCount(pDevice); 
	}
	inline UINT CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice) 
	{
		return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, MipLevels, ArraySize()); 
	}
	operator const D3D12_RESOURCE_DESC& () const 
	{
		return *this; 
	}
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
inline bool operator!=(const D3D12_RESOURCE_DESC &l, const D3D12_RESOURCE_DESC &r) { return !(l == r); }


//! ======================================================   ������ ��� D3D12_BOX   ======================================================
/*
   ��������� ����������������� ������������ ��� D3D12_BOX																				 */
struct CD3DX12_BOX : public D3D12_BOX
{
	CD3DX12_BOX() {};
	explicit CD3DX12_BOX(const D3D12_BOX &other) : D3D12_BOX(other) {}
	explicit CD3DX12_BOX(LONG left, LONG right) {
		this->left = left;
		this->right = right;
		top = 0;
		bottom = 1;
		front = 0;
		back = 1;
	}
	explicit CD3DX12_BOX(LONG left, LONG right, LONG top, LONG bottom) {
		this->left = left;
		this->right = right;
		this->top = top;
		this->bottom = bottom;
		front = 0;
		back = 1;
	}
	explicit CD3DX12_BOX(LONG left, LONG right, LONG top, LONG bottom, LONG front, LONG back) {
		this->left = left;
		this->right = right;
		this->top = top;
		this->bottom = bottom;
		this->front = front;
		this->back = back;
	}
	~CD3DX12_BOX() {};
	operator const D3D12_BOX&() const { return *this; }
};
inline bool operator==(const D3D12_BOX &l, const D3D12_BOX &r) {
	return l.left == r.left && l.right == r.right && l.top == r.top && l.bottom == r.bottom && l.front == r.front && l.back == r.back;
}
inline bool operator!=(const D3D12_BOX &l, const D3D12_BOX &r) { return !(l == r); }


//! =============================================   ������ ���  D3D12_TEXTURE_COPY_LOCATION   ============================================
/*
   ��������� ����������������� ������������ ��� D3D12_TEXTURE_COPY_LOCATION																 */
struct CD3DX12_TEXTURE_COPY_LOCATION : public D3D12_TEXTURE_COPY_LOCATION
{
	CD3DX12_TEXTURE_COPY_LOCATION() {}
	explicit CD3DX12_TEXTURE_COPY_LOCATION(const D3D12_TEXTURE_COPY_LOCATION &other) : D3D12_TEXTURE_COPY_LOCATION(other) {}
	CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource *res) { pResource = res; }
	CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource *res, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const &footprint) {
		pResource = res;
		Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		PlacedFootprint = footprint;
	}
	CD3DX12_TEXTURE_COPY_LOCATION(ID3D12Resource *res, UINT sub) {
		pResource = res;
		Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		SubresourceIndex = sub;
	}
};


//! ===============================================   ������ ��� ��������  root-��������   ===============================================
struct CD3DX12_DESCRIPTOR_RANGE : public D3D12_DESCRIPTOR_RANGE
{
	CD3DX12_DESCRIPTOR_RANGE() {}
	explicit CD3DX12_DESCRIPTOR_RANGE(const D3D12_DESCRIPTOR_RANGE &other) : D3D12_DESCRIPTOR_RANGE(other) {}

	CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpase = 0,
		UINT offsetDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		Init(rangeType, numDescriptors, baseShaderRegister, registerSpase, offsetDescriptorsFromTableStart);
	}
	inline void Init(
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpase = 0,
		UINT offsetDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		Init(*this, rangeType, numDescriptors, baseShaderRegister, registerSpase, offsetDescriptorsFromTableStart);
	}
	static inline void Init(
		_Out_ D3D12_DESCRIPTOR_RANGE &range,
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
		UINT numDescriptors,
		UINT baseShaderRegister,
		UINT registerSpase = 0,
		UINT offsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		range.RangeType = rangeType;
		range.NumDescriptors = numDescriptors;
		range.BaseShaderRegister = baseShaderRegister;
		range.RegisterSpace = registerSpase;
		range.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;
	}
};

struct CD3DX12_ROOT_DESCRIPTOR_TABLE : public D3D12_ROOT_DESCRIPTOR_TABLE
{
	CD3DX12_ROOT_DESCRIPTOR_TABLE() {}
	explicit CD3DX12_ROOT_DESCRIPTOR_TABLE(const D3D12_ROOT_DESCRIPTOR_TABLE &other) : D3D12_ROOT_DESCRIPTOR_TABLE(other) {}

	CD3DX12_ROOT_DESCRIPTOR_TABLE(UINT numDescriptorRanges, _In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE *descriptorRanges)
	{
		Init(numDescriptorRanges, descriptorRanges);
	}
	inline void Init(UINT numDescriptorRanges, _In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE *descriptorRanges)
	{
		Init(*this, numDescriptorRanges, descriptorRanges);
	}
	static inline void Init(
		_Out_ D3D12_ROOT_DESCRIPTOR_TABLE &rootDescriptorTable,
		UINT numDescriptorRanges,
		_In_reads_opt_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE *descriptorRanges)
	{
		rootDescriptorTable.NumDescriptorRanges = numDescriptorRanges;
		rootDescriptorTable.pDescriptorRanges = descriptorRanges;
	}
};

struct CD3DX12_ROOT_PARAMETER : public D3D12_ROOT_PARAMETER
{
	CD3DX12_ROOT_PARAMETER() {}
	explicit CD3DX12_ROOT_PARAMETER(const D3D12_ROOT_PARAMETER &other) : D3D12_ROOT_PARAMETER(other) {}

	static inline void InitAsDescriptorTable(
		_Out_ D3D12_ROOT_PARAMETER &rootParam,
		UINT numDescriptorRanges,
		_In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE *descriptorRanges,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = visibility;
		CD3DX12_ROOT_DESCRIPTOR_TABLE::Init(rootParam.DescriptorTable, numDescriptorRanges, descriptorRanges);
	}
	inline void InitAsDescriptorTable(
		UINT numDescriptorRanges,
		_In_reads_(numDescriptorRanges) const D3D12_DESCRIPTOR_RANGE *descriptorRanges,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsDescriptorTable(*this, numDescriptorRanges, descriptorRanges, visibility);
	}
};

struct CD3DX12_ROOT_SIGNATURE_DESC : public D3D12_ROOT_SIGNATURE_DESC
{
	CD3DX12_ROOT_SIGNATURE_DESC() {}
	explicit CD3DX12_ROOT_SIGNATURE_DESC(const D3D12_ROOT_SIGNATURE_DESC &other) : D3D12_ROOT_SIGNATURE_DESC(other) {}

	CD3DX12_ROOT_SIGNATURE_DESC(
		UINT numParams,
		_In_reads_opt_(numParams) const D3D12_ROOT_PARAMETER *parameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC *staticSamplers = NULL,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		Init(numParams, parameters, numStaticSamplers, staticSamplers, flags);
	}
	CD3DX12_ROOT_SIGNATURE_DESC(CD3DX12_DEFAULT)
	{
		Init(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);
	}
	inline void Init(
		UINT numParams,
		_In_reads_opt_(numParams) const D3D12_ROOT_PARAMETER *parameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC *staticSamplers = NULL,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		Init(*this, numParams, parameters, numStaticSamplers, staticSamplers, flags);
	}
	static inline void Init(
		_Out_ D3D12_ROOT_SIGNATURE_DESC &desc,
		UINT numParams,
		_In_reads_opt_(numParams) const D3D12_ROOT_PARAMETER *parameters,
		UINT numStaticSamplers = 0,
		_In_reads_opt_(numStaticSamplers) const D3D12_STATIC_SAMPLER_DESC *staticSamplers = NULL,
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		desc.NumParameters = numParams;
		desc.pParameters = parameters;
		desc.NumStaticSamplers = numStaticSamplers;
		desc.pStaticSamplers = staticSamplers;
		desc.Flags = flags;
	}
};


//! ================================================   ������ ��� D3D12_RASTERIZER_DESC   ================================================
/*
   ��������� �������������� �������� ������������� �� ��������� ��� ����������������.													 */
struct CD3DX12_RASTERIZER_DESC : public D3D12_RASTERIZER_DESC
{
	CD3DX12_RASTERIZER_DESC() {}
	explicit CD3DX12_RASTERIZER_DESC(const D3D12_RASTERIZER_DESC &other) : D3D12_RASTERIZER_DESC(other) {}
	explicit CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT)
	{
		FillMode = D3D12_FILL_MODE_SOLID;
		CullMode = D3D12_CULL_MODE_BACK;
		FrontCounterClockwise = false;
		DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		DepthClipEnable = true;
		MultisampleEnable = false;
		AntialiasedLineEnable = false;
		ForcedSampleCount = 0;
		ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}
	explicit CD3DX12_RASTERIZER_DESC(
		D3D12_FILL_MODE fillMOde,
		D3D12_CULL_MODE cullMode,
		BOOL frontCounterClockwise,
		INT depthBias,
		FLOAT depthBiasClamp,
		FLOAT slopeScaledDepthBias,
		BOOL depthClipEnable,
		BOOL multisampleEnable,
		BOOL antialiasedLineEnable,
		UINT forcedSampleCount,
		D3D12_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster)
	{
		FillMode = fillMOde;
		CullMode = cullMode;
		FrontCounterClockwise = frontCounterClockwise;
		DepthBias = depthBias;
		DepthBiasClamp = depthBiasClamp;
		SlopeScaledDepthBias = slopeScaledDepthBias;
		DepthClipEnable = depthClipEnable;
		MultisampleEnable = multisampleEnable;
		AntialiasedLineEnable = antialiasedLineEnable;
		ForcedSampleCount = forcedSampleCount;
		ConservativeRaster = conservativeRaster;
	}
	~CD3DX12_RASTERIZER_DESC() {}
	operator const D3D12_RASTERIZER_DESC&() const { return *this; }
};


//! ==================================================   ������ ���  D3D12_BLEND_DESC   ==================================================
/*
	��������� �������������� �������� �������� �� ���������.																			 */
struct CD3XD12_BLEND_DESC : public D3D12_BLEND_DESC
{
	CD3XD12_BLEND_DESC() {}
	explicit CD3XD12_BLEND_DESC(const D3D12_BLEND_DESC &other) : D3D12_BLEND_DESC(other) {}
	explicit CD3XD12_BLEND_DESC(CD3DX12_DEFAULT)
	{
		AlphaToCoverageEnable = FALSE;
		IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetRenderDesc =
		{
			FALSE, FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			RenderTarget[i] = defaultRenderTargetRenderDesc;
	}
	~CD3XD12_BLEND_DESC() {}
	operator const D3D12_BLEND_DESC&() const { return *this; }
};


//! =============================================   ������ ��� CD3DX12_DEPTH_STENCIL_DESC   ==============================================
/*
	��������� �������������� �������� ������ �������/��������� �� ��������� ��� ������������������										 */
struct CD3DX12_DEPTH_STENCIL_DESC : public D3D12_DEPTH_STENCIL_DESC
{
	CD3DX12_DEPTH_STENCIL_DESC() {}
	explicit CD3DX12_DEPTH_STENCIL_DESC(const D3D12_DEPTH_STENCIL_DESC &other) : D3D12_DEPTH_STENCIL_DESC(other) {}
	explicit CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT)
	{
		DepthEnable = TRUE;
		DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		StencilEnable = false;
		StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{
			D3D12_STENCIL_OP_KEEP, 
			D3D12_STENCIL_OP_KEEP, 
			D3D12_STENCIL_OP_KEEP, 
			D3D12_COMPARISON_FUNC_ALWAYS
		};
		FrontFace = defaultStencilOp;
		BackFace = defaultStencilOp;
	}
	explicit CD3DX12_DEPTH_STENCIL_DESC(
		BOOL depthEnable,
		D3D12_DEPTH_WRITE_MASK depthWriteMask,
		D3D12_COMPARISON_FUNC depthFunc,
		BOOL stencilEnable,
		UINT8 stencilReadMask,
		UINT8 stencilWriteMask,
		D3D12_STENCIL_OP frontStencilFailOp,
		D3D12_STENCIL_OP frontStencilDepthFailOp,
		D3D12_STENCIL_OP frontStencilPassOp,
		D3D12_COMPARISON_FUNC frontStencilFunc,
		D3D12_STENCIL_OP backStencilFailOp,
		D3D12_STENCIL_OP backStencilDepthFailOp,
		D3D12_STENCIL_OP backStencilPassOp,
		D3D12_COMPARISON_FUNC backStencilFunc)
	{
		DepthEnable = depthEnable;
		DepthWriteMask = depthWriteMask;
		DepthFunc = depthFunc;
		StencilEnable = stencilEnable;
		StencilReadMask = stencilReadMask;
		StencilWriteMask = stencilWriteMask;
		FrontFace.StencilFailOp = frontStencilFailOp;
		FrontFace.StencilDepthFailOp = frontStencilDepthFailOp;
		FrontFace.StencilPassOp = frontStencilPassOp;
		FrontFace.StencilFunc = frontStencilFunc;
		BackFace.StencilFailOp = backStencilFailOp;
		BackFace.StencilDepthFailOp = backStencilDepthFailOp;
		BackFace.StencilPassOp = backStencilPassOp;
		BackFace.StencilFunc = backStencilFunc;
	}
	~CD3DX12_DEPTH_STENCIL_DESC() {}
	operator const D3D12_DEPTH_STENCIL_DESC&() const { return *this; }
};


//! ==================================================   ����� ���������������  ������   ==================================================
class Util
{
public:
	static ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, const void *initData, UINT64 byteSize, ComPtr<ID3D12Resource>&uploadBuffer);

	static UINT CalcConstantBufferByteSize(UINT byteSize) { return (byteSize + 255) & ~255; }

	static ComPtr<ID3DBlob> LoadBinary(const std::wstring &fileName);

	static ComPtr<ID3DBlob> CompileShader(const std::wstring &file, const D3D_SHADER_MACRO *defs, const std::string &entry, const std::string &target);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ������� ���  ������ ������� � GPU   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//inline UINT64 UpdateSubresources(_In_ ID3D12GraphicsCommandList *cmdList,
//							 	 _In_ ID3D12Resource *destinationResource,
//								 _In_ ID3D12Resource *sourceResource,
//								 _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT firstSubresource,
//								 _In_range_(0, D3D12_REQ_SUBRESOURCES - firstSubresource) UINT numSubresources,
//								 UINT64 requiredSize,
//								 _In_reads_(numSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts,
//								 _In_reads_(numSubresources) const UINT *numRows,
//								 _In_reads_(numSubresources) const UINT64 *rowSizesInBytes,
//								 _In_reads_(numSubresources) const D3D12_SUBRESOURCE_DATA *sourceData) {
//	D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
//	D3D12_RESOURCE_DESC destinDesc = destinationResource->GetDesc();
//	if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || sourceDesc.Width < requiredSize + layouts[0].Offset || requiredSize >(SIZE_T) - 1 ||
//		(destinDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && (firstSubresource != 0 || numSubresources != 1))) {
//		return 0;
//	}
//	BYTE *data;
//	if (FAILED(sourceResource->Map(0, NULL, reinterpret_cast<void**>(&data))))
//		return 0;
//	for (UINT i = 0; i < numSubresources; ++i) {
//		if (rowSizesInBytes[i] >(SIZE_T)-1)
//			return 0;
//		D3D12_MEMCPY_DEST destData = { data + layouts[i].Offset, layouts[i].Footprint.RowPitch, layouts[i].Footprint.RowPitch * numRows[i] };
//		MemcpySubresource(&destData, &sourceData[i], (SIZE_T)rowSizesInBytes[i], numRows[i], layouts[i].Footprint.Depth);
//	}
//	sourceResource->Unmap(0, NULL);
//	if (destinDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
//		CD3DX12_BOX srcBox(UINT(layouts[0].Offset), UINT(layouts[0].Offset + layouts[0].Footprint.Width));
//		cmdList->CopyBufferRegion(destinationResource, 0, sourceResource, layouts[0].Offset, layouts[0].Footprint.Width);
//	}
//	else {
//		for (UINT i = 0; i < numSubresources; ++i) {
//			CD3DX12_TEXTURE_COPY_LOCATION Dst(destinationResource, i + firstSubresource);
//			CD3DX12_TEXTURE_COPY_LOCATION Src(sourceResource, layouts[i]);
//			cmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
//		}
//	}
//	return requiredSize;
//}

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
	//return UpdateSubresources(cmdList, destinationResource, sourceResource, firstSubresource, numSubresources, requiredSize, layouts, numRows, rowSizesInBytes, sourceData);
}


inline UINT D3D12CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize)
{
	return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
}


inline UINT8 D3D12GetFormatPlaneCount(_In_ ID3D12Device *pDevice, DXGI_FORMAT format) {
	D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = { format };
	if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo))))
		return 0;
	return formatInfo.PlaneCount;
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


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ��������������� ������, ���������   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
template<UINT8 numSlots> struct MeshGeometry;

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
	MeshGeometry<1> *geometry = nullptr;
};


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
template<UINT8 numSlots> struct MeshGeometry
{
	struct VBufferMetrics {
		UINT VertexByteStride = 0;
		UINT VertexBufferByteSize = 0;
	};

	std::string Name;
	ComPtr<ID3DBlob> VBufferCPU[numSlots];						// ��������� ������, ��������� CPU (��� ���������� �������� � �.�.)
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
	ComPtr<ID3D12Resource> VBufferGPU[numSlots];				// ��������� ������, ��������� GPU (���������� ��� ���������)
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	VBufferMetrics vbMetrics[numSlots];							// Stride � ByteSize ���������������� ���������� ������
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;				// ������ ���������� ������
	UINT IndexBufferByteSize = 0;

	D3D12_VERTEX_BUFFER_VIEW VBufferViews[numSlots];  // ����������� ��������� ������� (������������ � Draw() ������� IASetVertexBuffers())
	D3D12_INDEX_BUFFER_VIEW IBufferView;			  // ���������� ���������� ������ (������������ � Draw() ������� IASetIndexBuffer())
	bool vbViewInited = false;
	bool ibViewInited = false;
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;	// ������������ ������� � ������� (��������)

	//! ����� ������ ������������ ��������� �������, �������� ��� �������������
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

	//! ����� ���������� ���������� ������, �������� ��� �������������
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


//! ========================================================   �����  ����������   ========================================================
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   �������   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ��������������� �������   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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