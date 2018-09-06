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
	float3 PosL : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosH = mul(posW, gViewProj);

	vout.Color = float4(vin.Color);
	return vout;
}