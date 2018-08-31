#pragma once

#include "Helpers/Utilites.h"
#include "UploadBuffer.h"

struct Constants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 gPulseColor;
};

struct Vertex
{
	Vertex(XMFLOAT3 pos, XMCOLOR Col)
		: Position(pos)
		, Color(Col) 
	{}
	XMFLOAT3 Position;
	XMCOLOR Color;
};

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

/* Пообъектные константы */
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4();
};

/* Общие константы */
struct PassConstants
{
	XMFLOAT4X4 view = MathHelper::Identity4x4();
	XMFLOAT4X4 proj = MathHelper::Identity4x4();
	XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
	XMFLOAT4X4 inv_view = MathHelper::Identity4x4();
	XMFLOAT4X4 inv_proj = MathHelper::Identity4x4();
	XMFLOAT4X4 inv_viewProj = MathHelper::Identity4x4();
	XMFLOAT3 eyePos_w = { 0.0f, 0.0f, 0.0f };
	float cb_perObjectPad1 = 0.0f;
	XMFLOAT2 renderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 inv_renderTargetSize = { 0.0f, 0.0f };
	float nearZ = 1.0f;
	float farZ = 1000.0f;
	float totalTime = 0.0f;
	float deltaTime = 0.0f;
};


//! =======================================================   Покадровые  ресурсы   =======================================================
/*
   Циклический массив фрэйм-ресурсов используется для  эффективной загрузки GPU.  В то время, как  GPU выполняет список команд для кадра N,
используя при этом константы для этого кадра, CPU может не ждать и готовить константы и список команд для N+1 кадра. Массив  на 3  элемента
обеспечивает GPU данными на 3 кадра вперёд, если CPU работает быстрее.													(стр. 389 - 394) */
struct FrameResources
{
public:
	FrameResources(ID3D12Device *device, UINT passCB_count, UINT objectCB_count);
	~FrameResources();
	FrameResources(const FrameResources&) = delete;
	FrameResources& operator=(const FrameResources&) = delete;

	/* У каждого фрэйм-ресурса свой аллокатор команд */
	ComPtr<ID3D12CommandAllocator> cmdAllocator;

	/* У каждого фрэйм-ресурса свои константные буферы */
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;

	/* Следим, отработала ли GPU этот кадр. В Draw() значение fence инкрементится и ставится в очередь команд. В Update() проверяется fence
	текущего кадра и сравнивается с отработанным в очереди. Если оно больше - значит циклический массив пройден по кругу и  мы  упёрлись  в
	неотработанный кадр. */
	UINT64 fenceVal = 0;
};

