#ifdef _MSC_VER
#	pragma once
#endif
#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "pipeline.h"

struct vkRenderCtx_t
{
	vk::Instance instance;

	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physicalDeviceProperties;

	vk::Device device;

	VmaAllocator allocator;

	uint32_t queueFamily;
	vk::Queue queue;

	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;

	vk::RenderPass renderPass;

	vk::CommandPool commandPool;
};

extern vkRenderCtx_t vkRenderCtx;

template<typename T>
struct VmaAlloc
{
	VmaAllocation allocation;
	T value;


	VmaAlloc& operator=(std::nullptr_t)
	{
		allocation = nullptr;
		value = nullptr;
		return *this;
	}

	operator bool() const
	{
		return value;
	}

	bool operator==(const VmaAlloc& other) const
	{
		return value == other.value;
	}

	bool operator!=(const VmaAlloc& other) const
	{
		return value != other.value;
	}

};

template<typename T>
using UniqueVmaAlloc = vk::UniqueHandle<VmaAlloc<T>>;

template<typename T>
class UniqueVector :
	public vk::UniqueHandleTraits<T>::deleter
{
	using Vector = std::vector<T>;
	using Deleter = typename vk::UniqueHandleTraits<T>::deleter;

	using size_type = typename Vector::size_type;

private:
	Vector m_value;

public:
	UniqueVector(Vector data = Vector(), const Deleter& deleter = Deleter()) :
		Deleter(deleter),
		m_value(std::move(data))
	{}

	UniqueVector(const UniqueVector&) = delete;

	UniqueVector(UniqueVector&& other) :
		Deleter(std::move(static_cast<Deleter&>(other))),
		m_value( other.release() )
	{}

	~UniqueVector()
	{
		std::for_each(m_value.begin(), m_value.end(), [this](auto x) { destroy(x); });
	}

	UniqueVector& operator=(const UniqueVector&) = delete;

	UniqueVector& operator=(UniqueVector&& other)
	{
		reset( other.release() );
		*static_cast<Deleter*>(this) = std::move(static_cast<Deleter&>(other));
		return *this;
	}

	T& operator[](size_type i)
	{
		return m_value[i];
	}

	const T& operator[](size_type i) const
	{
		return m_value[i];
	}

	explicit operator bool() const
	{
		return m_value.operator bool();
	}

	const Vector* operator->() const
	{
		return &m_value;
	}

	Vector* operator->()
	{
		return &m_value;
	}

	const Vector& operator*() const
	{
		return m_value;
	}

	Vector& operator*()
	{
		return m_value;
	}

	const Vector& get() const
	{
		return m_value;
	}

	Vector& get()
	{
		return m_value;
	}

	void reset(Vector value = Vector())
	{
		std::for_each(m_value.begin(), m_value.end(), [this](auto x) { destroy(x); });
		m_value = std::move(value);
	}

	Vector release()
	{
		return std::exchange(m_value, Vector());
	}

	void swap(UniqueVector& rhs)
	{
		std::swap(m_value, rhs.m_value);
		std::swap(static_cast<Deleter&>(*this), static_cast<Deleter&>(rhs));
	}

};

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

namespace std
{
	template<typename T>
	void swap(UniqueVector<T>& lhs, UniqueVector<T>& rhs)
	{
		lhs.swap(rhs);
	}
}

namespace vk
{
	template<typename T>
	class VmaDestroy {};

	template<>
	class VmaDestroy<NoParent>
	{
	public:
		VmaDestroy(Optional<const AllocationCallbacks> = nullptr) {}

	protected:
		void destroy(VmaAllocator allocator)
		{
			vmaDestroyAllocator(allocator);
		}
	};

	template<>
	class VmaDestroy<VmaAllocator>
	{
	public:
		VmaDestroy(VmaAllocator allocator = nullptr, Optional<const AllocationCallbacks> = nullptr) : m_allocator(allocator) {}

	protected:
		void destroy(const VmaAlloc<vk::Buffer>& alloc)
		{
			vmaDestroyBuffer(m_allocator, static_cast<VkBuffer>(alloc.value), alloc.allocation);
		}
		void destroy(const VmaAlloc<vk::Image>& alloc)
		{
			vmaDestroyImage(m_allocator, static_cast<VkImage>(alloc.value), alloc.allocation);
		}

	private:
		VmaAllocator m_allocator;
	};


	template<>
	struct UniqueHandleTraits<VmaAllocator>
	{
		using deleter = VmaDestroy<NoParent>;
	};

	template<typename T>
	struct UniqueHandleTraits<VmaAlloc<T>>
	{
		using deleter = VmaDestroy<VmaAllocator>;
	};
}

const uint32_t WIDTH = 800, HEIGHT = 600;

#include "scene.h"
#include "mesh.h"
#include "shader.h"

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

	vk::UniqueHandle<VmaAllocator> m_allocator;

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

	UniqueVmaAlloc<vk::Buffer> m_quadVertexBuffer;
	UniqueVmaAlloc<vk::Buffer> m_instanceBuffer;

	vk::UniqueDescriptorPool m_descriptorPool;

	std::vector<vk::CommandBuffer> m_graphicsCommandBuffers;

	UniqueVector<VmaAlloc<vk::Image>> m_textureImages;
	UniqueVector<vk::ImageView> m_textureImageViews;
	vk::UniqueSampler m_textureSampler;

	UniqueVmaAlloc<vk::Buffer> m_meshVertexBuffer;
	std::vector<std::pair<uint32_t,uint32_t>> m_meshLocations;

	UniqueVmaAlloc<vk::Buffer> m_meshDataBuffer;
	std::vector<vk::DescriptorSet> m_meshDataDescriptorSets;
	vk::DescriptorSet m_renderDataDescriptorSet;

	std::vector<vk::DescriptorSet> m_textureSamplerDescriptorSets;

	glm::vec2 m_mousePos;

	glm::vec3 m_camPos{ 0.0f, 0.0f, 1.0f }, m_camDir{0.0f, 0.0f, -1.0f};

	size_t m_currentFrame = 0;

};

template<typename Vertex>
static VmaAlloc<vk::Buffer> Renderer::createVertexBuffer(const std::vector<std::vector<Vertex>>& meshes, std::vector<std::pair<uint32_t, uint32_t>>& output)
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