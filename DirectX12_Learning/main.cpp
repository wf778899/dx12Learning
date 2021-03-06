#include "stdafx.h"

#include "D3D12Engine.h"
#include "Exceptions/DxException.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	D3D12Engine *d3d12Engine = new D3D12Engine(hInstance);
	int exitCode = -1;
	try {
		if (d3d12Engine->Initialize())
			exitCode = d3d12Engine->Run();
	}
	catch (DxException e) {
		MessageBox(nullptr, e.toString().c_str(), L"HR_FAILED", MB_OK);
		exitCode = -1;
	}
	
	delete d3d12Engine;
	return exitCode;
}

