#pragma once

#include "Helpers/MathHelper.h"
#include "Helpers/Utilites.h"
#include "UploadBuffer.h"

struct Constants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 gPulseColor;
};

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
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

struct FrameResources
{
public:
	FrameResources();
	~FrameResources();
};

