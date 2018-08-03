#ifdef _MSC_VER
#	pragma once
#endif
#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.hpp>
#include <string>

class Shader
{
	// Static Stuff
public:

	static const Shader& FetchShader(const std::string& path);
	static void FreeShaders();


private:

	Shader(std::string path);

	std::string m_path;
	std::string m_code;
	vk::ShaderModule m_handle;

	std::string getCode() const;
	vk::ShaderStageFlagBits getStage() const;
	const char* findEntryPoint() const;

public:

	~Shader();
	Shader(const Shader&) = delete;
	Shader(Shader&& other);

	vk::PipelineShaderStageCreateInfo getStageInfo() const;

};

struct ShaderFree
{
	~ShaderFree()
	{
		Shader::FreeShaders();
	}
};

#endif