#ifdef _MSC_VER
#	pragma once
#endif
#ifndef SHADER_HPP
#define SHADER_HPP

#include <vulkan/vulkan.hpp>
#include <string>

class Shader
{
private:
	std::string m_path;
	std::string m_code;
	vk::ShaderModule m_handle;

	std::string getCode();
	vk::ShaderStageFlagBits getStage();
	const char* findEntryPoint();

public:

	Shader(std::string path);
	Shader(const Shader&) = delete;
	Shader(Shader&& other);
	~Shader();

	vk::PipelineShaderStageCreateInfo getStageInfo();

};

#endif