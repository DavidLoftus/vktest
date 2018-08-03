#include "stdafx.h"
#include "scene.h"


Scene Scene::Load(const std::string & path)
{
	Scene scene;

	std::ifstream inFile(path);
	
	if(!inFile) throw std::runtime_error("Scene file not found. Path = " + path);

	unsigned int nSprites, nObjects;
	inFile >> nSprites >> nObjects; inFile.ignore();

	scene.m_sprites.reserve(nSprites);
	scene.m_objects.reserve(nObjects);
	
	for(unsigned int i = 0; i < nSprites; ++i)
	{
		Sprite sprite;
		inFile >> sprite.pos.x >> sprite.pos.y >> sprite.scale.x >> sprite.scale.y; inFile.ignore();

		std::string texturePath;
		std::getline(inFile, texturePath);

		sprite.textureId = std::distance(scene.m_textures.begin(), std::find(scene.m_textures.begin(), scene.m_textures.end(), texturePath)); // Works if not present because index of end() will be size()
		if (sprite.textureId == scene.m_textures.size())
		{
			scene.m_textures.push_back(std::move(texturePath));
		}

		scene.m_sprites.push_back(sprite);
	}

	for (unsigned int i = 0; i < nObjects; ++i)
	{
		Object object;
		inFile >> object.pos.x >> object.pos.y >> object.pos.z; inFile.ignore();

		std::string objectPath;
		std::getline(inFile, objectPath);
		object.meshId = std::distance(scene.m_objFiles.begin(), std::find(scene.m_objFiles.begin(), scene.m_objFiles.end(), objectPath));
		if (object.meshId == scene.m_objFiles.size())
		{
			scene.m_objFiles.push_back(std::move(objectPath));
		}

		scene.m_objects.push_back(object);
	}


	return scene;
}
