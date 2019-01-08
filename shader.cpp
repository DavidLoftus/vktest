#include "stdafx.h"
#include "shader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "renderer.h"


std::unordered_map<std::string, Shader> shaders;

const Shader& Shader::FetchShader(const std::string& path)
{
	auto it = shaders.find(path);

	if (it == shaders.end()) // If not found add it and set it.
		it = shaders.emplace(path, Shader(path)).first;

	return it->second;
}

void Shader::FreeShaders()
{
	shaders.clear();
}

std::string Shader::getCode() const
{
	std::ifstream file(m_path, std::ios::binary);

	std::ostringstream os;
	os << file.rdbuf();

	return os.str();
}


vk::ShaderStageFlagBits Shader::getStage() const
{
	auto lastDot = std::find(m_path.rbegin(), m_path.rend(), '.');
	auto nextDot = std::find(std::next(lastDot), m_path.rend(), '.');

	std::string ext{ nextDot.base(), std::prev(lastDot.base()) };

	if (ext == "vert")
		return vk::ShaderStageFlagBits::eVertex;
	if (ext == "frag")
		return vk::ShaderStageFlagBits::eFragment;
	if (ext == "comp")
		return vk::ShaderStageFlagBits::eCompute;

	throw std::runtime_error("Unknown extension: " + ext);
}

const char* Shader::findEntryPoint() const
{
	// In future we might inspect spirv code.
	return "main";
}

Shader::Shader(std::string path) :
	m_path(std::move(path)),
	m_code(getCode()),
	m_handle(vkRenderCtx.device.createShaderModule(vk::ShaderModuleCreateInfo{ {}, m_code.size(), reinterpret_cast<uint32_t*>(m_code.data()) }))
{
	if (m_code.empty()) throw std::runtime_error("Shader code not found. Path = " + m_path);
}

Shader::Shader(Shader&& other) :
	m_path(std::move(other.m_path)),
	m_code(std::move(other.m_code)),
	m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

Shader::~Shader()
{
	vkRenderCtx.device.destroy(m_handle);
}

vk::PipelineShaderStageCreateInfo Shader::getStageInfo() const
{
	return vk::PipelineShaderStageCreateInfo{ {}, getStage(), m_handle, findEntryPoint() };
}
