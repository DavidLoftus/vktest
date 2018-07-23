#include "stdafx.h"
#include "renderer.hpp"
#include "pipeline.hpp"
#include <stdlib.h>

VkResult vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto pfn_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
	return pfn_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
}

void vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto pfn_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
	pfn_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
}


vkRenderCtx_t vkRenderCtx;

void Renderer::init()
{
	initWindow();
	initCoreRenderer();
}

const Vertex quad[] = {
	glm::vec2(-1.0f, -1.0f),
	glm::vec2( 1.0f, -1.0f),
	glm::vec2(-1.0f,  1.0f),
	glm::vec2( 1.0f,  1.0f)
};

void Renderer::loadScene(const Scene& scene)
{
	initPipelines();
	initBuffers(scene.m_sprites);
	initCommandBuffers(scene.m_sprites.size());
	
}

void Renderer::loop()
{
	while (!glfwWindowShouldClose(m_window.get()))
	{
		updateBuffers();
		renderFrame();

		glfwPollEvents();

		m_currentFrame++;
	}

	m_device->waitIdle();
	
}

void Renderer::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_RESIZABLE, false);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);

	m_window.reset(window);
}

void Renderer::initCoreRenderer()
{

	initInstance();
	vkRenderCtx.instance = *m_instance;

	initSurface();
	initDebugReportCallback();

	choosePhysicalDevice();
	vkRenderCtx.physicalDevice = m_physicalDevice;
	vkRenderCtx.physicalDeviceProperties = m_physicalDevice.getProperties();

	initDevice();
	vkRenderCtx.device = *m_device;
	vkRenderCtx.queueFamily = m_queueFamily;
	vkRenderCtx.queue = m_queue;

	initAllocator();

	initSwapchain();
	vkRenderCtx.swapchainFormat = m_swapchainFormat;
	vkRenderCtx.swapchainExtent = m_swapchainExtent;

	initSyncObjects();

	initRenderPass();
	vkRenderCtx.renderPass = *m_renderPass;

	initFrameBuffers();

	initCommandPools();

}

const std::vector<const char*> layers{
	"VK_LAYER_LUNARG_standard_validation"
};

void Renderer::initInstance()
{

	_putenv("DISABLE_VK_LAYER_VALVE_steam_overlay_1=1"); // Steam Overlay causes swapchain to break, placing this outside the program might be more ideal.

	uint32_t count;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

	std::vector<const char*> extensions = {
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};
	std::copy(glfwExtensions, &glfwExtensions[count], std::back_inserter(extensions));

	std::cout << "Loading Instance Extensions: {";
	std::copy(extensions.begin(), extensions.end(), std::ostream_iterator<const char*>(std::cout, ", "));
	std::cout << '}' << std::endl;

	vk::ApplicationInfo appInfo{ "vktest", 0, "Pomme", 0, VK_API_VERSION_1_0 };
	m_instance = vk::createInstanceUnique(
		vk::InstanceCreateInfo{
			{}, &appInfo,
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data()
		}
	);

}

void Renderer::initSurface()
{
	vk::SurfaceKHR surface;
	auto result = static_cast<vk::Result>(glfwCreateWindowSurface(*m_instance, m_window.get(), nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)));

	vk::ObjectDestroy<vk::Instance> deleter(*m_instance);
	m_surface = vk::createResultValue(result, surface, "glfwCreateWindowSurface", deleter);
}

VkBool32 callback_fn(
	VkDebugReportFlagsEXT                       flags,
	VkDebugReportObjectTypeEXT                  objectType,
	uint64_t                                    object,
	size_t                                      location,
	int32_t                                     messageCode,
	const char*                                 pLayerPrefix,
	const char*                                 pMessage,
	void*                                       pUserData);

void Renderer::initDebugReportCallback()
{
	m_debugReportCallback = m_instance->createDebugReportCallbackEXTUnique(
		vk::DebugReportCallbackCreateInfoEXT{
			vk::DebugReportFlagBitsEXT::eDebug
			| vk::DebugReportFlagBitsEXT::eError
		| vk::DebugReportFlagBitsEXT::eInformation
		| vk::DebugReportFlagBitsEXT::ePerformanceWarning
		| vk::DebugReportFlagBitsEXT::eWarning,
		callback_fn,
		nullptr
		}
	);

}

