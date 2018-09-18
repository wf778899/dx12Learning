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
	uint16 vertexPerRing = sliceCount + 1;	// ��������� ������ � ��������� ������� ������, �.�. � ��� ������ ���������� ��������.

	// ��������� ������� ������� ����������� ��������
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

			// ������� ����� ��������������� 2-�� ��������� �� �������� v [0;1] ���, ���
			/*
			Y(v) = h - hv				// ������� �� Y
			R(v) = r1 + (r0 - r1)v		// R - ������

			���: h- ������, r1 - ������� ������, r2 - ������ ������.
			
			x(t, v) = R(v) * cos(t)
			y(t, v) = h - hv
			z(t, v) = R(v) * sin(t)

			// ����������� �������-������� (����������� � ���������� ��������)
			dx/dt = -R(v) * sin(t)		
			dy/dt = 0
			dz/dt = +R(v) * cos(t)

			// ����������� ���������-������� (����������� � ������ ��������)
			dx/dv = (r0 - r1) * cos(t)
			dy/dv = -h
			dz/dv = (r0 - r1) * sin(t)
			*/
			
			vertex.tangentU = XMFLOAT3(-s, 0.0f, c);					// ��������� �������-������
			XMFLOAT3 bitangent(difRadius * c, -height, difRadius * s);	// ���������-������
			XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.normal, N);
			meshData.vertices.push_back(vertex);
		}
	}
	// ��������� ������� ������� ����������� ��������
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
	// ��������� ������� ������ ��������
	float y = 0.5f * height;

	// Top
	uint32 baseIndex = (uint32)meshData.vertices.size();
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i*thetaStep);
		float z = topRadius * sinf(i*thetaStep);
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		meshData.vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));		// ������� ����� top
	}
	meshData.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));		// ����� top
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
		meshData.vertices.push_back(Vertex(x, -y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));		// ������� ����� bottom
	}
	meshData.vertices.push_back(Vertex(0.0f, -y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));		// ����� bottom
	centerIndex = (uint32)meshData.vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + i);
		meshData.indices32.push_back(baseIndex + i + 1);
	}
	return meshData;
}
