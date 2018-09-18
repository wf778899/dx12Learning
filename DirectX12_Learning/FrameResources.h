#pragma once

#include "UploadBuffer.h"

struct ObjectConstants;
struct PassConstants;

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

