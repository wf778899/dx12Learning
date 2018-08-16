cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float4 gPulseColor;
	float gTime;
};

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
	//vin.PosL.xy += 0.5 * sin(vin.PosL.x) * sin(3.0 * gTime);
	//vin.PosL.z *= 0.6f + 0.4 * sin(2.0f * gTime);
	VertexOut vout;
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	vout.Color = vin.Color;
	return vout;
}