#pragma once

#include "D3D12Base.h"
#include "UploadBuffer.h"
#include "Helpers/MathHelper.h"

struct Constants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 gPulseColor;
	float gTime;
};

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

 //Этот тип вершин будет передаваться в GPU через 2 слота
struct SeparatedVertex {
	// 1 слот
	struct PosTex {
		XMFLOAT3 Pos;
		XMFLOAT2 Tex;
	};
	// 2 слот
	struct NorTanCol {
		XMFLOAT3 Normal;
		XMFLOAT3 Tangent;
		XMCOLOR Color;
	};
	PosTex pos_tex;
	NorTanCol nor_tan_col;
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

	void CreateCbvDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSO();
	void BuildBoxGeometry();

	ComPtr<ID3D12DescriptorHeap> m_CBV_heap = nullptr;
	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	ComPtr<ID3D12PipelineState> m_pipelineState = nullptr;
	ComPtr<ID3DBlob> m_vsByteCode = nullptr;
	ComPtr<ID3DBlob> m_psByteCode = nullptr;
	std::unique_ptr<MeshGeometry<2>> m_boxGeometry = nullptr;
	std::unique_ptr<UploadBuffer<Constants>> m_constantBuffer = nullptr;
	std::unique_ptr<UploadBuffer<float>> m_constantBuffer2 = nullptr;/////////////

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

	POINT m_lastMousePos;
	float m_theta = 1.5f * XM_PI;
	float m_phi = XM_PIDIV4;
	float m_radius = 5.0f;
};

