#include "../stdafx.h"
#include "Utilites.h"

ComPtr<ID3D12Resource> Util::CreateDefaultBuffer(ID3D12Device *device, 
												 ID3D12GraphicsCommandList *cmdList,
												 const void *initData, 
												 UINT64 byteSize, 
												 ComPtr<ID3D12Resource> &uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
												  D3D12_HEAP_FLAG_NONE,
												  &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
												  D3D12_RESOURCE_STATE_COMMON,
												  nullptr,
												  IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
												  D3D12_HEAP_FLAG_NONE,
												  &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
												  D3D12_RESOURCE_STATE_GENERIC_READ,
												  nullptr,
												  IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = initData;
	subresourceData.RowPitch = byteSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subresourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}

ComPtr<ID3DBlob> Util::LoadBinary(const std::wstring &fileName)
{
	std::ifstream fin(fileName, std::ios::binary);
	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	ComPtr<ID3D10Blob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read((char*)blob->GetBufferPointer(), size);
	fin.close();
	return blob;
}

ComPtr<ID3DBlob> Util::CompileShader(const std::wstring &file, const D3D_SHADER_MACRO *defs, const std::string &entry, const std::string &target)
{
	UINT compileFlags = 0;

#ifdef DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = S_OK;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> err;
	hr = D3DCompileFromFile(file.c_str(), defs, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry.c_str(), target.c_str(), compileFlags, 0, &byteCode, &err);
	if (err != nullptr) {
		OutputDebugStringA((char*)err->GetBufferPointer());
		ThrowIfFailed(-1);
	}
	return byteCode;
}
