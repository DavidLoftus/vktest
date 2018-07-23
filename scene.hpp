#ifdef _MSC_VER
#	pragma once
#endif
#ifndef SCENE_HPP
#define SCENE_HPP
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <boost/hana/adapt_struct.hpp>

struct Vertex
{
	glm::vec2 vpos;
};

struct Sprite
{
	glm::vec2 pos;
	glm::vec2 scale;
	glm::vec3 color;
};

BOOST_HANA_ADAPT_STRUCT(Sprite, pos, scale, color);
BOOST_HANA_ADAPT_STRUCT(Vertex, vpos);

class Scene
{
public:

	std::vector<Sprite> m_sprites;

	static Scene Load(const std::string& path);

private:

	Scene(std::vector<Sprite> sprites);

};

#endif