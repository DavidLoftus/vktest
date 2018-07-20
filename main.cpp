#define VMA_IMPLEMENTATION
#ifdef WIN32
#include <windows.h>
#endif
#include <math.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <boost/hana.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "termcolor.hpp"
#include "vk_mem_alloc.h"
#include "pipeline.hpp"


#define RENDERDOC

#define NPARTICLES 100


VkResult vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto pfn_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugReportCallbackEXT"));
	return pfn_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
}

void vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto pfn_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugReportCallbackEXT"));
	pfn_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
}

std::ostream& operator<<(std::ostream& os, vk::DebugReportFlagsEXT flags)
{
	bool multiple = false;

	if(flags & vk::DebugReportFlagBitsEXT::eDebug)
	{
		if(multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::dark << termcolor::bold << "DEBUG";
	}
	if(flags & vk::DebugReportFlagBitsEXT::eError)
	{
		if(multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::red << termcolor::bold << "ERROR";
	}
	if(flags & vk::DebugReportFlagBitsEXT::eInformation)
	{
		if(multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::dark << "INFO";
	}
	if(flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
	{
		if(multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::yellow << "PERFWARN";
	}
	if(flags & vk::DebugReportFlagBitsEXT::eWarning)
	{
		if(multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::red << "WARNING";
	}

	if(!multiple)
		os << termcolor::blue << "UNKNOWN";

	os << termcolor::reset;
	return os;
}

VkBool32 callback_fn(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData)
{
	std::cout << '[' << vk::DebugReportFlagsEXT(flags) << "] " << pMessage << std::endl;
	
	return false;
}

vk::UniqueShaderModule createShaderModule(vk::Device device, const std::string& path)
{
	std::vector<char> code;

	std::ifstream file(path, std::ios::in | std::ios::binary);

	if(!file) throw std::runtime_error("File " + path + " not found.");

	std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), std::back_inserter(code));

	if(code.empty()) throw std::runtime_error("File " + path + " empty");

	return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{{}, static_cast<uint32_t>(code.size()), reinterpret_cast<uint32_t*>(code.data())});

}

const uint32_t WIDTH = 800, HEIGHT = 600;

auto createWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_RESIZABLE,false);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);

	return std::unique_ptr<GLFWwindow, void(*)(GLFWwindow* ptr)>(window,glfwDestroyWindow);
}

std::vector<const char*> layers{
	"VK_LAYER_LUNARG_standard_validation"
};
std::vector<const char*> extensions{ 
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

auto createInstance()
{	

	for (auto& layer : vk::enumerateInstanceLayerProperties())
	{
		std::cout << layer.layerName << ": " << layer.description << std::endl;
	}

	uint32_t count;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

	std::vector<const char*> extensions_copy = extensions;
	extensions_copy.insert(extensions_copy.end(), glfwExtensions, &glfwExtensions[count]);

	for(auto extension : extensions_copy) std::cout << "InstanceExtension " << extension << std::endl;

	vk::ApplicationInfo appInfo{"vktest", 0, "Pomme", 0, VK_API_VERSION_1_0};
	return vk::createInstanceUnique(
		vk::InstanceCreateInfo{
			{}, &appInfo,
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions_copy.size()), extensions_copy.data()
		}
	);
}

auto createSurface(vk::Instance instance, GLFWwindow* window)
{
	vk::SurfaceKHR surface;
	auto result = static_cast<vk::Result>(glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)));

	vk::ObjectDestroy<vk::Instance> deleter(instance);
	return vk::createResultValue(result, surface, "createSurface", deleter);
}

auto createDebugThinger(vk::Instance instance)
{
	return instance.createDebugReportCallbackEXTUnique(
		vk::DebugReportCallbackCreateInfoEXT{
			vk::DebugReportFlagBitsEXT::eDebug
		|	vk::DebugReportFlagBitsEXT::eError
		|	vk::DebugReportFlagBitsEXT::eInformation
		|	vk::DebugReportFlagBitsEXT::ePerformanceWarning
		|	vk::DebugReportFlagBitsEXT::eWarning,
			callback_fn,
			nullptr
		}
	);
}

