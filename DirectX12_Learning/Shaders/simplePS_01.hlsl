//cbuffer cbPerObject : register(b0)
//{
//	float4x4 gWorldViewProj;
//	float4 gPulseColor;
//};
//
//cbuffer cbPerFrame : register(b1)
//{
//	float gTime;
//}

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
}

cbuffer cbPerPass : register(b1)
{
	float4x4 gView;
	float4x4 gProj;
	float4x4 gViewProj;
	float4x4 gInv_view;
	float4x4 gInv_proj;
	float4x4 gInv_viewProj;
	float3 gEyePos_w;
	float gCb_perObjectPad1;
	float2 gRenderTargetSize;
	float2 gInv_renderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
}
//cbuffer cbPerFrame2 : register(b2)
//{
//	float gFactor;
//}

struct VertexIn
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

float4 PS(VertexIn pin) : SV_Target
{
	//const float pi = 3.14159;
	//float s = 0.5f * sin(2 * gTotalTime - 0.25f * pi) + 0.5f;

	//float4 c = lerp(pin.Color, gPulseColor, s);
	return pin.Color;
}