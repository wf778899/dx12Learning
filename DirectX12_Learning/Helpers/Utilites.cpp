#include "../stdafx.h"

#include "d3dx12.h"
#include "Utilites.h"
#include "../Exceptions/DxException.h"

template <UINT MaxSubresources> UINT64 UpdateSubresources(
															_In_ ID3D12GraphicsCommandList *cmdList,
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


void MemcpySubresource(_In_ const D3D12_MEMCPY_DEST *dest, _In_ const D3D12_SUBRESOURCE_DATA *srcData, SIZE_T rowSizeInBytes, UINT numRows, UINT numSlices)
{
	for (UINT i = 0; i < numSlices; ++i) {
		BYTE *destSlice = reinterpret_cast<BYTE*>(dest->pData) + dest->SlicePitch * i;
		const BYTE *srcSlice = reinterpret_cast<const BYTE*>(srcData->pData) + srcData->SlicePitch * i;
		for (UINT k = 0; k < numRows; ++k) {
			memcpy(destSlice + dest->RowPitch * k, srcSlice + srcData->RowPitch * k, rowSizeInBytes);
		}
	}
}


//! =======================================================   CreateDefaultBuffer   =======================================================
/*
   Создаёт буфер в GPU без возможности доступа со стороны CPU  (D3D12_HEAP_TYPE_DEFAULT).  Инициализирует  его  данными.  Используется  при
создании вершинного и индексного буферов.																								 */
ComPtr<ID3D12Resource> Util::CreateDefaultBuffer(ID3D12Device *device, 
												 ID3D12GraphicsCommandList *cmdList,
												 const void *initData, 
												 UINT64 byteSize, 
												 ComPtr<ID3D12Resource> &uploadBuffer)
{
	/* Сперва создаём default-буфер (ресурс) нужного размера. Для получения его описания используем CD3DX12_RESOURCE_DESC::Buffer().	 */
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
												  D3D12_HEAP_FLAG_NONE,
												  &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
												  D3D12_RESOURCE_STATE_COMMON,
												  nullptr,
												  IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
	/* Теперь создаём upload-буфер, в который потом закинем данные из CPU. */
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
												  D3D12_HEAP_FLAG_NONE,
												  &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
												  D3D12_RESOURCE_STATE_GENERIC_READ,
												  nullptr,
												  IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
	/* Описание данных, которыми инициализируем default-буфер. */
	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = initData;
	subresourceData.RowPitch = byteSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	
	UpdateSubresources<1>(cmdList,
						  defaultBuffer.Get(),	// Ресурс-назначение
						  uploadBuffer.Get(),	// Ресурс-источник
						  0,					// Смещение данных в источнике
						  0,					// Первый субресурс
						  1,					// Число субресурсов
						  &subresourceData);	// Данные
	
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}



//! =================================================   Создание ID3DBlob из .cso-файла   =================================================
/*
   Просто открывает в бинарном режиме файл предварительно скомпилированного шейдера и загружает его в Blob.								 */
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



//! =================================================   Создание ID3DBlob из .txt-файла   =================================================
/*
   Открывает файл с кодом шейдера, компилирует его в рантайме и загружает в Blob.														  */
ComPtr<ID3DBlob> Util::CompileShader(
	const std::wstring &file, const D3D_SHADER_MACRO *defs, const std::string &entry, const std::string &target)
{
	UINT compileFlags = 0;

#ifdef DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = S_OK;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> err;
	hr = D3DCompileFromFile(
		file.c_str(), defs, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry.c_str(), target.c_str(), compileFlags, 0, &byteCode, &err);
	if (err != nullptr) {
		OutputDebugStringA((char*)err->GetBufferPointer());
		ThrowIfFailed(-1);
	}
	return byteCode;
}
