#include "scene.hpp"


Scene Scene::Load(const std::string & path)
{
	std::vector<Sprite> sprites;

	std::ifstream inFile(path);

	while (inFile)
	{
		Sprite sprite;
		inFile >> sprite.pos.x >> sprite.pos.y >> sprite.scale.x >> sprite.scale.y >> sprite.color.r >> sprite.color.g >> sprite.color.b;
		inFile.ignore();

		sprites.push_back(sprite);
	}

	return Scene{ std::move(sprites) };
}

Scene::Scene(std::vector<Sprite> sprites) : 
	m_sprites(std::move(sprites)) 
{
}