void Renderer::choosePhysicalDevice()
{

	auto devices = m_instance->enumeratePhysicalDevices();

	/// TODO: Actual logic to choose the device.

	m_physicalDevice = devices.front();

}

void Renderer::initDevice()
{

	auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

	/// TODO: Multiple Queues when RENDERDOC is not defined.

	auto desiredFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
	auto it = std::find_if(
		queueFamilies.begin(),
		queueFamilies.end(),
		[desiredFlags,physicalDevice=m_physicalDevice, surface=*m_surface, idx = 0](vk::QueueFamilyProperties family) mutable
		{
			return ((family.queueFlags & desiredFlags) == desiredFlags) && physicalDevice.getSurfaceSupportKHR(idx++, surface);
		}
	);

	m_queueFamily = std::distance(queueFamilies.begin(), it);

	float priorities[] = { 1.0f }; // Ensure this is has as many numbers as queues.
	std::vector<vk::DeviceQueueCreateInfo> queues{ vk::DeviceQueueCreateInfo{ {}, m_queueFamily, 1, priorities } };

	std::vector<const char*> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	vk::PhysicalDeviceFeatures features;
	features.largePoints = true;

	m_device = m_physicalDevice.createDeviceUnique(vk::DeviceCreateInfo{ {},
		static_cast<uint32_t>(queues.size()),	  queues.data(),
		static_cast<uint32_t>(layers.size()),	  layers.data(),
		static_cast<uint32_t>(extensions.size()), extensions.data(),
		&features
		});

	m_queue = m_device->getQueue(m_queueFamily, 0);

}

void Renderer::initAllocator()
{
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.physicalDevice = m_physicalDevice;
	createInfo.device = *m_device;

	VmaAllocator allocator;
	vmaCreateAllocator(&createInfo, &allocator);

	m_allocator.reset(allocator);
}

void Renderer::initSyncObjects()
{
	vk::SemaphoreCreateInfo sCreateInfo;
	vk::FenceCreateInfo fCreateInfo{ vk::FenceCreateFlagBits::eSignaled };

	std::vector<vk::Semaphore> semaphores(m_swapchainImages.size());
	std::generate(semaphores.begin(), semaphores.end(), [device = *m_device, &sCreateInfo]() { return device.createSemaphore(sCreateInfo); });
	m_imageAvailableSemaphores = UniqueVector<vk::Semaphore>(std::move(semaphores), *m_device);

	semaphores.resize(m_swapchainImages.size());
	std::generate(semaphores.begin(), semaphores.end(), [device = *m_device, &sCreateInfo]() { return device.createSemaphore(sCreateInfo); });
	m_renderFinishedSemaphores = UniqueVector<vk::Semaphore>(std::move(semaphores), *m_device);

	std::vector<vk::Fence> bufferFences(m_swapchainImages.size());
	std::generate(bufferFences.begin(), bufferFences.end(), [device = *m_device, &fCreateInfo]() { return device.createFence(fCreateInfo); });
	m_bufferFences = UniqueVector<vk::Fence>(std::move(bufferFences), *m_device);

}

