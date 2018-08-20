#pragma once

#include "Helpers/Utilites.h"

template<typename T> class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device *device, UINT numElements, bool isConstantBuffer)
		: m_isConstantBuffer(isConstantBuffer)
	{
		m_elementByteSize = sizeof(T);
		if (isConstantBuffer) {
			m_elementByteSize = Util::CalcConstantBufferByteSize(sizeof(T));
		}
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * numElements),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_uploadBuffer)));
		ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
	}
	~UploadBuffer()
	{
		if (m_uploadBuffer != nullptr)
			m_uploadBuffer->Unmap(0, nullptr);
		m_mappedData = nullptr;
	}
	UploadBuffer(const UploadBuffer &other) = delete;
	UploadBuffer &operator=(const UploadBuffer &other) = delete;

	ID3D12Resource *Resource() const { return m_uploadBuffer.Get(); }

	void CopyData(int elementIndex, const T &data) {
		memcpy(&m_mappedData[elementIndex * m_elementByteSize], &data, sizeof(T));
	}

	UINT elementByteSize() const { return m_elementByteSize; }
private:
	UINT m_elementByteSize = 0;
	bool m_isConstantBuffer = false;
	ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE *m_mappedData = nullptr;
};

