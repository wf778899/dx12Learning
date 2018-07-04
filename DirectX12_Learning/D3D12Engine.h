#pragma once

#include "D3D12Base.h"

class D3D12Engine : public D3D12Base
{
public:
	D3D12Engine(HINSTANCE hInstance);
	~D3D12Engine();

	virtual bool Initialize() override;
};

