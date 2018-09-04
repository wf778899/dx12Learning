#pragma once

#include "D3D12Base.h"

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

	void OnKeyboardInput(const GameTimer &timer);
	void UpdateCamera(const GameTimer &timer);
	void UpdateObjectCB(const GameTimer &timer);
	void UpdatePassCB(const GameTimer &timer);

	void CreateCbvDescriptorHeaps();
	void CreateCBVdescriptorHeaps();
	void BuildConstantBuffers();
	void Build_ConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSO();
	void BuildBoxGeometry();
	void BuildGeometry();
	void BuildRenderItems();
	void BuildFrameResources();

	std::vector<std::unique_ptr<RenderItem>> m_allRenderItems;		// Массив всех моделей для рендеринга
	std::vector<RenderItem*> m_opaqueRenderItems;					// Массив непрозрачных моделей (подмножество всех моделей)
	std::vector<std::unique_ptr<FrameResources>> m_frameResources;	// Массив покадровых ресурсов
	UINT m_currFrameIndex = 0;
	UINT m_passCBVoffset = 0;		// Начало области дескрипторов на покадровые константы

	FrameResources *m_currFrame = nullptr;

	PassConstants m_passConstant;

	ComPtr<ID3D12DescriptorHeap> m_CBV_heap = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;			// Новая куча дескрипторов
	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	//ComPtr<ID3D12PipelineState> m_pipelineState = nullptr;
	ComPtr<ID3DBlob> m_vsByteCode = nullptr;
	ComPtr<ID3DBlob> m_psByteCode = nullptr;
	std::unique_ptr<MeshGeometry<2>> m_boxGeometry = nullptr;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry<1>>> m_geometries;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders_Vertex;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders_Pixel;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_pipelineState;

	std::unique_ptr<UploadBuffer<Constants>> m_constant_po_buf = nullptr;	// Константы пообъектные
	std::unique_ptr<UploadBuffer<float>> m_constant_pf_buf = nullptr;		// Константы покадровые

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4();
	XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
	POINT m_lastMousePos;
	float m_theta = 1.5f * XM_PI;
	float m_phi = 0.2f * XM_PI;
	float m_radius = 15.0f;
	bool m_wireFrame = false;
};

