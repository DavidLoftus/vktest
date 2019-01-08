#ifdef _MSC_VER
#	pragma once
#endif
#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "globals.h"
#include "pipeline.h"

struct GraphicsPipelineDefaults
{
	static const vk::PipelineVertexInputStateCreateInfo vertexInputState;
	static const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
	static const vk::PipelineTessellationStateCreateInfo tessellationState;
	static const vk::PipelineViewportStateCreateInfo viewportState;
	static const vk::PipelineRasterizationStateCreateInfo rasterizationState;
	static const vk::PipelineMultisampleStateCreateInfo multisampleState;
	static const vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	static const vk::PipelineColorBlendStateCreateInfo colorBlendState;
	static const vk::PipelineDynamicStateCreateInfo dynamicState;
};

struct RenderData
{
	glm::mat4 projection;
	glm::mat4 view;
};

const uint32_t WIDTH = 800, HEIGHT = 600;

#include "scene.h"
#include "mesh.h"
#include "shader.h"
#include "buffer.h"


using sometype = vertex_buffer<sprite_vertex>;

class Renderer
{
public:

	void init();
	void loadScene(const Scene& scene);
	void loop();


	void mouseMoved(float x, float y);

#pragma region Utils

	static VmaAlloc<vk::Buffer> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage);
	static UniqueVmaAlloc<vk::Buffer> createBufferUnique(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage);

	static void copyBuffer(VmaAlloc<vk::Buffer> src, VmaAlloc<vk::Buffer> dst);
	static void copyBuffer(VmaAlloc<vk::Buffer> src, VmaAlloc<vk::Buffer> dst, const std::vector<vk::BufferCopy>& ranges);

	template<typename Vertex>
	static VmaAlloc<vk::Buffer> createVertexBuffer(const std::vector<std::vector<Vertex>>& meshes, std::vector<std::pair<uint32_t, uint32_t>>& output);

	template<typename Vertex>
	static UniqueVmaAlloc<vk::Buffer> createVertexBufferUnique(const std::vector<std::vector<Vertex>>& meshes, std::vector<std::pair<uint32_t, uint32_t>>& output)
	{
		return UniqueVmaAlloc<vk::Buffer>{ createVertexBuffer(meshes, output), vkRenderCtx.allocator };
	}

	static VmaAlloc<vk::Image> createImage(const std::string& path);
	static UniqueVmaAlloc<vk::Image> createImageUnique(const std::string& path);


#pragma endregion



private:

	void initWindow();

	// Instance, Surface, DebugReportCallback, etc, all objects that will be alive for duration of app, without prior knowledge of scene.
#pragma region CoreRenderer
	void initCoreRenderer();

	void initInstance();
	void initSurface();
	void initDebugReportCallback();

	void choosePhysicalDevice();
	void initDevice();

	void initAllocator();
	void initSyncObjects();

	void initSwapchain();
	void initRenderPass();
	void initFrameBuffers();

	void initCommandPools();
#pragma endregion

#pragma region SceneLoad

	void initBuffers(const std::vector<Sprite>& sceneSprites, const std::vector<std::string>& objFiles, const std::vector<Object>& objects);
	void initTextures(const std::vector<std::string>& textures);
	void initPipelineLayout();
	void initPipelines();
	void initDescriptorSets(size_t nObjects);
	void initCommandBuffers(const std::vector<Sprite>& sprites, const std::vector<Object>& objects);

#pragma endregion

#pragma region RenderLoop

	void updateBuffers(float deltaT);
	void renderFrame();

#pragma endregion

