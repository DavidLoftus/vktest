#ifdef _MSC_VER
#	pragma once
#endif
#ifndef SCENE_HPP
#define SCENE_HPP
#include <vulkan/vulkan.hpp>
#include <boost/hana/adapt_struct.hpp>

struct Vertex
{
	glm::vec2 vpos;
	glm::vec2 tpos;
};

struct Sprite
{
	glm::vec2 pos;
	glm::vec2 scale;
};

BOOST_HANA_ADAPT_STRUCT(Sprite, pos, scale);
BOOST_HANA_ADAPT_STRUCT(Vertex, vpos, tpos);

class Scene
{
public:

	std::vector<Sprite> m_sprites;

	static Scene Load(const std::string& path);

private:

	Scene(std::vector<Sprite> sprites);

};

#endif