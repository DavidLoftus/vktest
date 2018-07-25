#include "stdafx.h"
#include "pipeline.hpp"
#include "renderer.hpp"
#include "shader.hpp"

void Pipeline::create(bool keepCreateInfo)
{
	if (!m_createInfo) throw std::runtime_error("Pipeline info was never initialized before create() call.");

	m_createInfo->m_pipelineCreateInfo.stageCount = m_createInfo->m_shaderStages.size();
	m_createInfo->m_pipelineCreateInfo.pStages = m_createInfo->m_shaderStages.data();

	m_pipeline = vkRenderCtx.device.createGraphicsPipelineUnique(nullptr, m_createInfo->m_pipelineCreateInfo);

	if (!keepCreateInfo)
		m_createInfo.reset();
}

void Pipeline::addShaderStage(const std::string& shaderPath)
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();

	const Shader& shader = Shader::FetchShader(shaderPath);
	
	m_createInfo->m_shaderStages.push_back(shader.getStageInfo());
}

vk::PipelineVertexInputStateCreateInfo& Pipeline::getVertexInputState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_vertexInputState;
}

vk::PipelineInputAssemblyStateCreateInfo& Pipeline::getInputAssemblyState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_inputAssemblyState;
}

vk::PipelineTessellationStateCreateInfo& Pipeline::getTessellationState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_tessellationState;
}

vk::PipelineViewportStateCreateInfo& Pipeline::getViewportState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_viewportState;
}

vk::PipelineRasterizationStateCreateInfo& Pipeline::getRasterizationState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_rasterizationState;
}

vk::PipelineMultisampleStateCreateInfo& Pipeline::getMultisampleState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_multisampleState;
}

vk::PipelineDepthStencilStateCreateInfo& Pipeline::getDepthStencilState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_depthStencilState;
}

vk::PipelineColorBlendStateCreateInfo& Pipeline::getColorBlendState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_colorBlendState;
}

vk::PipelineDynamicStateCreateInfo& Pipeline::getDynamicState()
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	return m_createInfo->m_dynamicState;
}

Pipeline& Pipeline::setLayout(vk::PipelineLayout layout)
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	m_createInfo->m_pipelineCreateInfo.layout = layout;
	return *this;
}

Pipeline& Pipeline::setRenderPass(vk::RenderPass renderPass, uint32_t subpass)
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	m_createInfo->m_pipelineCreateInfo.renderPass = renderPass;
	m_createInfo->m_pipelineCreateInfo.subpass = subpass;
	return *this;
}

Pipeline& Pipeline::setBasePipeline(vk::Pipeline basePipelineHandle)
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	m_createInfo->m_pipelineCreateInfo.basePipelineHandle = basePipelineHandle;
	return *this;
}

Pipeline& Pipeline::setBasePipeline(int32_t basePipelineIndex)
{
	if (!m_createInfo) m_createInfo = std::make_unique<CompleteGraphicsPipelineCreateInfo>();
	m_createInfo->m_pipelineCreateInfo.subpass = basePipelineIndex;
	return *this;
}

void Pipeline::bind(vk::CommandBuffer cb)
{
	cb.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
}
