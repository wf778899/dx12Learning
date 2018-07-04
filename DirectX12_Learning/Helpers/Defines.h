#pragma once

#define DEBUG

#include <d3d12.h>

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