template<typename Func>
auto findQueue(std::vector<vk::QueueFamilyProperties>& queuePropertiesList, Func func)
{
#ifdef RENDERDOC
	return std::make_pair(0, 0);
#else
	uint32_t i = static_cast<uint32_t>(queuePropertiesList.size());
	auto it = find_if(
		queuePropertiesList.rbegin(),
		queuePropertiesList.rend(),
		[&func, &i](const vk::QueueFamilyProperties properties)
	{
		--i;
		return properties.queueCount > 0 && func(i, properties);
	}
	);

	if (it == queuePropertiesList.rend())
	{
		std::cerr << queuePropertiesList.size() << std::endl;
		throw runtime_error("No appropriate queue.");
	}

	return std::make_pair(i, it->queueCount--);
#endif
}

template<typename ... Types>
auto findQueues(std::vector<vk::QueueFamilyProperties>& queuePropertiesList, Types ... args)
{
	return make_tuple(
		findQueue(
			queuePropertiesList,
			[args](uint32_t i, const vk::QueueFamilyProperties& properties)
	{
		return (properties.queueFlags & args) == args;
	}
		)...
	);
}

std::pair<uint32_t,uint32_t> pipelineQueueFamily, transferQueueFamily, presentQueueFamily; // <family,queue>


auto createDevice(vk::Instance instance, vk::SurfaceKHR surface)
{
	auto physicalDevices = instance.enumeratePhysicalDevices();

	for (auto& physicalDevice : physicalDevices)
	{
		auto properties = physicalDevice.getProperties();
		std::cout << "PhysicalDevice " << properties.deviceName << " " << vk::to_string(properties.deviceType) << std::endl;
	}

	auto physicalDevice = physicalDevices.front();
	for(auto extension : physicalDevice.enumerateDeviceExtensionProperties())
	{
		std::cout << "DeviceExtension: " << extension.extensionName << '(' << extension.specVersion << ')' << std::endl;
	}

	auto queuePropertiesList = physicalDevice.getQueueFamilyProperties();

	for (auto& prop : queuePropertiesList)
		std::cout << vk::to_string(prop.queueFlags) << ": " << prop.queueCount << std::endl;

	tie(pipelineQueueFamily,
		transferQueueFamily) = findQueues(queuePropertiesList,
		vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eTransfer
	);

	presentQueueFamily = findQueue(queuePropertiesList, [physicalDevice, surface](uint32_t i, const vk::QueueFamilyProperties&) {return physicalDevice.getSurfaceSupportKHR(i, surface); });

	std::unordered_map<uint32_t, uint32_t> queueCounts;
	queueCounts[pipelineQueueFamily.first]++;
	queueCounts[transferQueueFamily.first]++;
	queueCounts[presentQueueFamily.first]++;

	float priorities[] = { 1.0f, 1.0f, 1.0f }; // Ensure this is has as many numbers as queues.
#ifdef RENDERDOC
	std::vector<vk::DeviceQueueCreateInfo> queues{ vk::DeviceQueueCreateInfo{ {}, 0, 1, priorities } };
#else
	std::vector<vk::DeviceQueueCreateInfo> queues;
	std::transform(
		queueCounts.begin(),
		queueCounts.end(),
		std::back_inserter(queues),
		[&priorities](auto p)
		{
			return vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags{}, p.first, p.second, priorities};
		}
	);
#endif

	vk::PhysicalDeviceFeatures features;
	features.largePoints = true;

	extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	
	auto device = physicalDevice.createDeviceUnique(
		vk::DeviceCreateInfo{{},
			static_cast<uint32_t>(queues.size()),	  queues.data(),
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data(),
			&features
		}
	);

	return std::make_pair(std::move(device),physicalDevice);
}

vk::PhysicalDeviceProperties properties;

vk::Format swapchainFormat;
vk::Extent2D swapchainExtent;

auto createSwapchain(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

	uint32_t result = physicalDevice.getSurfaceSupportKHR(0, surface);
	std::cout << "Result: " << result << std::endl;

	auto formats = physicalDevice.getSurfaceFormatsKHR(surface);

	swapchainFormat = formats[0].format;

	std::cout << "Printing formats" << std::endl;
	for(auto format : formats)
	{
		std::cout << vk::to_string(format.format) << ' ' << vk::to_string(format.colorSpace) << std::endl;
	}

	swapchainExtent = vk::Extent2D{
		std::clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
	};

	auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

	std::cout << "Printing present modes:" << std::endl;
	for(auto mode : presentModes)
	{
		std::cout << vk::to_string(mode) << std::endl;
	}

	return device.createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR{
		{}, surface,
		capabilities.minImageCount,
		swapchainFormat, vk::ColorSpaceKHR::eSrgbNonlinear,
		swapchainExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		0, nullptr,
		vk::SurfaceTransformFlagBitsKHR::eIdentity,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eFifo,
		true
	});

}

