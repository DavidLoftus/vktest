#include "stdafx.h"
#include "renderer.h"
#include <stdlib.h>
#include <stb_image.h>
#include <numeric>
#include "shader.h"

namespace hana = boost::hana;

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


const vk::PipelineVertexInputStateCreateInfo GraphicsPipelineDefaults::vertexInputState;
const vk::PipelineInputAssemblyStateCreateInfo GraphicsPipelineDefaults::inputAssemblyState{ {}, vk::PrimitiveTopology::eTriangleList };
const vk::PipelineTessellationStateCreateInfo GraphicsPipelineDefaults::tessellationState;
const vk::PipelineViewportStateCreateInfo GraphicsPipelineDefaults::viewportState;
const vk::PipelineRasterizationStateCreateInfo GraphicsPipelineDefaults::rasterizationState{ {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f };
const vk::PipelineMultisampleStateCreateInfo GraphicsPipelineDefaults::multisampleState;
const vk::PipelineDepthStencilStateCreateInfo GraphicsPipelineDefaults::depthStencilState;

const vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState().setColorWriteMask(
	vk::ColorComponentFlagBits::eR |
	vk::ColorComponentFlagBits::eG |
	vk::ColorComponentFlagBits::eB |
	vk::ColorComponentFlagBits::eA
);

const vk::PipelineColorBlendStateCreateInfo GraphicsPipelineDefaults::colorBlendState{ {}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment };
const vk::PipelineDynamicStateCreateInfo GraphicsPipelineDefaults::dynamicState;


void Renderer::init()
{
	initWindow();
	initCoreRenderer();
}

const std::vector<sprite_vertex> quad = {
	{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) },
	{ glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 0.0f) },
	{ glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 1.0f) },
	{ glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f, 1.0f) }
};

void Renderer::loadScene(const Scene& scene)
{

	std::vector<Sprite> sorted_sprites = scene.sprites();
	std::sort(sorted_sprites.begin(), sorted_sprites.end(), [](const Sprite& a, const Sprite& b) { return a.textureId < b.textureId; });

	initPipelineLayout();
	initPipelines();

	initBuffers(sorted_sprites, scene.objFiles(), scene.objects());
	initTextures(scene.textures());

	initDescriptorSets(scene.objects().size());
	initCommandBuffers(sorted_sprites, scene.objects());

}

void Renderer::loop()
{
	std::chrono::high_resolution_clock clock;

	auto time = clock.now();

	while (!glfwWindowShouldClose(m_window.get()))
	{

		auto oldTime = std::exchange(time, clock.now());

		std::chrono::duration<float> tDiff = time - oldTime;

		updateBuffers(tDiff.count());
		renderFrame();

		glfwPollEvents();

		m_currentFrame++;
	}

	m_device->waitIdle();

	Shader::FreeShaders();
	
}


const float sensitivity = 0.02f;
const glm::vec3 VECTOR_UP{0.0f, 1.0f, 0.0f};

void Renderer::mouseMoved(float x, float y)
{

	glm::vec2 newPos(x, y);
	glm::vec2 mouseMove = newPos - m_mousePos;
	m_mousePos = newPos;

	//std::cout << glm::to_string(mouseMove) << std::endl;

	m_camDir = glm::rotate(m_camDir, -sensitivity * mouseMove.y, glm::cross(m_camDir, VECTOR_UP));
	m_camDir = glm::rotate(m_camDir, -sensitivity * mouseMove.x, VECTOR_UP);

}


VmaAlloc<vk::Buffer> Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	VmaAlloc<vk::Buffer> alloc;

	vk::BufferCreateInfo createInfo{
		{}, size, usage, vk::SharingMode::eExclusive // For now we only have 1 queue.
	};

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = memUsage;

	vmaCreateBuffer(vkRenderCtx.allocator, reinterpret_cast<VkBufferCreateInfo*>(&createInfo), &allocInfo, reinterpret_cast<VkBuffer*>(&alloc.value), &alloc.allocation, nullptr);

	return alloc;
}

UniqueVmaAlloc<vk::Buffer> Renderer::createBufferUnique(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	return UniqueVmaAlloc<vk::Buffer>(createBuffer(size, usage, memUsage), vkRenderCtx.allocator);
}

