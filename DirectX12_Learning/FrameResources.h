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
	// 1 ����
	struct PosTex {
		XMFLOAT3 Pos;
		XMFLOAT2 Tex;
	};
	// 2 ����
	struct NorTanCol {
		XMFLOAT3 Normal;
		XMFLOAT3 Tangent;
		XMCOLOR Color;
	};
	PosTex pos_tex;
	NorTanCol nor_tan_col;
};

/* ����������� ��������� */
struct ObjectConstants
{
	XMFLOAT4X4 world = MathHelper::Identity4x4();
};

/* ����� ��������� */
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