auto createComputePipeline(vk::Device device, vk::PipelineLayout pipelineLayout)
{

	auto module = createShaderModule(device, "shaders/shader.comp.spv");
	return device.createComputePipelineUnique({}, 
		vk::ComputePipelineCreateInfo{
			{},
			vk::PipelineShaderStageCreateInfo{{},vk::ShaderStageFlagBits::eCompute, module.get(), "main"},
			pipelineLayout
		}
	);
}

auto createRenderPass(vk::Device device)
{

	std::vector<vk::AttachmentDescription> attachments {
		vk::AttachmentDescription{
			{},
			swapchainFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		}
	};

	std::vector<vk::AttachmentReference> attachmentReferences {
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal}
	};

	

	std::vector<vk::SubpassDescription> subpasses {
		vk::SubpassDescription{
			{},
			vk::PipelineBindPoint::eGraphics,
			0, nullptr,
			static_cast<uint32_t>(attachmentReferences.size()), attachmentReferences.data()
		}
	};

	std::vector<vk::SubpassDependency> depencies {

	};

	return device.createRenderPassUnique(
		vk::RenderPassCreateInfo{
			{},
			static_cast<uint32_t>(attachments.size()), attachments.data(),
			static_cast<uint32_t>(subpasses.size()), subpasses.data(),
			static_cast<uint32_t>(depencies.size()), depencies.data(),
		}
	);

}


struct vertex_data
{
	BOOST_HANA_DEFINE_STRUCT(vertex_data,
		(glm::vec2, pos)
	);

};

struct instance_data
{
	BOOST_HANA_DEFINE_STRUCT(instance_data,
		(glm::vec2, pos),
		(glm::vec3, colour)
	);
};

#define NUMPOINTS 32

#define INDEX_TYPE uint32_t
#if INDEX_TYPE == uint32_t
#define VK_INDEX_TYPE vk::IndexType::eUint32
#elif INDEX_TYPE == uint16_t
#define VK_INDEX_TYPE vk::IndexType::eUint16
#endif


auto idx_tpl = hana::to<hana::tuple_tag>(hana::range_c<INDEX_TYPE, 0, NUMPOINTS>);
auto vtx_tpl = hana::append(hana::transform(idx_tpl, [](auto i)
{
	constexpr double angle = hana::value(i) * (2 * M_PI/NUMPOINTS);
	return vertex_data{ glm::vec2(std::cos(angle), std::sin(angle)) };
}), vertex_data{glm::vec2()});

auto vertices = hana::unpack(vtx_tpl,to_array<vertex_data>);
auto indices = hana::unpack(hana::flatten(hana::transform(idx_tpl, [](auto i) {
	return hana::make_tuple(static_cast<INDEX_TYPE>(NUMPOINTS), hana::value(i), (hana::value(i) + 1) % NUMPOINTS);
})),to_array<uint32_t>);

std::array<instance_data, NPARTICLES> instances;

inline size_t pad_up(size_t offset, size_t alignment)
{
	if ((offset & (alignment - 1)) == 0)
		return offset;
	return (offset & !(alignment - 1)) + alignment;
}

size_t instancesOffset;
size_t indicesOffset;

std::pair<VmaAllocation, vk::Buffer> make_vertex_buffer(VmaAllocator allocator)
{
	size_t verticesSize = vertices.size() * sizeof(vertex_data);
	size_t instancesSize = instances.size() * sizeof(instance_data);
	size_t indicesSize = indices.size() * sizeof(INDEX_TYPE);

	instancesOffset = pad_up(verticesSize, sizeof(instance_data));
	indicesOffset = pad_up(instancesOffset+ instancesSize, sizeof(INDEX_TYPE));


	size_t bufferSize = indicesOffset + indicesSize;


	VmaAllocation allocation;
	VkBuffer buffer;

	vk::BufferCreateInfo bufferInfo = { {}, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer };

	VmaAllocationCreateInfo allocationInfo = {};
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateBuffer(allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocationInfo, &buffer, &allocation, nullptr);
	return std::make_pair(allocation, vk::Buffer(buffer));
}