void Renderer::copyBuffer(VmaAlloc<vk::Buffer> src, VmaAlloc<vk::Buffer> dst)
{
	VmaAllocationInfo srcAllocInfo, dstAllocInfo;
	vmaGetAllocationInfo(vkRenderCtx.allocator, src.allocation, &srcAllocInfo);
	vmaGetAllocationInfo(vkRenderCtx.allocator, dst.allocation, &dstAllocInfo);

	size_t sz = std::min(srcAllocInfo.size, dstAllocInfo.size);

	auto cb = std::move(vkRenderCtx.device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ vkRenderCtx.commandPool, vk::CommandBufferLevel::ePrimary, 1 }).front());

	cb->begin(vk::CommandBufferBeginInfo{});

	cb->copyBuffer(src.value, dst.value, { vk::BufferCopy{ 0, 0, sz } });

	cb->end();

	auto fence = vkRenderCtx.device.createFenceUnique(vk::FenceCreateInfo{});

	vkRenderCtx.queue.submit(
		{
			vk::SubmitInfo{
			0, nullptr, nullptr,
			1, &cb.get()
		}
		},
		*fence
	);

	vkRenderCtx.device.waitForFences({ *fence }, true, std::numeric_limits<uint64_t>::max());
}

void transitionImageLayout(vk::CommandBuffer cb, vk::Image image, vk::Format imageFormat, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	vk::PipelineStageFlags srcStage, dstStage;
	vk::AccessFlags srcAccess, dstAccess;
	if (oldLayout == vk::ImageLayout::eUndefined)
	{
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		srcStage = vk::PipelineStageFlagBits::eTransfer;
		srcAccess = vk::AccessFlagBits::eTransferWrite;
	}

	if (newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		dstStage = vk::PipelineStageFlagBits::eTransfer;
		dstAccess = vk::AccessFlagBits::eTransferWrite;
	}
	else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
		dstAccess = vk::AccessFlagBits::eShaderRead;
	}


	cb.pipelineBarrier(
		srcStage,
		dstStage,
		{},
		{},
		{},
		{
			vk::ImageMemoryBarrier{
				srcAccess,
				dstAccess,
				oldLayout,
				newLayout,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image,
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			}
		}
	);
}

VmaAlloc<vk::Image> Renderer::createImage(const std::string& path)
{
	UniqueVmaAlloc<vk::Buffer> stagingBuffer;
	vk::DeviceSize imageSize;
	uint32_t width, height;

	{
		int channels;
		using unique_image = std::unique_ptr<stbi_uc, void(*)(void*)>;
		unique_image pixels = unique_image{ stbi_load(path.c_str(), reinterpret_cast<int*>(&width), reinterpret_cast<int*>(&height), &channels, STBI_rgb_alpha), stbi_image_free };

		if (!pixels) throw std::runtime_error("Image file not found. Path = " + path);

		imageSize = width * height * sizeof(uint32_t);

		stagingBuffer = createBufferUnique(imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
		{
			void* data;
			vk::Result result{ vmaMapMemory(vkRenderCtx.allocator, stagingBuffer->allocation, &data) };
			if (result != vk::Result::eSuccess)
				vk::throwResultException(result, "vmaMapMemory");

			memcpy(data, pixels.get(), imageSize);

			vmaUnmapMemory(vkRenderCtx.allocator, stagingBuffer->allocation);
		}
	}

	VmaAlloc<vk::Image> image;
	{
		vk::ImageCreateInfo createInfo{
			{},
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Unorm,
		{ width, height, 1 },
		1, 1,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive, 0, nullptr,
		vk::ImageLayout::eUndefined
		};

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vk::Result result{ vmaCreateImage(vkRenderCtx.allocator, reinterpret_cast<VkImageCreateInfo*>(&createInfo), &allocInfo, reinterpret_cast<VkImage*>(&image.value), &image.allocation, nullptr) };

		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaCreateImage");
	}

	auto cb = std::move(vkRenderCtx.device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ vkRenderCtx.commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0]);

	cb->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	transitionImageLayout(*cb, image.value, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	cb->copyBufferToImage(
		stagingBuffer->value,
		image.value,
		vk::ImageLayout::eTransferDstOptimal,
		{
			vk::BufferImageCopy{ 0, 0, 0, vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },{},{ width, height, 1 } }
		}
	);

	transitionImageLayout(*cb, image.value, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	cb->end();

	vkRenderCtx.queue.submit({
		vk::SubmitInfo{ 0, nullptr, nullptr, 1, &cb.get() }
		}, nullptr);

	vkRenderCtx.queue.waitIdle();

	return image;
}

UniqueVmaAlloc<vk::Image> Renderer::createImageUnique(const std::string & path)
{
	return UniqueVmaAlloc<vk::Image>(createImage(path), vkRenderCtx.allocator);
}

static void mouse_move_cb(GLFWwindow* window, double xpos, double ypos)
{
	reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window))->mouseMoved(xpos, ypos);
}

