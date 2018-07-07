#include "stdafx.h"
#include "D3D12Engine.h"


D3D12Engine::D3D12Engine(HINSTANCE hInstance) : D3D12Base(hInstance)
{
}


D3D12Engine::~D3D12Engine()
{
}


bool D3D12Engine::Initialize()
{
	if(!D3D12Base::Initialize())
		return false;
	return true;
}

void D3D12Engine::OnResize()
{
	D3D12Base::OnResize();
}

void D3D12Engine::Update(const GameTimer & timer)
{
}

void D3D12Engine::Draw(const GameTimer & timer)
{
	ThrowIfFailed(m_cmdAllocator->Reset());
	ThrowIfFailed(m_cmdList->Reset(m_cmdAllocator.Get(), nullptr));
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	m_cmdList->RSSetViewports(1, &m_viewPort);
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);
	m_cmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::DarkViolet, 0, nullptr);
	m_cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(m_cmdList->Close());

	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	ThrowIfFailed(m_swapChain->Present(0, 0));

	m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBuffersCount;

	FlushCommandQueue();
}
