#pragma once

#include "D3D12Base.h"

#include "Definitions/Meshes_wraps.h"
#include "Definitions/Constants_defs.h"

class FrameResources;
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
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnKeyboardInput(const GameTimer &timer);

	virtual void Draw(const GameTimer &timer) override;
	virtual void Update(const GameTimer &timer) override;

	void UpdateCamera(const GameTimer &timer);
	void UpdateObjectCB(const GameTimer &timer);
	void UpdatePassCB(const GameTimer &timer);

	void BuildRootSignature();
	void CreateCBVdescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildConstantBuffers();
	void BuildPSO();

	std::vector<std::unique_ptr<RenderItem>> m_allRenderItems;		// Массив всех моделей для рендеринга
	std::vector<RenderItem*> m_opaqueRenderItems;					// Массив непрозрачных моделей (подмножество всех моделей)
	std::vector<std::unique_ptr<FrameResources>> m_frameResources;	// Массив покадровых ресурсов
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders_Vertex;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders_Pixel;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_pipelineState;

	ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;				// Новая куча дескрипторов
	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

	FrameResources *m_currFrame = nullptr;
	PassConstants m_passConstant;

	POINT m_lastMousePos;
	UINT m_currFrameIndex = 0;
	UINT m_passCBVoffset = 0;		// Начало области дескрипторов на покадровые константы

	XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	XMFLOAT4X4 m_proj = MathHelper::Identity4x4();
	XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };

	float m_theta = 1.5f * XM_PI;
	float m_phi = 0.2f * XM_PI;
	float m_radius = 15.0f;
	bool m_wireFrame = false;
};

