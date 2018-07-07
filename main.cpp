#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <memory>


#include <vulkan/vulkan.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define NOMINMAX
#include "termcolor.hpp"

//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define RENDERDOC

using namespace std;

using buffer_handle = size_t;
using memory_handle = size_t;

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

ostream& operator<<(ostream& os, vk::DebugReportFlagsEXT flags)
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
	cout << '[' << vk::DebugReportFlagsEXT(flags) << "] " << pMessage << endl;
	
	return false;
}

vk::UniqueShaderModule createShaderModule(vk::Device device, const std::string& path)
{
	vector<char> code;

	ifstream file(path, ios::in | ios::binary);

	if(!file) throw std::runtime_error("File " + path + " not found.");

	copy(istreambuf_iterator<char>(file), istreambuf_iterator<char>(), back_inserter(code));

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

	return unique_ptr<GLFWwindow, void(*)(GLFWwindow* ptr)>(window,glfwDestroyWindow);
}

vector<const char*> layers{
	"VK_LAYER_LUNARG_standard_validation",
#ifdef RENDERDOC
	"VK_LAYER_RENDERDOC_Capture"
#endif
}, extensions{VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

auto createInstance()
{	

	for (auto& layer : vk::enumerateInstanceLayerProperties())
	{
		cout << layer.layerName << ": " << layer.description << endl;
	}

	uint32_t count;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

	vector<const char*> extensions_copy = extensions;
	extensions_copy.insert(extensions_copy.end(), glfwExtensions, &glfwExtensions[count]);

	for(auto extension : extensions_copy) cout << "InstanceExtension " << extension << endl;

	vk::ApplicationInfo appInfo{"vktest", 0, "Pomme", 0, VK_API_VERSION_1_1};
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
			vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning,
			callback_fn,
			nullptr
		}
	);
}

template<typename Func>
auto findQueue(std::vector<vk::QueueFamilyProperties>& queuePropertiesList, Func func)
{
#ifdef RENDERDOC
	return make_pair(0, 0);
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
		cerr << queuePropertiesList.size() << endl;
		throw runtime_error("No appropriate queue.");
	}

	return make_pair(i, it->queueCount--);
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

pair<uint32_t,uint32_t> pipelineQueueFamily, transferQueueFamily, presentQueueFamily; // <family,queue>

