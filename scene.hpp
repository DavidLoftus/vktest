#ifdef _MSC_VER
#	pragma once
#endif
#ifndef SCENE_HPP
#define SCENE_HPP
#include <vulkan/vulkan.hpp>
#include <boost/hana/adapt_struct.hpp>

struct Sprite
{
	glm::vec2 pos;
	glm::vec2 scale;
	uint32_t textureId;
};

//BOOST_HANA_ADAPT_STRUCT(Sprite, pos, scale);
//BOOST_HANA_ADAPT_STRUCT(Vertex, vpos, tpos);


class Scene
{
public:
	static Scene Load(const std::string& path);

public:

	const std::vector<Sprite>& sprites() const
	{
		return m_sprites;
	}
	const std::vector<std::string>& textures() const
	{
		return m_textures;
	}

private:

	std::vector<std::string> m_textures;
	std::vector<Sprite> m_sprites;

};
#endif