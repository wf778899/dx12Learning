struct VertexIn
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

float4 PS(VertexIn pin) : SV_Target
{
	return pin.Color;
}