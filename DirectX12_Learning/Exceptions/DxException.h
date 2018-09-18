#pragma once

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



#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
	HRESULT hr = (x);													\
	std::wstring fileName = AnsiToWString(__FILE__);					\
	if(FAILED(hr)) { throw DxException(hr, L#x, fileName, __LINE__); }	\
}
#endif