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

struct VertexIn
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

float4 PS(VertexIn pin) : SV_Target
{
	return pin.Color;
}