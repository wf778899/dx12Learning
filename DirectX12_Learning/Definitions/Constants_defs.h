#pragma once

#include "../Helpers/MathHelper.h"


const XMFLOAT4X4 g_unitMatrix = MathHelper::Identity4x4();


/* Пообъектные константы */
struct ObjectConstants
{
	XMFLOAT4X4 world = g_unitMatrix;
};


/* Pass-константы */
struct PassConstants
{
	XMFLOAT4X4 view = g_unitMatrix;
	XMFLOAT4X4 proj = g_unitMatrix;
	XMFLOAT4X4 viewProj = g_unitMatrix;
	XMFLOAT4X4 inv_view = g_unitMatrix;
	XMFLOAT4X4 inv_proj = g_unitMatrix;
	XMFLOAT4X4 inv_viewProj = g_unitMatrix;
	XMFLOAT3 eyePos_w = { 0.0f, 0.0f, 0.0f };
	float cb_perObjectPad1 = 0.0f;
	XMFLOAT2 renderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 inv_renderTargetSize = { 0.0f, 0.0f };
	float nearZ = 1.0f;
	float farZ = 1000.0f;
	float totalTime = 0.0f;
	float deltaTime = 0.0f;
};