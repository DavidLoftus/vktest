#include "shader.hpp"
#include <fstream>
#include <sstream>
#include "renderer.hpp"

std::string Shader::getCode()
{
	std::ifstream file(m_path, std::ios::binary);

	std::ostringstream os;
	os << file.rdbuf();

	return os.str();
}


vk::ShaderStageFlagBits Shader::getStage()
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

const char* Shader::findEntryPoint()
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

vk::PipelineShaderStageCreateInfo Shader::getStageInfo()
{
	return vk::PipelineShaderStageCreateInfo{ {}, getStage(), m_handle, findEntryPoint() };
}