auto createDevice(vk::Instance instance, vk::SurfaceKHR surface)
{
	auto physicalDevices = instance.enumeratePhysicalDevices();

	for (auto& physicalDevice : physicalDevices)
	{
		auto properties = physicalDevice.getProperties();
		cout << "PhysicalDevice " << properties.deviceName << " " << vk::to_string(properties.deviceType) << endl;
	}

	auto physicalDevice = physicalDevices.front();
	for(auto extension : physicalDevice.enumerateDeviceExtensionProperties())
	{
		cout << "DeviceExtension: " << extension.extensionName << '(' << extension.specVersion << ')' << endl;
	}

	auto queuePropertiesList = physicalDevice.getQueueFamilyProperties();

	for (auto& prop : queuePropertiesList)
		cout << vk::to_string(prop.queueFlags) << ": " << prop.queueCount << endl;

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
	vector<vk::DeviceQueueCreateInfo> queues{ vk::DeviceQueueCreateInfo{ {}, 0, 1, priorities } };
#else
	vector<vk::DeviceQueueCreateInfo> queues;
	transform(
		queueCounts.begin(),
		queueCounts.end(),
		back_inserter(queues),
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

	return make_pair(move(device),physicalDevice);
}

vk::Format swapchainFormat;
vk::Extent2D swapchainExtent;

auto createSwapchain(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

	uint32_t result = physicalDevice.getSurfaceSupportKHR(0, surface);
	cout << "Result: " << result << endl;

	auto formats = physicalDevice.getSurfaceFormatsKHR(surface);

	swapchainFormat = formats[0].format;

	cout << "Printing formats" << endl;
	for(auto format : formats)
	{
		cout << vk::to_string(format.format) << ' ' << vk::to_string(format.colorSpace) << endl;
	}

	swapchainExtent = vk::Extent2D{
		clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
	};

	auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

	cout << "Printing present modes:" << endl;
	for(auto mode : presentModes)
	{
		cout << vk::to_string(mode) << endl;
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

	vector<vk::AttachmentDescription> attachments {
		vk::AttachmentDescription{
			{},
			swapchainFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		}
	};

	vector<vk::AttachmentReference> attachmentReferences {
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal}
	};

	

	vector<vk::SubpassDescription> subpasses {
		vk::SubpassDescription{
			{},
			vk::PipelineBindPoint::eGraphics,
			0, nullptr,
			static_cast<uint32_t>(attachmentReferences.size()), attachmentReferences.data()
		}
	};

	vector<vk::SubpassDependency> depencies {

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

auto createGraphicsPipeline(vk::Device device, vk::RenderPass renderPass, vk::PipelineLayout pipelineLayout)
{
	auto vertexShader = createShaderModule(device, "shaders/shader.vert.spv"), fragmentShader = createShaderModule(device, "shaders/shader.frag.spv");

	vector<vk::PipelineShaderStageCreateInfo> stages{
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

	vk::VertexInputBindingDescription vertexBinding{0,sizeof(float)*6,vk::VertexInputRate::eVertex};
	vk::VertexInputAttributeDescription vertexAttribute{0,0, vk::Format::eR32G32B32Sfloat, 0};
	vk::PipelineVertexInputStateCreateInfo vertexInput{{}, 1, &vertexBinding, 1, &vertexAttribute};
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{{},vk::PrimitiveTopology::ePointList,0};
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

	vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments {
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
}

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

	return make_pair(move(commandPool),move(commandBuffers));
}

struct Vertex
{
	float pos[3];
	float vel[3];
};

float rand_float()
{
	return rand() / float(RAND_MAX);
}

int vkmain()
{
	for(auto& extension : vk::enumerateInstanceExtensionProperties())
	{
		cout << "Extension: " << extension.extensionName << '(' << extension.specVersion << ')' << endl;
	}
	auto window = createWindow();
	auto instance = createInstance();
	auto surface = createSurface(instance.get(), window.get());

	auto debugReportCallback = createDebugThinger(instance.get());

	auto [device,physicalDevice] = createDevice(instance.get(), surface.get());
	

	auto pipelineQueue = device->getQueue(pipelineQueueFamily.first, pipelineQueueFamily.first),
		 transferQueue = device->getQueue(transferQueueFamily.first, transferQueueFamily.second),
		 presentQueue = device->getQueue(presentQueueFamily.first, presentQueueFamily.second);

	auto swapchain = createSwapchain(device.get(), physicalDevice, surface.get());

	auto swapchainImages = device->getSwapchainImagesKHR(swapchain.get());
	
	vector<vk::UniqueImageView> swapchainImageViews;
	swapchainImageViews.reserve(swapchainImages.size());
	transform(
		begin(swapchainImages),
		end(swapchainImages),
		back_inserter(swapchainImageViews),
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

	vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
	};

	auto descriptorSetLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo{{}, static_cast<uint32_t>(bindings.size()), bindings.data()});

	auto computePipelineLayout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{{}, 1, &descriptorSetLayout.get()});
	auto computePipeline = createComputePipeline(device.get(), computePipelineLayout.get());

	auto renderPass = createRenderPass(device.get());
	auto graphicsPipelineLayout = device->createPipelineLayoutUnique({});;
	auto graphicsPipeline = createGraphicsPipeline(device.get(), renderPass.get(), graphicsPipelineLayout.get());


	vector<vk::UniqueFramebuffer> framebuffers;
	framebuffers.reserve(swapchainImageViews.size());
	transform(
		begin(swapchainImageViews),
		end(swapchainImageViews),
		back_inserter(framebuffers),
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

	vk::Buffer buffer;
	VmaAllocation allocation;
	{
		vk::BufferCreateInfo bufferInfo = {{}, sizeof(Vertex)*256, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eVertexBuffer};
		
		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		
		VkBuffer bufferhandle;
		vmaCreateBuffer(allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocationInfo, &bufferhandle, &allocation, nullptr);
		buffer = bufferhandle;
	}

	vk::DescriptorPoolSize poolSize{vk::DescriptorType::eStorageBuffer,2};
	auto descriptorPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{{}, 2, 1, &poolSize});
	auto descriptorSets = device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{descriptorPool.get(), 1, &descriptorSetLayout.get()});


	vk::DescriptorBufferInfo bufferInfo{buffer, 0, allocation->GetSize()};
	device->updateDescriptorSets({vk::WriteDescriptorSet{descriptorSets[0],0,0,1,vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfo}},{});

	auto fence = device->createFenceUnique(vk::FenceCreateInfo{});


	auto [commandPool, commandBuffers] = createCommandBuffers(
		device.get(), framebuffers.size(),
		[
			renderPass=renderPass.get(),
			graphicsPipeline=graphicsPipeline.get(),
			computePipeline = computePipeline.get(),
			&framebuffers,
			buffer,
			&descriptorSets,
			computePipelineLayout=computePipelineLayout.get(),
			family = pipelineQueueFamily.first
		](vk::CommandBuffer cb, size_t i)
		{

			cb.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
			cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computePipelineLayout, 0, descriptorSets, {});
			cb.dispatch(1, 1, 1);

			cb.pipelineBarrier(
				vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAllGraphics,
				{},
				{},
				{
					vk::BufferMemoryBarrier{vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderRead, family, family, buffer, 0, sizeof(Vertex) * 256 }
				},
				{}
			);

			vector<vk::ClearValue> clearValues {
				vk::ClearColorValue().setFloat32({1.0f, 0.0f, 0.0f, 1.0f})
			};

			cb.beginRenderPass(vk::RenderPassBeginInfo{
				renderPass,
				framebuffers[i].get(),
				vk::Rect2D{{}, swapchainExtent},
				static_cast<uint32_t>(clearValues.size()), clearValues.data()
			}, vk::SubpassContents::eInline);

			cb.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

			cb.bindVertexBuffers(0, {buffer}, {0});

			cb.draw(256, 1, 0, 0);

			cb.endRenderPass();
		}
	);

	auto transferCommandPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{ {}, transferQueueFamily.first });

	{
		vk::BufferCreateInfo bufferInfo = { {}, allocation->GetSize(), vk::BufferUsageFlagBits::eTransferSrc};

		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		vmaCreateBuffer(allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocationInfo, &stagingBuffer, &stagingAllocation, nullptr);

		void* data;
		vmaMapMemory(allocator, stagingAllocation, &data);

		auto start = reinterpret_cast<Vertex*>(data);
		srand(static_cast<uint32_t>(time(NULL)));
		generate_n(
			start,
			256,
			[]()
		{
			return Vertex{
				{ rand_float(), rand_float(), rand_float() },
				{ rand_float(), rand_float(), rand_float() }
			};
		}
		);

		vmaUnmapMemory(allocator, stagingAllocation);

		auto transferCommandBuffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ transferCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 });
		auto cb = transferCommandBuffers[0].get();
		
		cb.begin(vk::CommandBufferBeginInfo{});
		cb.copyBuffer(stagingBuffer, buffer, { vk::BufferCopy{0, 0, allocation->GetSize()} });
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

	while (!glfwWindowShouldClose(window.get())) 
	{
		glfwWaitEvents();
		glfwPollEvents();

		auto result = device->acquireNextImageKHR(swapchain.get() ,numeric_limits<uint64_t>::max(),imageAvailableSemaphore.get(), nullptr);
		uint32_t idx = result.value;

		//cout << idx << ':' << commandBuffers.size() << endl;

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

		device->waitForFences({ renderFinishedFence.get() }, true, numeric_limits<uint64_t>::max());
		device->resetFences({ renderFinishedFence.get() });

    }

	{
		vk::BufferCreateInfo bufferInfo = { {}, allocation->GetSize(), vk::BufferUsageFlagBits::eTransferDst };

		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		vmaCreateBuffer(allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocationInfo, &stagingBuffer, &stagingAllocation, nullptr);

		auto transferCommandBuffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{ transferCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1 });
		auto cb = transferCommandBuffers[0].get();

		cb.begin(vk::CommandBufferBeginInfo{});
		cb.copyBuffer(buffer, stagingBuffer, { vk::BufferCopy{ 0, 0, allocation->GetSize() } });
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

		void* data;
		vmaMapMemory(allocator, stagingAllocation, &data);

		auto start = reinterpret_cast<Vertex*>(data);
		for_each_n(
			start,
			256,
			[](const Vertex& vertex)
		{
			cout << vertex.pos[0] << ", " << vertex.pos[1] << endl;
		});

		vmaUnmapMemory(allocator, stagingAllocation);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	}

	while (!glfwWindowShouldClose(window.get())) glfwPollEvents();

	vmaDestroyBuffer(allocator, buffer, allocation);
	vmaDestroyAllocator(allocator);

	return 0;
}


int main()
{
	ofstream file("hello.txt");
	file << "hi";
	file.close();

	try {
		return vkmain();
	}
	catch(std::runtime_error e)
	{
		cerr << e.what() << endl;
		return 1;
	}
}