void Renderer::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_RESIZABLE, false);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);

	m_window.reset(window);

	glfwSetWindowUserPointer(m_window.get(), this);

	glfwSetInputMode(m_window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(m_window.get(), mouse_move_cb);

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
	vkRenderCtx.allocator = m_allocator.get();

	initSwapchain();
	vkRenderCtx.swapchainFormat = m_swapchainFormat;
	vkRenderCtx.swapchainExtent = m_swapchainExtent;

	initSyncObjects();

	initRenderPass();
	vkRenderCtx.renderPass = *m_renderPass;

	initFrameBuffers();

	initCommandPools();
	vkRenderCtx.commandPool = *m_commandPool;

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

	m_queueFamily = static_cast<uint32_t>(std::distance(queueFamilies.begin(), it));

	float priorities[] = { 1.0f }; // Ensure this is has as many numbers as queues.
	std::vector<vk::DeviceQueueCreateInfo> queues{ vk::DeviceQueueCreateInfo{ {}, m_queueFamily, 1, priorities } };

	std::vector<const char*> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	vk::PhysicalDeviceFeatures features;
	//features.largePoints = true;
	features.samplerAnisotropy = true;

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

void Renderer::initPipelineLayout()
{
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings{
			vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
		};

		std::vector<vk::DescriptorSetLayout> descriptorSetLayout{
			m_device->createDescriptorSetLayout(
				vk::DescriptorSetLayoutCreateInfo{
					{},
					static_cast<uint32_t>(bindings.size()),
					bindings.data()
				}
			)
		};

		m_texturePipelineDescriptorSetLayouts = UniqueVector<vk::DescriptorSetLayout>(std::move(descriptorSetLayout), *m_device);

		m_texturePipelineLayout = m_device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{ {}, static_cast<uint32_t>(m_texturePipelineDescriptorSetLayouts->size()), m_texturePipelineDescriptorSetLayouts->data() });
	}

	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings{
			vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
			vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex }
		};

		std::vector<vk::DescriptorSetLayout> descriptorSetLayout{
			m_device->createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{ {}, 1, &bindings[0] }),
			m_device->createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{ {}, 1, &bindings[1] }),
		};

		m_worldPipelineDescriptorSetLayouts = UniqueVector<vk::DescriptorSetLayout>(std::move(descriptorSetLayout), *m_device);

		m_worldPipelineLayout = m_device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{ {}, static_cast<uint32_t>(m_worldPipelineDescriptorSetLayouts->size()), m_worldPipelineDescriptorSetLayouts->data() });
	}
}