//struct Vertex {};


/*auto createGraphicsPipeline(vk::Device device, vk::RenderPass renderPass, vk::PipelineLayout pipelineLayout)
{
	auto vertexShader = createShaderModule(device, "shaders/shader.vert.spv"), fragmentShader = createShaderModule(device, "shaders/shader.frag.spv");

	std::vector<vk::PipelineShaderStageCreateInfo> stages{
		vk::PipelineShaderStageCreateInfo{
			{},
			vk::ShaderStageFlagBits::eVertex,
			vertexShader.get(),
			"main",
			
		},
		vk::PipelineShaderStageCreateInfo{
			{},
			vk::ShaderStageFlagBits::eFragment,
			fragmentShader.get(),
			"main",
			
		},
	};

	vk::VertexInputBindingDescription vertexBinding{0,sizeof(Vertex),vk::VertexInputRate::eVertex};
	vk::VertexInputAttributeDescription vertexAttribute{0,0, vk::Format::eR32G32B32Sfloat, 0};
	vk::PipelineVertexInputStateCreateInfo vertexInput{ {}, 1, &vertexBinding, 1, &vertexAttribute };
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{{},vk::PrimitiveTopology::ePointList, false};
	//vk::PipelineTessellationStateCreateInfo tesselation{{}, ?};
	vk::Viewport viewport{0.0f,0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f};
	vk::Rect2D scissor{{},swapchainExtent};
	vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &viewport, 1, &scissor};

	vk::PipelineRasterizationStateCreateInfo rasterizationState{
		{},
		false,
		false,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
		false, 0.0f, 0.0f, 0.0f,
		1.0f
	};

	vk::PipelineMultisampleStateCreateInfo multisampleState;

	//vk::PipelineDepthStencilStateCreateInfo depthStencilState;

	std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments {
		vk::PipelineColorBlendAttachmentState().setColorWriteMask(
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA
		)
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendState{
		{},
		false,
		vk::LogicOp::eCopy,
		static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data(),
	};

	//vk::PipelineDynamicStateCreateInfo dynamicState;

	return device.createGraphicsPipelineUnique({}, 
		vk::GraphicsPipelineCreateInfo{
			{},
			static_cast<uint32_t>(stages.size()),stages.data(),
			&vertexInput,
			&inputAssemblyState,
			nullptr,
			&viewportState,
			&rasterizationState,
			&multisampleState,
			nullptr,
			&colorBlendState,
			nullptr,
			pipelineLayout,
			renderPass, 0
		}
	);
}*/

template<typename Func>
auto createCommandBuffers(vk::Device device, size_t size, Func func)
{
	auto commandPool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo{{}, 0});
	auto commandBuffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{commandPool.get(), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(size)});

	for(size_t i = 0; i < commandBuffers.size(); ++i)
	{
		auto cb = commandBuffers[i];

		cb.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse});
		func(cb, i);
		cb.end();
	}

	return std::make_pair(std::move(commandPool),std::move(commandBuffers));
}


float rand_float()
{
	return rand() / float(RAND_MAX);
}

bool render = true;

void mousebuttonfun(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
		render = true;
}

