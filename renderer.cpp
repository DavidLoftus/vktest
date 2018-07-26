#include "stdafx.h"
#include "renderer.hpp"
#include <stdlib.h>
#include <stb_image.h>
#include <numeric>
#include "shader.hpp"

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

struct vertex_data
{
	glm::vec2 vpos;
	glm::vec2 tpos;
};

struct instance_data
{
	glm::vec2 pos;
	glm::vec2 scale;
};

const vertex_data quad[] = {
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

	initBuffers(sorted_sprites);
	initTextures(scene.textures());

	initDescriptorSets();
	initCommandBuffers(sorted_sprites);

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

	Shader::FreeShaders();
	
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

	m_pipelineDescriptorSetLayouts = UniqueVector<vk::DescriptorSetLayout>( std::move(descriptorSetLayout), *m_device );

	m_pipelineLayout = m_device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{{}, static_cast<uint32_t>(m_pipelineDescriptorSetLayouts->size()), m_pipelineDescriptorSetLayouts->data() });
}

void Renderer::initPipelines()
{
	/*constexpr auto bindingLayout = create_vertex_input_layout(hana::make_tuple(
		hana::make_pair(hana::type_c<vertex_data>, hana::bool_c<false>),
		hana::make_pair(hana::type_c<instance_data>, hana::bool_c<true>)
	));

	auto pipeline = make_graphics_pipeline(bindingLayout, ENUM_CONSTANT(vk::PrimitiveTopology, eTriangleStrip), ENUM_CONSTANT(vk::PolygonMode, eFill));

	dump_layout(bindingLayout);

	pipeline.create({ "shaders/shader.vert.spv", "shaders/shader.frag.spv" });
	*/

	m_pipeline.addShaderStage("shaders/shader.vert.spv");
	m_pipeline.addShaderStage("shaders/shader.frag.spv");

	vk::Viewport viewport{ 0.0f, 0.0f, (float)m_swapchainExtent.width, (float)m_swapchainExtent.height, 0.0f, 1.0f };
	vk::Rect2D scissor{ {}, m_swapchainExtent };


	std::vector<vk::VertexInputBindingDescription> bindings{
		vk::VertexInputBindingDescription{ 0, sizeof(vertex_data), vk::VertexInputRate::eVertex},
		vk::VertexInputBindingDescription{ 1, sizeof(instance_data), vk::VertexInputRate::eInstance }
	};
	std::vector<vk::VertexInputAttributeDescription> attributes{
		vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32Sfloat, 0 },
		vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32Sfloat, 8 },
		vk::VertexInputAttributeDescription{ 2, 1, vk::Format::eR32G32Sfloat, 0 },
		vk::VertexInputAttributeDescription{ 3, 1, vk::Format::eR32G32Sfloat, 8 }
	};

	m_pipeline.getVertexInputState()
		.setVertexBindingDescriptionCount(bindings.size())
		.setPVertexBindingDescriptions(bindings.data())
		.setVertexAttributeDescriptionCount(attributes.size())
		.setPVertexAttributeDescriptions(attributes.data());

	m_pipeline.getInputAssemblyState()
		.setTopology(vk::PrimitiveTopology::eTriangleStrip);

	m_pipeline.getViewportState()
		.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	m_pipeline.setLayout(*m_pipelineLayout);

	m_pipeline.setRenderPass(*m_renderPass, 0);

	m_pipeline.create();
}

void Renderer::initDescriptorSets()
{
	std::vector<vk::DescriptorPoolSize> poolSizes{
		vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(m_textureImages->size()) }
	};

	uint32_t maxSize = std::accumulate(
		poolSizes.begin(),
		poolSizes.end(),
		0u,
		[](uint32_t sum, const vk::DescriptorPoolSize& poolSize) { return sum + poolSize.descriptorCount; }
	);

	m_descriptorPool = m_device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{ {}, maxSize, static_cast<uint32_t>(poolSizes.size()), poolSizes.data() });

	std::vector<vk::DescriptorSetLayout> layouts( m_textureImages->size(), m_pipelineDescriptorSetLayouts[0] );
	
	m_textureSamplerDescriptorSets = m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		*m_descriptorPool,
		static_cast<uint32_t>(m_textureImages->size()),
		layouts.data()
	});
	
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(m_textureImages->size());

	for(auto imageView : *m_textureImageViews)
		imageInfos.emplace_back( *m_textureSampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal );

	std::vector<vk::WriteDescriptorSet> writeInfos;
	writeInfos.reserve(m_textureImages->size());
	for (size_t i = 0; i < m_textureImages->size(); ++i)
	{
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

	m_device->updateDescriptorSets( writeInfos, {} );

}