void Renderer::initPipelines()
{

	vk::Viewport viewport{ 0.0f, 0.0f, (float)m_swapchainExtent.width, (float)m_swapchainExtent.height, 0.0f, 1.0f };
	vk::Rect2D scissor{ {}, m_swapchainExtent };

	// Texture (2D) Pipeline
	{
		m_texturePipeline.addShaderStage("shaders/texture.vert.spv");
		m_texturePipeline.addShaderStage("shaders/texture.frag.spv");

		std::vector<vk::VertexInputBindingDescription> bindings{
			vk::VertexInputBindingDescription{ 0, sizeof(sprite_vertex), vk::VertexInputRate::eVertex},
			vk::VertexInputBindingDescription{ 1, sizeof(sprite_instance), vk::VertexInputRate::eInstance }
		};
		std::vector<vk::VertexInputAttributeDescription> attributes{
			vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32Sfloat, 0 },
			vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32Sfloat, 8 },
			vk::VertexInputAttributeDescription{ 2, 1, vk::Format::eR32G32Sfloat, 0 },
			vk::VertexInputAttributeDescription{ 3, 1, vk::Format::eR32G32Sfloat, 8 }
		};

		m_texturePipeline.getVertexInputState()
			.setVertexBindingDescriptionCount(static_cast<uint32_t>(bindings.size()))
			.setPVertexBindingDescriptions(bindings.data())
			.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributes.size()))
			.setPVertexAttributeDescriptions(attributes.data());
		m_texturePipeline.getInputAssemblyState()
			.setTopology(vk::PrimitiveTopology::eTriangleStrip);
		m_texturePipeline.getViewportState()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
			.setPScissors(&scissor);
		m_texturePipeline.setLayout(*m_texturePipelineLayout);
		m_texturePipeline.setRenderPass(*m_renderPass, 0);

		m_texturePipeline.create();
	}

	// World (3D) Pipeline
	{
		m_worldPipeline.addShaderStage("shaders/world.vert.spv");
		m_worldPipeline.addShaderStage("shaders/world.frag.spv");


		std::vector<vk::VertexInputBindingDescription> bindings{
			vk::VertexInputBindingDescription{ 0, sizeof(mesh_vertex), vk::VertexInputRate::eVertex }
		};
		std::vector<vk::VertexInputAttributeDescription> attributes{
			vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
			vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32B32Sfloat, 12 }
		};

		m_worldPipeline.getVertexInputState()
			.setVertexBindingDescriptionCount( static_cast<uint32_t>(bindings.size()) )
			.setPVertexBindingDescriptions(bindings.data())
			.setVertexAttributeDescriptionCount( static_cast<uint32_t>(attributes.size()) )
			.setPVertexAttributeDescriptions(attributes.data());
		m_worldPipeline.getInputAssemblyState()
			.setTopology(vk::PrimitiveTopology::eTriangleList);
		m_worldPipeline.getViewportState()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
			.setPScissors(&scissor);
		m_worldPipeline.setLayout(*m_worldPipelineLayout);
		m_worldPipeline.setRenderPass(*m_renderPass, 0);

		m_worldPipeline.create();
	}

}

struct ObjectRenderData
{
	glm::mat4 world;
};

struct RenderData
{
	glm::mat4 projection;
	glm::mat4 view;
};

size_t pad_up(size_t offset, size_t alignment)
{
	size_t r = offset & (alignment - 1);
	return (r == 0) ? offset : offset - r + alignment;
}


