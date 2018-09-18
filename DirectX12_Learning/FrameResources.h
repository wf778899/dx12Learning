#pragma once

#include "UploadBuffer.h"

struct ObjectConstants;
struct PassConstants;

//! =======================================================   ����������  �������   =======================================================
/*
   ����������� ������ �����-�������� ������������ ���  ����������� �������� GPU.  � �� �����, ���  GPU ��������� ������ ������ ��� ����� N,
��������� ��� ���� ��������� ��� ����� �����, CPU ����� �� ����� � �������� ��������� � ������ ������ ��� N+1 �����. ������  �� 3  ��������
������������ GPU ������� �� 3 ����� �����, ���� CPU �������� �������.													(���. 389 - 394) */
struct FrameResources
{
public:
	FrameResources(ID3D12Device *device, UINT passCB_count, UINT objectCB_count);
	~FrameResources();
	FrameResources(const FrameResources&) = delete;
	FrameResources& operator=(const FrameResources&) = delete;

	/* � ������� �����-������� ���� ��������� ������ */
	ComPtr<ID3D12CommandAllocator> cmdAllocator;

	/* � ������� �����-������� ���� ����������� ������ */
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;

	/* ������, ���������� �� GPU ���� ����. � Draw() �������� fence ������������� � �������� � ������� ������. � Update() ����������� fence
	�������� ����� � ������������ � ������������ � �������. ���� ��� ������ - ������ ����������� ������ ������� �� ����� �  ��  �������  �
	�������������� ����. */
	UINT64 fenceVal = 0;
};

