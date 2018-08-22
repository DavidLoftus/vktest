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
	std::vector<uint16_t> indices;

	auto& shape = shapes.front();

	vertices.reserve(attrib.vertices.size() / 3);
	for (auto it = attrib.vertices.begin(); it != attrib.vertices.end(); std::advance(it, 3))
	{
		vertices.push_back(
			mesh_vertex{
				glm::vec3{
					*it,
					*std::next(it),
					*std::next(it,2)
				},
				glm::vec3{0.0f,1.0f,0.0f}
			}
		);
	}


	indices.reserve(shape.mesh.indices.size());
	for (auto& shape : shape.mesh.indices)
	{
		indices.push_back(shape.vertex_index);
	}

	return {std::move(path), std::move(vertices), std::move(indices)};
}
