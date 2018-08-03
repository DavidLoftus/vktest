#include "stdafx.h"
#include <tiny_obj_loader.h>

#include "mesh.h"
#include "renderer.h"

MeshData MeshData::Load(std::string path)
{

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str());

	std::vector<mesh_vertex> vertices;

	auto& shape = shapes.front();

	vertices.reserve(shape.mesh.indices.size());
	for (auto& shape : shape.mesh.indices)
	{
		vertices.push_back(
			mesh_vertex{
				glm::vec3(
					attrib.vertices[shape.vertex_index * 3 + 0],
					attrib.vertices[shape.vertex_index * 3 + 1],
					attrib.vertices[shape.vertex_index * 3 + 2]
				),
				glm::vec3(
					attrib.colors[shape.vertex_index * 3 + 0],
					attrib.colors[shape.vertex_index * 3 + 1],
					attrib.colors[shape.vertex_index * 3 + 2]
				)
			}
		);
	}

	return {std::move(path), std::move(vertices)};
}
