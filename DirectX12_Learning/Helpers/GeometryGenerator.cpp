#include "../stdafx.h"

#include "GeometryGenerator.h"


GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint16 sliceCount, uint16 stackCount)
{
	MeshData meshData;
	float stackHeight = height / stackCount;
	float radiusStep = (topRadius - bottomRadius) / stackCount;
	float difRadius = bottomRadius - topRadius;
	float thetaStep = 2.0 * XM_PI / sliceCount;
	uint16 ringsNum = stackCount + 1;
	uint16 vertexPerRing = sliceCount + 1;	// Дублируем первую и последнюю вершины кольца, т.к. у них разные координаты текстуры.

	// Вычисляем вершины боковой поверхности цилиндра
	for (int i = 0; i < ringsNum; ++i)
	{
		float y = -0.5f * height + i * stackHeight;
		float radius = bottomRadius + i * radiusStep;

		for (int j = 0; j <= sliceCount; ++j)
		{
			Vertex vertex;
			float c = cosf(thetaStep * j);
			float s = sinf(thetaStep * j);
			vertex.position = XMFLOAT3(radius * c, y, radius * s);

			vertex.texCoord.x = (float)j / sliceCount;
			vertex.texCoord.y = 1.0f - (float)i / stackCount;

			// Цилиндр можно параметризовать 2-мя функциями по величине v [0;1] так, что
			/*
			Y(v) = h - hv				// позиция по Y
			R(v) = r1 + (r0 - r1)v		// R - радиус

			где: h- высота, r1 - верхний радиус, r2 - нижний радиус.
			
			x(t, v) = R(v) * cos(t)
			y(t, v) = h - hv
			z(t, v) = R(v) * sin(t)

			// направление тангент-вектора (касательной к окружности цилиндра)
			dx/dt = -R(v) * sin(t)		
			dy/dt = 0
			dz/dt = +R(v) * cos(t)

			// направление битангент-вектора (касательной к высоте цилиндра)
			dx/dv = (r0 - r1) * cos(t)
			dy/dv = -h
			dz/dv = (r0 - r1) * sin(t)
			*/
			
			vertex.tangentU = XMFLOAT3(-s, 0.0f, c);					// Единичный тангент-вектор
			XMFLOAT3 bitangent(difRadius * c, -height, difRadius * s);	// битангент-вектор
			XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.normal, N);
			meshData.vertices.push_back(vertex);
		}
	}
	// Вычисляем индексы боковой поверхности цилиндра
	for (uint32 i = 0; i < stackCount; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.indices32.push_back(i * vertexPerRing + j);
			meshData.indices32.push_back((i + 1) * vertexPerRing + j);
			meshData.indices32.push_back((i + 1) * vertexPerRing + j + 1);
			meshData.indices32.push_back(i * vertexPerRing + j);
			meshData.indices32.push_back((i + 1) * vertexPerRing + j + 1);
			meshData.indices32.push_back(i * vertexPerRing + j + 1);
		}
	}
	// Вычисляем вершины торцов цилиндра
	float y = 0.5f * height;

	// Top
	uint32 baseIndex = (uint32)meshData.vertices.size();
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i*thetaStep);
		float z = topRadius * sinf(i*thetaStep);
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		meshData.vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));		// Вершины торца top
	}
	meshData.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));		// Центр top
	uint32 centerIndex = (uint32)meshData.vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + i + 1);
		meshData.indices32.push_back(baseIndex + i);
	}

	// Bottom
	baseIndex = (uint32)meshData.vertices.size();
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i*thetaStep);
		float z = bottomRadius * sinf(i*thetaStep);
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		meshData.vertices.push_back(Vertex(x, -y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));		// Вершины торца bottom
	}
	meshData.vertices.push_back(Vertex(0.0f, -y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));		// Центр bottom
	centerIndex = (uint32)meshData.vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + i);
		meshData.indices32.push_back(baseIndex + i + 1);
	}
	return meshData;
}
