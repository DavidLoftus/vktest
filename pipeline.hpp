#ifdef _MSC_VER
#	pragma once
#endif
#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <vulkan/vulkan.hpp>



struct CompleteGraphicsPipelineCreateInfo
{
	CompleteGraphicsPipelineCreateInfo() = default;

	template<class Default>
	constexpr CompleteGraphicsPipelineCreateInfo(Default) :
		m_vertexInputState(Default::vertexInputState),
		m_inputAssemblyState(Default::inputAssemblyState),
		m_tessellationState(Default::tessellationState),
		m_viewportState(Default::viewportState),
		m_rasterizationState(Default::rasterizationState),
		m_multisampleState(Default::multisampleState),
		m_depthStencilState(Default::depthStencilState),
		m_colorBlendState(Default::colorBlendState),
		m_dynamicState(Default::dynamicState)
	{
	}

	std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
	vk::PipelineVertexInputStateCreateInfo m_vertexInputState;
	vk::PipelineInputAssemblyStateCreateInfo m_inputAssemblyState;
	vk::PipelineTessellationStateCreateInfo m_tessellationState;
	vk::PipelineViewportStateCreateInfo m_viewportState;
	vk::PipelineRasterizationStateCreateInfo m_rasterizationState;
	vk::PipelineMultisampleStateCreateInfo m_multisampleState;
	vk::PipelineDepthStencilStateCreateInfo m_depthStencilState;
	vk::PipelineColorBlendStateCreateInfo m_colorBlendState;
	vk::PipelineDynamicStateCreateInfo m_dynamicState;

	vk::GraphicsPipelineCreateInfo m_pipelineCreateInfo{ {},
		0, nullptr, // Filled out later
		&m_vertexInputState,
		&m_inputAssemblyState,
		nullptr, //&m_tessellationState,
		&m_viewportState,
		&m_rasterizationState,
		&m_multisampleState,
		&m_depthStencilState,
		&m_colorBlendState,
		nullptr, //&m_dynamicState,
		nullptr, nullptr, 0
	};
};

class Pipeline
{
public:

#pragma region Creation Stuff

	Pipeline() = default;
	Pipeline(Pipeline&& other) = default;
	Pipeline(const Pipeline& other) = delete;

	template<typename T>
	Pipeline(T&& DefaultSettings) : m_createInfo(std::make_unique<CompleteGraphicsPipelineCreateInfo>(std::forward<T>(DefaultSettings)))
	{
	}

	void create(bool keepCreateInfo = false);

	void addShaderStage(const std::string& shaderPath);
	vk::PipelineVertexInputStateCreateInfo& getVertexInputState();
	vk::PipelineInputAssemblyStateCreateInfo& getInputAssemblyState();
	vk::PipelineTessellationStateCreateInfo& getTessellationState();
	vk::PipelineViewportStateCreateInfo& getViewportState();
	vk::PipelineRasterizationStateCreateInfo& getRasterizationState();
	vk::PipelineMultisampleStateCreateInfo& getMultisampleState();
	vk::PipelineDepthStencilStateCreateInfo& getDepthStencilState();
	vk::PipelineColorBlendStateCreateInfo& getColorBlendState();
	vk::PipelineDynamicStateCreateInfo& getDynamicState();
	Pipeline& setLayout(vk::PipelineLayout layout);
	Pipeline& setRenderPass(vk::RenderPass renderPass, uint32_t subpass);
	Pipeline& setBasePipeline(vk::Pipeline basePipelineHandle);
	Pipeline& setBasePipeline(int32_t basePipelineIndex);

#pragma endregion

#pragma region Runtime Stuff

	void bind(vk::CommandBuffer cb);

#pragma endregion

private:
	vk::UniquePipeline m_pipeline;
	std::unique_ptr<CompleteGraphicsPipelineCreateInfo> m_createInfo;
};

#endif