void Renderer::initSwapchain()
{

	auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);

	uint32_t result = m_physicalDevice.getSurfaceSupportKHR(0, *m_surface);
	std::cout << "Result: " << result << std::endl;

	auto formats = m_physicalDevice.getSurfaceFormatsKHR(*m_surface);

	m_swapchainFormat = formats[0].format;

	std::cout << "Printing formats" << std::endl;
	for (auto format : formats)
	{
		std::cout << vk::to_string(format.format) << ' ' << vk::to_string(format.colorSpace) << std::endl;
	}

	m_swapchainExtent = vk::Extent2D{
		std::clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
	};

	auto presentModes = m_physicalDevice.getSurfacePresentModesKHR(*m_surface);

	std::cout << "Printing present modes:" << std::endl;
	for (auto mode : presentModes)
	{
		std::cout << vk::to_string(mode) << std::endl;
	}

	m_swapchain = m_device->createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{
		{}, *m_surface,
		capabilities.minImageCount,
		m_swapchainFormat, vk::ColorSpaceKHR::eSrgbNonlinear,
		m_swapchainExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		0, nullptr,
		vk::SurfaceTransformFlagBitsKHR::eIdentity,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eFifo,
		true
	});

	m_swapchainImages = m_device->getSwapchainImagesKHR(*m_swapchain);
	
	m_swapchainImageViews = UniqueVector<vk::ImageView>{ {}, *m_device };

	m_swapchainImageViews->reserve(m_swapchainImages.size());
	std::transform(
		m_swapchainImages.begin(),
		m_swapchainImages.end(), 
		std::back_inserter(*m_swapchainImageViews),
		[device=*m_device, swapchainFormat=m_swapchainFormat](vk::Image image)
		{
			return device.createImageView(vk::ImageViewCreateInfo{
				{},
				image,
				vk::ImageViewType::e2D,
				swapchainFormat,
				{},
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			});
		}
	);

}

void Renderer::initRenderPass()
{

	std::vector<vk::AttachmentDescription> attachments{
		vk::AttachmentDescription{
			{},
			m_swapchainFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
	}
	};

	std::vector<vk::AttachmentReference> attachmentReferences{
		vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal }
	};



	std::vector<vk::SubpassDescription> subpasses{
		vk::SubpassDescription{
			{},
			vk::PipelineBindPoint::eGraphics,
			0, nullptr,
			static_cast<uint32_t>(attachmentReferences.size()), attachmentReferences.data()
	}
	};

	std::vector<vk::SubpassDependency> dependencies{

	};

	m_renderPass = m_device->createRenderPassUnique(
		vk::RenderPassCreateInfo{
			{},
			static_cast<uint32_t>(attachments.size()), attachments.data(),
			static_cast<uint32_t>(subpasses.size()), subpasses.data(),
			static_cast<uint32_t>(dependencies.size()), dependencies.data(),
		}
	);

}

void Renderer::initFrameBuffers()
{
	m_swapchainFramebuffers = UniqueVector<vk::Framebuffer>{ {}, *m_device };
	m_swapchainFramebuffers->reserve(m_swapchainImages.size());

	std::transform(
		m_swapchainImageViews->begin(),
		m_swapchainImageViews->end(),
		std::back_inserter(*m_swapchainFramebuffers),
		[device=*m_device,renderPass=*m_renderPass,&swapchainExtent=m_swapchainExtent](vk::ImageView imageView)
		{
		return device.createFramebuffer(vk::FramebufferCreateInfo{
				{},
				renderPass,
				1, &imageView,
				swapchainExtent.width, swapchainExtent.height,
				1
			});
		}
	);

}

void Renderer::initCommandPools()
{

	m_commandPool = m_device->createCommandPoolUnique(vk::CommandPoolCreateInfo{ {}, m_queueFamily });

}

void Renderer::initPipelines()
{
	constexpr auto bindingLayout = create_vertex_input_layout(hana::make_tuple(
		hana::make_pair(hana::type_c<Vertex>, hana::bool_c<false>),
		hana::make_pair(hana::type_c<Sprite>, hana::bool_c<true>)
	));

	auto pipeline = make_graphics_pipeline(bindingLayout, ENUM_CONSTANT(vk::PrimitiveTopology, eTriangleStrip), ENUM_CONSTANT(vk::PolygonMode, eFill));

	dump_layout(bindingLayout);

	pipeline.create({ "shaders/shader.vert.spv", "shaders/shader.frag.spv" });

	m_pipelines = UniqueVector<vk::Pipeline>{ std::vector<vk::Pipeline>(), *m_device };
	m_pipelines->push_back(pipeline.m_pipeline.release());
}