int vkmain()
{

	for(auto& extension : vk::enumerateInstanceExtensionProperties())
	{
		std::cout << "Extension: " << extension.extensionName << '(' << extension.specVersion << ')' << std::endl;
	}
	auto window = createWindow();
	auto instance = createInstance();
	auto surface = createSurface(instance.get(), window.get());

	auto debugReportCallback = createDebugThinger(instance.get());

	auto [device,physicalDevice] = createDevice(instance.get(), surface.get());
	
	properties = physicalDevice.getProperties();

	auto pipelineQueue = device->getQueue(pipelineQueueFamily.first, pipelineQueueFamily.first),
		 transferQueue = device->getQueue(transferQueueFamily.first, transferQueueFamily.second),
		 presentQueue = device->getQueue(presentQueueFamily.first, presentQueueFamily.second);

	auto swapchain = createSwapchain(device.get(), physicalDevice, surface.get());

	auto swapchainImages = device->getSwapchainImagesKHR(swapchain.get());
	
	std::vector<vk::UniqueImageView> swapchainImageViews;
	swapchainImageViews.reserve(swapchainImages.size());
	std::transform(
		std::begin(swapchainImages),
		std::end(swapchainImages),
		std::back_inserter(swapchainImageViews),
		[device=device.get()](auto image){
			return device.createImageViewUnique(
				vk::ImageViewCreateInfo{
					{},
					image,
					vk::ImageViewType::e2D,
					swapchainFormat,
					{},
					vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
				}
			);
		}
	);

	/*std::vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
	};

	auto descriptorSetLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo{ {}, static_cast<uint32_t>(bindings.size()), bindings.data()});

	auto computePipelineLayout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{{}, 1, &descriptorSetLayout.get()});
	auto computePipeline = createComputePipeline(device.get(), computePipelineLayout.get());*/

	auto renderPass = createRenderPass(device.get());
	auto graphicsPipelineLayout = device->createPipelineLayoutUnique({});

	auto shader = create_shader(device.get(), "shaders/shader.vert.spv");


	auto layout = create_vertex_input_layout(hana::make_tuple(
		hana::make_pair(hana::type_c<vertex_data>, hana::bool_c<false>),
		hana::make_pair(hana::type_c<instance_data>, hana::bool_c<true>)
	));
	auto pipeline = make_graphics_pipeline(
		layout,
		ENUM_CONSTANT(vk::PrimitiveTopology, eTriangleList),
		ENUM_CONSTANT(vk::PolygonMode, eFill)
	);
	
	dump_layout(layout);


	pipeline.create(device.get(), renderPass.get(), { "shaders/shader.vert.spv",  "shaders/shader.frag.spv" });

	auto graphicsPipeline = pipeline.m_pipeline.get();//createGraphicsPipeline(device.get(), renderPass.get(), graphicsPipelineLayout.get());


	std::vector<vk::UniqueFramebuffer> framebuffers;
	framebuffers.reserve(swapchainImageViews.size());
	transform(
		std::begin(swapchainImageViews),
		std::end(swapchainImageViews),
		std::back_inserter(framebuffers),
		[device=device.get(),renderPass=renderPass.get()](const auto& imageView)
		{
			return device.createFramebufferUnique(
				vk::FramebufferCreateInfo{
					{},
					renderPass,
					1, &imageView.get(),
					swapchainExtent.width, swapchainExtent.height,
					1
				}
			);
		}
	);


	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device.get();

	VmaAllocator allocator;
	vmaCreateAllocator(&allocatorInfo, &allocator);

	auto vertex_buffer_pair = make_vertex_buffer(allocator);

	//vk::DescriptorPoolSize poolSize{vk::DescriptorType::eStorageBuffer,2};
	//auto descriptorPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{{}, 2, 1, &poolSize});
	//auto descriptorSets = device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{descriptorPool.get(), 1, &descriptorSetLayout.get()});


	//vk::DescriptorBufferInfo bufferInfo{vertex_buffer_pair.second, 0, vertex_buffer_pair.first->GetSize()};
	//device->updateDescriptorSets({vk::WriteDescriptorSet{descriptorSets[0],0,0,1,vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfo}},{});

	auto fence = device->createFenceUnique(vk::FenceCreateInfo{});


	auto [commandPool, commandBuffers] = createCommandBuffers(
		device.get(), framebuffers.size(),
		[
			renderPass=renderPass.get(),
			graphicsPipeline=graphicsPipeline,
			//computePipeline = computePipeline.get(),
			&framebuffers,
			&vertex_buffer_pair
			//&descriptorSets,
			//computePipelineLayout=computePipelineLayout.get(),
			//family = pipelineQueueFamily.first
		](vk::CommandBuffer cb, size_t i)
		{

			//cb.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
			//cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipelineLayout, 0, descriptorSets, {});
			//cb.dispatch(NPARTICLES, 1, 1);

			/*cb.pipelineBarrier(
				vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAllGraphics,
				{},
				{},
				{
					vk::BufferMemoryBarrier{vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderRead, family, family, vertex_buffer_pair.second, 0, sizeof(Vertex) * NPARTICLES }
				},
				{}
			);*/

			std::vector<vk::ClearValue> clearValues {
				vk::ClearColorValue().setFloat32({0.0f, 0.0f, 0.0f, 1.0f})
			};

			cb.beginRenderPass(vk::RenderPassBeginInfo{
				renderPass,
				framebuffers[i].get(),
				vk::Rect2D{{}, swapchainExtent},
				static_cast<uint32_t>(clearValues.size()), clearValues.data()
			}, vk::SubpassContents::eInline);

			cb.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

			cb.bindVertexBuffers(0, { vertex_buffer_pair.second, vertex_buffer_pair.second }, { 0, instancesOffset });
			cb.bindIndexBuffer(vertex_buffer_pair.second, indicesOffset, VK_INDEX_TYPE);

			cb.drawIndexed(static_cast<uint32_t>(indices.size()), NPARTICLES, 0, 0, 0);

			cb.endRenderPass();
		}
	);

	auto transferCommandPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{ {}, transferQueueFamily.first });

	{
		std::srand(static_cast<unsigned int>(time(NULL)));
		std::generate(
			instances.begin(),
			instances.end(),
			[]()
			{
				return instance_data{
					{ 2*rand_float()-1.0f, 2*rand_float()-1.0f },
					{ rand_float(), rand_float(), rand_float() }
				};
			}
		);

		vk::BufferCreateInfo bufferInfo = { {}, vertex_buffer_pair.first->GetSize(), vk::BufferUsageFlagBits::eTransferSrc};

		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		vmaCreateBuffer(allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocationInfo, &stagingBuffer, &stagingAllocation, nullptr);

		char* data;
		vmaMapMemory(allocator, stagingAllocation, reinterpret_cast<void**>(&data));
		 
		memcpy(data, vertices.data(), vertices.size() * sizeof(vertex_data));
		memcpy(data + instancesOffset, instances.data(), instances.size() * sizeof(instance_data));
		memcpy(data + indicesOffset, indices.data(), indices.size() * sizeof(INDEX_TYPE));

		//start->pos[0] = 0.5f, start->pos[1] = -0.5f, start->pos[2] = 0.0f;
		//start->vel[0] = -10.0f, start->vel[1] = 0.0f, start->vel[2] = 0.0f;

		vmaUnmapMemory(allocator, stagingAllocation);

		auto transferCommandBuffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ transferCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 });
		auto cb = transferCommandBuffers[0].get();
		
		cb.begin(vk::CommandBufferBeginInfo{});
		cb.copyBuffer(stagingBuffer, vertex_buffer_pair.second, { vk::BufferCopy{0, 0, vertex_buffer_pair.first->GetSize()} });
		cb.end();

		transferQueue.submit(
			{
				vk::SubmitInfo{
					0, nullptr, nullptr,
					1, &cb,
					0, nullptr
				}
			},
			nullptr
		);

		transferQueue.waitIdle();

		vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	}

	auto imageAvailableSemaphore = device->createSemaphoreUnique({}), imageReadySemaphore = device->createSemaphoreUnique({});
	auto renderFinishedFence = device->createFenceUnique({});

	glfwSetMouseButtonCallback(window.get(), mousebuttonfun);

	render = false;

	while (!glfwWindowShouldClose(window.get())) 
	{

		auto result = device->acquireNextImageKHR(swapchain.get() , std::numeric_limits<uint64_t>::max(),imageAvailableSemaphore.get(), nullptr);
		uint32_t idx = result.value;

		//std::cout << idx << ':' << commandBuffers.size() << std::endl;

		vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		pipelineQueue.submit({
			vk::SubmitInfo{
				1, &imageAvailableSemaphore.get(), &waitStage,
				1, &commandBuffers[idx],
				1, &imageReadySemaphore.get()
			}
		}, renderFinishedFence.get());

		presentQueue.presentKHR(
			vk::PresentInfoKHR{
				1, &imageReadySemaphore.get(),
				1, &swapchain.get(),
				&idx
			}
		);

		device->waitForFences({ renderFinishedFence.get() }, true, std::numeric_limits<uint64_t>::max());
		device->resetFences({ renderFinishedFence.get() });


		glfwPollEvents();

    }

	vmaDestroyBuffer(allocator, vertex_buffer_pair.second, vertex_buffer_pair.first);
	vmaDestroyAllocator(allocator);

	return 0;
}

int main()
{
	try {
		return vkmain();
	}
	catch(std::runtime_error e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}