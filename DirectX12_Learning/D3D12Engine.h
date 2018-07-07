#pragma once

#include "D3D12Base.h"

class D3D12Engine : public D3D12Base
{
public:
	D3D12Engine(HINSTANCE hInstance);
	~D3D12Engine();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer &timer) override;
	virtual void Draw(const GameTimer &timer) override;
};

