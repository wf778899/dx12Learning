cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float4 gPulseColor;
	float gTime;
};

cbuffer cbPerFrame : register(b1)
{
	float gTime2;
}

struct VertexIn
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

float4 PS(VertexIn pin) : SV_Target
{
	const float pi = 3.14159;
	float s = 0.5f * sin(2 * gTime2 - 0.25f * pi) + 0.5f;

	float4 c = lerp(pin.Color, gPulseColor, s);
	return c;
}