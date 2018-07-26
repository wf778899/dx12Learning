#pragma once

#include "D3D12Base.h"
#include "UploadBuffer.h"
#include "Helpers/MathHelper.h"

struct Constants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class D3D12Engine : public D3D12Base
{
public:
	D3D12Engine(HINSTANCE hInstance);
	~D3D12Engine();
	D3D12Engine(const D3D12Engine &other) = delete;
	D3D12Engine &operator=(const D3D12Engine &other) = delete;

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer &timer) override;
	virtual void Draw(const GameTimer &timer) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();

	ComPtr<ID3D12DescriptorHeap> m_CBV_heap = nullptr;
	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	std::unique_ptr<MeshGeometry> m_boxGeometry = nullptr;
	std::unique_ptr<UploadBuffer<Constants>> m_constantBuffer = nullptr;

	POINT m_lastMousePos;
	float m_theta = 1.5f * XM_PI;
	float m_phi = XM_PIDIV4;
	float m_radius = 5.0f;
};

