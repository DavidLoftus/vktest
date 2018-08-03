#ifdef _MSC_VER
#	pragma once
#endif
#ifndef SCENE_H
#define SCENE_H
#include <vulkan/vulkan.hpp>
#include <boost/hana/adapt_struct.hpp>

#include "mesh.h"

struct sprite_vertex
{
	glm::vec2 vpos;
	glm::vec2 tpos;
};

struct sprite_instance
{
	glm::vec2 pos;
	glm::vec2 scale;
};

struct Sprite
{
	glm::vec2 pos;
	glm::vec2 scale;
	uint32_t textureId;
};

struct Object
{
	glm::vec3 pos;
	uint32_t meshId;
};

//BOOST_HANA_ADAPT_STRUCT(Sprite, pos, scale);
//BOOST_HANA_ADAPT_STRUCT(Vertex, vpos, tpos);


class Scene
{
public:
	static Scene Load(const std::string& path);

public:

	const std::vector<std::string>& textures() const
	{
		return m_textures;
	}
	const std::vector<Sprite>& sprites() const
	{
		return m_sprites;
	}

	const std::vector<std::string>& objFiles() const
	{
		return m_objFiles;
	}
	const std::vector<Object>& objects() const
	{
		return m_objects;
	}

private:

	std::vector<std::string> m_textures;
	std::vector<Sprite> m_sprites;

	std::vector<std::string> m_objFiles;
	std::vector<Object> m_objects;

};
#endif