void Renderer::initBuffers(const std::vector<Sprite>& sceneSprites)
{

	m_vertexBuffer = createBufferUnique(4 * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
	m_instanceBuffer = createBufferUnique(4 * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Staging Buffer
	auto vertexStagingBuffer = createBufferUnique(4 * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
	{
		void* data;
		vk::Result result = vk::Result(vmaMapMemory(*m_allocator, vertexStagingBuffer->allocation, &data));
		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaMapMemory");

		memcpy(data, quad, sizeof(Vertex) * 4);
		vmaUnmapMemory(*m_allocator, vertexStagingBuffer->allocation);
	}

	auto cb = std::move(m_device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ *m_commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0]);

	cb->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
	cb->copyBuffer(vertexStagingBuffer->value, m_vertexBuffer->value, { vk::BufferCopy{ 0, 0, sizeof(Vertex) * 4 } });
	cb->end();

	m_queue.submit({ vk::SubmitInfo{ 0, nullptr, nullptr, 1, &cb.get() } }, nullptr);

	
	{ // Instance Buffer
		void* data;
		vk::Result result = vk::Result(vmaMapMemory(*m_allocator, m_instanceBuffer->allocation, &data));
		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaMapMemory");

		memcpy(data, sceneSprites.data(), sizeof(Sprite) * sceneSprites.size());
		vmaUnmapMemory(*m_allocator, m_instanceBuffer->allocation);
	}


	m_queue.waitIdle();
}

void Renderer::initCommandBuffers(size_t nInstances)
{

	m_graphicsCommandBuffers = m_device->allocateCommandBuffers(vk::CommandBufferAllocateInfo{ *m_commandPool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(m_swapchainFramebuffers->size()) });

	for (uint32_t i = 0; i < m_graphicsCommandBuffers.size(); ++i)
	{
		auto cb = m_graphicsCommandBuffers[i];

		cb.begin(vk::CommandBufferBeginInfo{});

		vk::ClearValue clearValue{ vk::ClearColorValue().setFloat32({0.0f, 0.0f, 0.0f, 0.0f}) };

		cb.beginRenderPass(
			vk::RenderPassBeginInfo{
				*m_renderPass,
				m_swapchainFramebuffers[i],
				vk::Rect2D({}, m_swapchainExtent),
				1, &clearValue
			},
			vk::SubpassContents::eInline
		);

		if (nInstances > 0)
		{
			cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipelines->front());

			cb.bindVertexBuffers(0, { m_vertexBuffer->value, m_instanceBuffer->value }, { 0, 0 });

			cb.draw(4, nInstances, 0, 0);
		}

		cb.endRenderPass();

		cb.end();

	}

}

void Renderer::updateBuffers()
{
}

void Renderer::renderFrame()
{
	uint32_t imageIdx = 0;

	auto result = m_device->acquireNextImageKHR(*m_swapchain, std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphores[imageIdx], nullptr);
	uint32_t idx = result.value;

	m_device->waitForFences({m_bufferFences[idx]}, true, std::numeric_limits<uint64_t>::max());
	m_device->resetFences({m_bufferFences[idx]});

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	m_queue.submit({
		vk::SubmitInfo{
			1, &m_imageAvailableSemaphores[imageIdx], &waitStage,
			1, &m_graphicsCommandBuffers[idx],
			1, &m_renderFinishedSemaphores[idx]
		}
		}, m_bufferFences[idx]);

	m_queue.presentKHR(
		vk::PresentInfoKHR{
			1, &m_renderFinishedSemaphores[idx],
			1, &m_swapchain.get(),
			&idx
		}
	);

	//vkRenderCtx.device.waitForFences({ renderFinishedFence.get() }, true, std::numeric_limits<uint64_t>::max());
	//vkRenderCtx.device.resetFences({ renderFinishedFence.get() });
}


VmaAlloc<vk::Buffer> Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	VmaAlloc<vk::Buffer> alloc;

	vk::BufferCreateInfo createInfo{
		{}, size, usage, vk::SharingMode::eExclusive // For now we only have 1 queue.
	};

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = memUsage;

	vmaCreateBuffer(*m_allocator, reinterpret_cast<VkBufferCreateInfo*>(&createInfo), &allocInfo, reinterpret_cast<VkBuffer*>(&alloc.value), &alloc.allocation, nullptr);

	return alloc;
}

vk::UniqueHandle<VmaAlloc<vk::Buffer>> Renderer::createBufferUnique(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	return vk::UniqueHandle<VmaAlloc<vk::Buffer>>( createBuffer(size, usage, memUsage), *m_allocator );
}