void Renderer::initBuffers(const std::vector<Sprite>& sceneSprites)
{

	m_vertexBuffer = createBufferUnique(4 * sizeof(vertex_data), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, VMA_MEMORY_USAGE_GPU_ONLY);
	m_instanceBuffer = createBufferUnique(sceneSprites.size() * sizeof(instance_data), vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

	// Staging Buffer
	auto vertexStagingBuffer = createBufferUnique(4 * sizeof(vertex_data), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
	{
		void* data;
		vk::Result result = vk::Result(vmaMapMemory(*m_allocator, vertexStagingBuffer->allocation, &data));
		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaMapMemory");

		memcpy(data, quad, sizeof(vertex_data) * 4);
		vmaUnmapMemory(*m_allocator, vertexStagingBuffer->allocation);
	}

	auto cb = std::move(m_device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ *m_commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0]);

	cb->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
	cb->copyBuffer(vertexStagingBuffer->value, m_vertexBuffer->value, { vk::BufferCopy{ 0, 0, sizeof(vertex_data) * 4 } });
	cb->end();

	m_queue.submit({ vk::SubmitInfo{ 0, nullptr, nullptr, 1, &cb.get() } }, nullptr);

	
	{ // Instance Buffer
		void* data;
		vk::Result result = vk::Result(vmaMapMemory(*m_allocator, m_instanceBuffer->allocation, &data));
		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaMapMemory");

		instance_data* instData = reinterpret_cast<instance_data*>(data);
		for (auto& sprite : sceneSprites)
			*instData++ = { sprite.pos, sprite.scale };
		
		vmaUnmapMemory(*m_allocator, m_instanceBuffer->allocation);
	}


	m_queue.waitIdle();
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

void Renderer::initCommandBuffers(const std::vector<Sprite>& sprites)
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
			m_pipeline.bind(cb);
			cb.bindVertexBuffers(0, { m_vertexBuffer->value, m_instanceBuffer->value }, { 0, 0 });
		}

		auto searchIt = sprites.begin();

		while (searchIt != sprites.end())
		{
			uint32_t id = searchIt->textureId;

			auto endIt = std::find_if_not(searchIt, sprites.end(), [id](const Sprite& sp) { return sp.textureId == id; });

			for (auto cb : m_graphicsCommandBuffers)
			{
				cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineLayout, 0, { m_textureSamplerDescriptorSets[id] }, {});
				cb.draw(4, static_cast<uint32_t>(std::distance(searchIt, endIt)), 0, static_cast<uint32_t>(std::distance(sprites.begin(), searchIt)));
			}

			searchIt = endIt;
		}
	}

	for (auto cb : m_graphicsCommandBuffers)
	{
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

UniqueVmaAlloc<vk::Buffer> Renderer::createBufferUnique(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	return UniqueVmaAlloc<vk::Buffer>( createBuffer(size, usage, memUsage), *m_allocator );
}

void transitionImageLayout(vk::CommandBuffer cb, vk::Image image, vk::Format imageFormat, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	vk::PipelineStageFlags srcStage, dstStage;
	vk::AccessFlags srcAccess, dstAccess;
	if (oldLayout == vk::ImageLayout::eUndefined)
	{
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
	}
	else if(oldLayout == vk::ImageLayout::eTransferDstOptimal)
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

		if(!pixels) throw std::runtime_error("Image file not found. Path = " + path);
		
		imageSize = width * height * sizeof(uint32_t);

		stagingBuffer = createBufferUnique(imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
		{
			void* data;
			vk::Result result{ vmaMapMemory(*m_allocator, stagingBuffer->allocation, &data) };
			if (result != vk::Result::eSuccess)
				vk::throwResultException(result, "vmaMapMemory");

			memcpy(data, pixels.get(), imageSize);

			vmaUnmapMemory(*m_allocator, stagingBuffer->allocation);
		}
	}

	VmaAlloc<vk::Image> image;
	{
		vk::ImageCreateInfo createInfo{
			{},
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Unorm,
			{width, height, 1},
			1, 1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive, 0, nullptr,
			vk::ImageLayout::eUndefined
		};

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vk::Result result{ vmaCreateImage(*m_allocator, reinterpret_cast<VkImageCreateInfo*>(&createInfo), &allocInfo, reinterpret_cast<VkImage*>(&image.value), &image.allocation, nullptr) };

		if (result != vk::Result::eSuccess)
			vk::throwResultException(result, "vmaCreateImage");
	}

	auto cb = std::move(m_device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ *m_commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0]);

	cb->begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

	transitionImageLayout(*cb, image.value, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	cb->copyBufferToImage(
		stagingBuffer->value,
		image.value,
		vk::ImageLayout::eTransferDstOptimal,
		{ 
			vk::BufferImageCopy{0, 0, 0, vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {}, {width, height, 1} }
		}
	);

	transitionImageLayout(*cb, image.value, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	cb->end();

	m_queue.submit({
		vk::SubmitInfo{0, nullptr, nullptr, 1, &cb.get() }
	}, nullptr);

	m_queue.waitIdle();

	return image;
}

UniqueVmaAlloc<vk::Image> Renderer::createImageUnique(const std::string & path)
{
	return UniqueVmaAlloc<vk::Image>(createImage(path), *m_allocator );
}