private:

	std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> m_window{nullptr, glfwDestroyWindow};

	vk::UniqueInstance m_instance;
	vk::UniqueSurfaceKHR m_surface;
	vk::UniqueDebugReportCallbackEXT m_debugReportCallback;

	vk::PhysicalDevice m_physicalDevice;

	vk::UniqueDevice m_device;

	uint32_t m_queueFamily;
	vk::Queue m_queue;

	vk::UniqueSwapchainKHR m_swapchain;
	vk::Format m_swapchainFormat;
	vk::Extent2D m_swapchainExtent;

	std::vector<vk::Image> m_swapchainImages;
	UniqueVector<vk::ImageView> m_swapchainImageViews;

	vk::UniqueRenderPass m_renderPass;
	UniqueVector<vk::Framebuffer> m_swapchainFramebuffers;

	vk::UniqueCommandPool m_commandPool;

	UniqueVmaAllocator m_allocator;

	UniqueVector<vk::Semaphore> m_imageAvailableSemaphores;
	UniqueVector<vk::Semaphore> m_renderFinishedSemaphores;
	UniqueVector<vk::Fence> m_bufferFences;

	ShaderFree _sf;

	UniqueVector<vk::DescriptorSetLayout> m_texturePipelineDescriptorSetLayouts;
	vk::UniquePipelineLayout m_texturePipelineLayout;
	Pipeline m_texturePipeline{ GraphicsPipelineDefaults() };

	UniqueVector<vk::DescriptorSetLayout> m_worldPipelineDescriptorSetLayouts;
	vk::UniquePipelineLayout m_worldPipelineLayout;
	Pipeline m_worldPipeline{ GraphicsPipelineDefaults() };

	vertex_buffer<sprite_vertex> m_quadVertices;
	vertex_buffer<mesh_vertex> m_meshVertices;
	index_buffer<uint16_t> m_meshIndices;

	UniqueVmaAlloc<vk::Buffer> m_vertexDataBuffer;


	vertex_buffer<sprite_instance> m_spriteData{ 0 };
	uniform_buffer<std::pair<glm::mat4,glm::mat4>> m_renderData;
	uniform_buffer<glm::mat4> m_objectData;
	UniqueVmaAlloc<vk::Buffer> m_instanceDataBuffer;

	vk::UniqueDescriptorPool m_descriptorPool;

	std::vector<vk::CommandBuffer> m_graphicsCommandBuffers;

	UniqueVector<VmaAlloc<vk::Image>> m_textureImages;
	UniqueVector<vk::ImageView> m_textureImageViews;
	vk::UniqueSampler m_textureSampler;

	std::vector<std::pair<uint32_t,uint32_t>> m_meshLocations;

	std::vector<vk::DescriptorSet> m_meshDataDescriptorSets;
	vk::DescriptorSet m_renderDataDescriptorSet;

	std::vector<vk::DescriptorSet> m_textureSamplerDescriptorSets;

	RenderData m_cameraRenderData;

	glm::vec2 m_mousePos;
	glm::vec3 m_camPos{ 0.0f, 0.0f, 1.0f }, m_camDir{0.0f, 0.0f, -1.0f};

	size_t m_currentFrame = 0;

};

template<typename Vertex>
VmaAlloc<vk::Buffer> Renderer::createVertexBuffer(const std::vector<std::vector<Vertex>>& meshes, std::vector<std::pair<uint32_t, uint32_t>>& output)
{
	size_t totalVertices = std::accumulate(
		meshes.begin(),
		meshes.end(),
		0ull,
		[](size_t n, const std::vector<Vertex>& data) {return n + data.size(); }
	);

	if (totalVertices == 0)
	{
		return Renderer::createBuffer(
			1,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			VMA_MEMORY_USAGE_GPU_ONLY
		);
	}

	VmaAlloc<vk::Buffer> buffer = Renderer::createBuffer(
		totalVertices * sizeof(Vertex),
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		VMA_MEMORY_USAGE_GPU_ONLY
	);

	UniqueVmaAlloc<vk::Buffer> stagingBuffer = Renderer::createBufferUnique(
		totalVertices * sizeof(Vertex),
		vk::BufferUsageFlagBits::eTransferSrc,
		VMA_MEMORY_USAGE_CPU_ONLY
	);

	Vertex* data;
	vmaMapMemory(vkRenderCtx.allocator, stagingBuffer->allocation, reinterpret_cast<void**>(&data));

	uint32_t firstVertex = 0;
	for (auto& vertices : meshes)
	{
		uint32_t nVertices = static_cast<uint32_t>(vertices.size());
		memcpy(data + firstVertex, vertices.data(), nVertices * sizeof(Vertex));

		output.emplace_back(firstVertex, nVertices);

		firstVertex += nVertices;
	}

	vmaUnmapMemory(vkRenderCtx.allocator, stagingBuffer->allocation);

	Renderer::copyBuffer(*stagingBuffer, buffer);

	return buffer;
}

#endif
