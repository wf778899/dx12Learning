struct VertexIn
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

float4 PS(VertexIn vin) : SV_TARGET
{
	return vin.Color;
}