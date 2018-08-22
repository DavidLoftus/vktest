#ifdef _MSC_VER
#	pragma once
#endif
#ifndef MESH_H
#define MESH_H

#include <vector>

struct mesh_vertex
{
	glm::vec3 pos;
	glm::vec3 color;
};

struct MeshData
{
public:
	static MeshData Load(std::string path);

	const std::string& path() const
	{
		return m_path;
	}
	const std::vector<mesh_vertex>& vertices() const
	{
		return m_vertices;
	}
	std::vector<mesh_vertex>& vertices()
	{
		return m_vertices;
	}
	const std::vector<uint16_t>& indices() const
	{
		return m_indices;
	}
	std::vector<uint16_t>& indices()
	{
		return m_indices;
	}

private:

	MeshData(std::string path, std::vector<mesh_vertex> vertices, std::vector<uint16_t> indices) :
		m_path(std::move(path)),
		m_vertices(std::move(vertices)),
		m_indices(std::move(indices))
	{}

	std::string m_path;
	std::vector<mesh_vertex> m_vertices;
	std::vector<uint16_t> m_indices;
};

#endif