#pragma once

using namespace DirectX;
using namespace DirectX::PackedVector;

class MathHelper
{
public:

	static XMFLOAT4X4 Identity4x4() {
		static XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		return I;
	}

	template <typename T> static T Clamp(const T &x, const T &low, const T &high) {
		return x < low ? low : (x > high ? high : x);
	}

	static const float Pi;
};