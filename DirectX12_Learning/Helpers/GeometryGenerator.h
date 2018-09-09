#pragma once
#include "MathHelper.h"
#include <cstdint>

class GeometryGenerator
{
public:
	GeometryGenerator();
	~GeometryGenerator();


	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	struct Vertex
	{
		Vertex() {}
		Vertex(const XMFLOAT3 &pos, const XMFLOAT3 &nor, const XMFLOAT3 &tan, const XMFLOAT2 &tex)
			: position(pos)
			, normal(nor)
			, tangent(tan)
			, texCoord(tex) {}

		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT2 texCoord;
	};

	struct MeshData
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices32;

		std::vector<uint16> &GetIndices16()
		{
			m_indices16.resize(indices32.size());
			if(m_indices16.empty())
				for (size_t i = 0; i < indices32.size(); ++i)
					m_indices16[i] = static_cast<uint16>(indices32[i]);
			return m_indices16;
		}

	private:
		std::vector<uint16> m_indices16;
	};
};