void Renderer::initDescriptorSets(size_t nObjects)
{

	// Pool
	{
		std::vector<vk::DescriptorPoolSize> poolSizes{
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(m_textureImages->size()) },
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(nObjects+1) }
		};

		uint32_t maxSize = std::accumulate(
			poolSizes.begin(),
			poolSizes.end(),
			0u,
			[](uint32_t sum, const vk::DescriptorPoolSize& poolSize) { return sum + poolSize.descriptorCount; }
		);

		m_descriptorPool = m_device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{ {}, maxSize, static_cast<uint32_t>(poolSizes.size()), poolSizes.data() });
	}



	// Image Descriptors
	{
		std::vector<vk::DescriptorSetLayout> layouts(m_textureImages->size(), m_texturePipelineDescriptorSetLayouts[0]);

		m_textureSamplerDescriptorSets = m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			*m_descriptorPool,
			static_cast<uint32_t>(m_textureImages->size()),
			layouts.data()
		});
	}

	// Object Descriptors
	{

		m_renderDataDescriptorSet = m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{ *m_descriptorPool, 1, &m_worldPipelineDescriptorSetLayouts[0] })[0];

		std::vector<vk::DescriptorSetLayout> layouts(nObjects, m_worldPipelineDescriptorSetLayouts[1]);

		m_meshDataDescriptorSets = m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			*m_descriptorPool,
			static_cast<uint32_t>(nObjects),
			layouts.data()
		});
	}

	std::vector<vk::WriteDescriptorSet> writeInfos;
	writeInfos.reserve(m_textureImages->size() + nObjects + 1);


	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(m_textureImages->size());

	for (size_t i = 0; i < m_textureImageViews->size(); ++i)
	{
		imageInfos.emplace_back(*m_textureSampler, m_textureImageViews[i], vk::ImageLayout::eShaderReadOnlyOptimal);

		writeInfos.push_back(
			vk::WriteDescriptorSet{
				m_textureSamplerDescriptorSets[i],
				0,
				0,
				1,
				vk::DescriptorType::eCombinedImageSampler,
			}.setPImageInfo(&imageInfos[i])
		);
	}

	size_t RenderData_size = pad_up(sizeof(RenderData),vkRenderCtx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
	size_t ObjectRenderData_size = pad_up(sizeof(ObjectRenderData), vkRenderCtx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

	std::vector<vk::DescriptorBufferInfo> bufferInfos{ vk::DescriptorBufferInfo{ m_meshDataBuffer->value, 0, sizeof(RenderData) } };

	bufferInfos.reserve(1 + nObjects);

	writeInfos.push_back(
		vk::WriteDescriptorSet{
			m_renderDataDescriptorSet,
			0,
			0,
			1,
			vk::DescriptorType::eUniformBuffer,
		}.setPBufferInfo(&bufferInfos.back())
	);

	for (size_t i = 0; i < nObjects; ++i)
	{
		bufferInfos.emplace_back(m_meshDataBuffer->value, RenderData_size + i * ObjectRenderData_size, sizeof(ObjectRenderData));
		writeInfos.push_back(
			vk::WriteDescriptorSet{
				m_meshDataDescriptorSets[i],
				0,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
			}.setPBufferInfo(&bufferInfos.back())
		);
	}

	m_device->updateDescriptorSets( writeInfos, {} );

}

void Renderer::initBuffers(const std::vector<Sprite>& sceneSprites, const std::vector<std::string>& objFiles, const std::vector<Object>& objects)
{
	std::vector<std::pair<uint32_t,uint32_t>> _unk;
	m_quadVertexBuffer = createVertexBufferUnique(std::vector<std::vector<sprite_vertex>>{ quad }, _unk);
	m_instanceBuffer = createBufferUnique(sceneSprites.size() * sizeof(mesh_vertex), vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
	
	{ // Instance Buffer
		void* data;
		vk::Result result = vk::Result(vmaMapMemory(*m_allocator, m_instanceBuffer->allocation, &data));
		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaMapMemory");

		sprite_instance* instData = reinterpret_cast<sprite_instance*>(data);
		for (auto& sprite : sceneSprites)
			*instData++ = { sprite.pos, sprite.scale };
		
		vmaUnmapMemory(*m_allocator, m_instanceBuffer->allocation);
	}

	std::vector<std::vector<mesh_vertex>> meshVertices;

	std::transform(objFiles.begin(), objFiles.end(), std::back_inserter(meshVertices), [](auto path) {return std::move(MeshData::Load(path).vertices()); });

	m_meshVertexBuffer = createVertexBufferUnique(meshVertices, m_meshLocations);

	size_t RenderData_size = pad_up(sizeof(RenderData), vkRenderCtx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
	size_t ObjectRenderData_size = pad_up(sizeof(ObjectRenderData), vkRenderCtx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

	m_meshDataBuffer = createBufferUnique(RenderData_size + objects.size() * sizeof(ObjectRenderData), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	{ // Mesh Data Buffer
		intptr_t data;
		vk::Result result = vk::Result(vmaMapMemory(*m_allocator, m_meshDataBuffer->allocation, reinterpret_cast<void**>(&data)));
		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaMapMemory");

		RenderData* instData = reinterpret_cast<RenderData*>(data);
		instData->projection = glm::infinitePerspective(glm::radians(100.0f), static_cast<float>(m_swapchainExtent.height)/m_swapchainExtent.width, 0.1f);
		instData->view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		instData->projection[1][1] *= -1;

		for (size_t i = 0; i < objects.size(); ++i)
			reinterpret_cast<ObjectRenderData*>(data + RenderData_size + ObjectRenderData_size * i)->world = glm::translate(glm::mat4(), objects[i].pos);

		vmaUnmapMemory(*m_allocator, m_meshDataBuffer->allocation);
	}


}

void Renderer::initTextures(const std::vector<std::string>& textures)
{
	{
		std::vector<VmaAlloc<vk::Image>> images;
		std::vector<vk::ImageView> imageViews;

		images.reserve(textures.size());
		imageViews.reserve(textures.size());

		for (auto& path : textures)
		{
			images.push_back(createImage(path));
			imageViews.push_back(m_device->createImageView(vk::ImageViewCreateInfo{
				{},
				images.back().value,
				vk::ImageViewType::e2D,
				vk::Format::eR8G8B8A8Unorm,
				vk::ComponentMapping{},
				vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
				}));
		}

		m_textureImages = UniqueVector<VmaAlloc<vk::Image>>( std::move(images), *m_allocator );
		m_textureImageViews = UniqueVector<vk::ImageView>(std::move(imageViews), *m_device);
	}
	

	m_textureSampler = m_device->createSamplerUnique(vk::SamplerCreateInfo{
		{},
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		0.0f,
		true, 16,
		false, vk::CompareOp::eAlways,
		0.0f, 0.0f,
		vk::BorderColor::eIntOpaqueBlack,
		false
	});

}

void Renderer::initCommandBuffers(const std::vector<Sprite>& sprites, const std::vector<Object>& objects)
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

	}
		
	if (!sprites.empty())
	{
		for (auto cb : m_graphicsCommandBuffers)
		{
			m_texturePipeline.bind(cb);
			cb.bindVertexBuffers(0, { m_quadVertexBuffer->value, m_instanceBuffer->value }, { 0, 0 });
		}

		auto searchIt = sprites.begin();

		while (searchIt != sprites.end())
		{
			uint32_t id = searchIt->textureId;

			auto endIt = std::find_if_not(searchIt, sprites.end(), [id](const Sprite& sp) { return sp.textureId == id; });

			for (auto cb : m_graphicsCommandBuffers)
			{
				cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_texturePipelineLayout, 0, { m_textureSamplerDescriptorSets[id] }, {});
				cb.draw(4, static_cast<uint32_t>(std::distance(searchIt, endIt)), 0, static_cast<uint32_t>(std::distance(sprites.begin(), searchIt)));
			}

			searchIt = endIt;
		}
	}

	if (!objects.empty())
	{
		for (auto cb : m_graphicsCommandBuffers)
		{
			m_worldPipeline.bind(cb);
			cb.bindVertexBuffers(0, { m_meshVertexBuffer->value }, { 0 });
			cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_worldPipelineLayout, 0, { m_renderDataDescriptorSet }, {});
		}

		for (size_t i = 0; i < objects.size(); ++i)
		{
			for (auto cb : m_graphicsCommandBuffers)
			{
				cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_worldPipelineLayout, 1, { m_meshDataDescriptorSets[i] }, {});
				cb.draw(m_meshLocations[objects[i].meshId].second, 1, m_meshLocations[objects[i].meshId].first, 0);
			}
		}

	}

	for (auto cb : m_graphicsCommandBuffers)
	{
		cb.endRenderPass();
		cb.end();
	}

}


