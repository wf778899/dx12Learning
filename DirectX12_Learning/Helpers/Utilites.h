#pragma once

#define DEBUG

using Microsoft::WRL::ComPtr;

//==============================================================   Ã¿ –Œ—€   ==============================================================
#define SC_ALLOW_MODE_SWITCH DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH

#ifndef ReleaseCom
#define ReleaseCom(x)				\
{									\
	if(x) {							\
		x->Release();				\
		x = 0;						\
	}								\
}
#endif


//! ==================================================    Î‡ÒÒ ‚ÒÔÓÏÓ„‡ÚÂÎ¸Ì˚ı  ÛÚËÎËÚ   ==================================================
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
template <UINT MaxSubresources> 
inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList *cmdList, _In_ ID3D12Resource *destinationResource, _In_ ID3D12Resource *sourceResource, UINT64 sourceOffset,
	_In_range_(0, MaxSubresources) UINT firstSubresource,  _In_range_(1, MaxSubresources - firstSubresource) UINT numSubresources,
	_In_reads_(numSubresources) D3D12_SUBRESOURCE_DATA *sourceData);


inline void MemcpySubresource(
	_In_ const D3D12_MEMCPY_DEST *dest, _In_ const D3D12_SUBRESOURCE_DATA *srcData, SIZE_T rowSizeInBytes, UINT numRows, UINT numSlices);



//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ¬—œŒÃŒ√¿“≈À‹Õ€≈ ‘”Õ ÷»»   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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