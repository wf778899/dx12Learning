#pragma once

using namespace DirectX;

class GeometryGenerator
{
public:

	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	struct Vertex
	{
		Vertex() {}
		Vertex(const XMFLOAT3 &pos, const XMFLOAT3 &nor, const XMFLOAT3 &tan, const XMFLOAT2 &tex)
			: position(pos)
			, normal(nor)
			, tangentU(tan)
			, texCoord(tex) {}
		Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v)
			: position(px, py, pz)
			, normal(nx, ny, nz)
			, tangentU(tx, ty, tz)
			, texCoord(u, v) {}

		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangentU;
		XMFLOAT2 texCoord;
	};

	struct MeshData
	{
		std::vector<Vertex> vertices;
		std::vector<uint32> indices32;

		std::vector<uint16>& GetIndices16()
		{
			if (m_indices16.empty())
				m_indices16.resize(indices32.size());
				for (size_t i = 0; i < indices32.size(); ++i)
					m_indices16[i] = static_cast<uint16>(indices32[i]);
			return m_indices16;
		}
	private:
		std::vector<uint16> m_indices16;
	};

	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint16 sliceCount, uint16 stackCount);
};

