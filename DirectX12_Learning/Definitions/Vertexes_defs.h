#pragma once

#include <DirectXMath.h>
#include <DirectXPackedVector.h>


using namespace DirectX;
using namespace DirectX::PackedVector;


struct Vertex
{
	Vertex(XMFLOAT3 pos, XMCOLOR Col)
		: Position(pos)
		, Color(Col) 
	{}

	Vertex()
		: Position(XMFLOAT3())
		, Color(DirectX::Colors::DarkGreen)
	{}

	XMFLOAT3 Position;
	XMCOLOR Color;
};