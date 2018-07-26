#include "stdafx.h"
#include "scene.hpp"


Scene Scene::Load(const std::string & path)
{
	Scene scene;

	std::ifstream inFile(path);
	
	if(!inFile) throw std::runtime_error("Scene file not found. Path = " + path);

	while (true)
	{
		Sprite sprite;
		inFile >> sprite.pos.x >> sprite.pos.y >> sprite.scale.x >> sprite.scale.y;
		inFile.ignore();

		if (!inFile) return scene;

		std::string texturePath;
		std::getline(inFile, texturePath);

		sprite.textureId = std::distance(scene.m_textures.begin(), std::find(scene.m_textures.begin(), scene.m_textures.end(), texturePath));
		if (sprite.textureId == scene.m_textures.size())
		{
			scene.m_textures.push_back(std::move(texturePath));
		}



		scene.m_sprites.push_back(sprite);
	}
}
