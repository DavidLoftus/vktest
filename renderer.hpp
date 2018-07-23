#ifdef _MSC_VER
#	pragma once
#endif
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

struct vkRenderCtx_t
{
	vk::Instance instance;

	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physicalDeviceProperties;

	vk::Device device;

	uint32_t queueFamily;
	vk::Queue queue;

	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;

	vk::RenderPass renderPass;
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

#include "scene.hpp"

class Renderer
{
public:

	void init();

	void loadScene(const Scene& scene);

	void loop();

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

	void initPipelines();
	void initBuffers(const std::vector<Sprite>& sceneSprites);
	void initCommandBuffers(size_t nInstances);

#pragma endregion

#pragma region RenderLoop

	void updateBuffers();
	void renderFrame();

#pragma endregion

#pragma region Utils

	VmaAlloc<vk::Buffer> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage);
	vk::UniqueHandle<VmaAlloc<vk::Buffer>> createBufferUnique(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage);

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

	UniqueVector<vk::Pipeline> m_pipelines;

	vk::UniqueHandle<VmaAlloc<vk::Buffer>> m_vertexBuffer;
	vk::UniqueHandle<VmaAlloc<vk::Buffer>> m_instanceBuffer;

	std::vector<vk::CommandBuffer> m_graphicsCommandBuffers;

	size_t m_currentFrame = 0;

};

#endif