const float speed = 50.0f;

void Renderer::updateBuffers(float deltaT)
{
	glm::vec3 moveDir{ 0.0f };
	if (glfwGetKey(m_window.get(), GLFW_KEY_W) == GLFW_PRESS)
	{
		moveDir += m_camDir;
	}
	if (glfwGetKey(m_window.get(), GLFW_KEY_S) == GLFW_PRESS)
	{
		moveDir -= m_camDir;
	}
	if (glfwGetKey(m_window.get(), GLFW_KEY_A) == GLFW_PRESS)
	{
		moveDir += glm::cross(VECTOR_UP, m_camDir);
	}
	if (glfwGetKey(m_window.get(), GLFW_KEY_D) == GLFW_PRESS)
	{
		moveDir -= glm::cross(VECTOR_UP, m_camDir);
	}

	if (moveDir != glm::zero<glm::vec3>())
	{
		m_camPos += glm::normalize(moveDir) * speed * deltaT;
	}

	if (glfwGetKey(m_window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE);
	}
	

	RenderData* data;
	vmaMapMemory(*m_allocator, m_meshDataBuffer->allocation, reinterpret_cast<void**>(&data));

	data->view = glm::lookAt(m_camPos, m_camPos+m_camDir, VECTOR_UP);

	vmaUnmapMemory(*m_allocator, m_meshDataBuffer->allocation);
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
}