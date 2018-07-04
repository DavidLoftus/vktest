#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "termcolor.hpp"

using namespace std;

using buffer_handle = size_t;
using memory_handle = size_t;


/*
{
template<typename T, typename U>
T pad_up(T offset, U alignment)
{
	if(alignment == 0 || (offset & (alignment-1)) == 0)
		return offset;
	
	return (offset & !(alignment-1)) + alignment;
}

enum QueueFlags
{
	GRAPHICS,
	PRESENT,
	TRANSFER,
};

class buffer
{
private:
	buffer_handle m_handle;
	size_t m_offset;
	size_t m_size;
	bool m_initialized;

	buffer(buffer_handle handle, size_t offset, size_t size) :
		m_handle(handle),
		m_offset(offset),
		m_size(size)
	{
	}

	friend buffer create_buffer(size_t size, size_t alignment, vk::BufferUsageFlags usage, uint32_t queueFlags);
};

struct vk_buffer
{
	vk_buffer(vk::BufferUsageFlags usageFlags, uint32_t queueFlags) :
		m_usageFlags(usageFlags),
		m_queueFlags(queueFlags),
		m_size(0)
	{
	}

	vk::BufferUsageFlags m_usageFlags;
	uint32_t m_queueFlags;

	vk::Buffer m_handle;

	memory_handle m_memory;
	vk::DeviceSize m_offset;
	vk::DeviceSize m_size;
};

vector<vk_buffer> buffers;

buffer create_buffer(size_t size, size_t alignment, vk::BufferUsageFlags usage, uint32_t queueFlags)
{

	auto it = std::find_if(buffers.begin(), buffers.end(), [usage,queueFlags](const vk_buffer& buffer){
		return !buffer.m_handle && buffer.m_usageFlags & usage && buffer.m_queueFlags & queueFlags; // Check pending buffers for ability to contain new buffer
	});

	if(it == buffers.end())
	{
		buffers.emplace_back(usage,queueFlags);
		it = prev(buffers.end());
	}

	size_t offset = pad_up(it->m_size, alignment);
	it->m_size = offset+size;

	return buffer(distance(buffers.begin(),it), offset, size);
}

struct vk_memory
{
	vk::DeviceMemory m_handle;

	uint32_t m_flags;
	size_t m_size;
};

vector<vk_memory> memory;

size_t reserve_memory(size_t size)
{



}

int somefunc(VkBuffer* test)
{

}

void commit_allocs(vk::Device device)
{

	for(auto& buffer : buffers)
	{
		if(!buffer.m_handle)
		{

			vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
			vector<uint32_t> indices; // Empty for now because mac has only 1 queue family

			buffer.m_handle = device.createBuffer(
				vk::BufferCreateInfo{{},
					buffer.m_size,
					buffer.m_usageFlags,
					sharingMode,
					static_cast<uint32_t>(indices.size()), indices.data()
				}
			);

			size_t offset = reserve_memory(buffer.m_size, );

		}
	}

	for(auto& memoryObj : memory)
	{
		if(!memoryObj.m_handle)
		{

			uint32_t idx;

			

			memoryObj.m_handle = device.allocateMemory(vk::MemoryAllocateInfo{memoryObj.m_size,idx});
		}
	}
}

}*/

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
	//std::cout << '[' << vk::DebugReportFlagsEXT(objectType) << "] " << pMessage << endl;
	return false;
}

vk::UniqueShaderModule createShaderModule(vk::Device device, const std::string& path)
{
	vector<char> code;

	ifstream file("build/shader.comp.spirv", ios::in | ios::binary);

	if(!file) throw std::runtime_error("File " + path + " not found.");

	copy(istreambuf_iterator<char>(file), istreambuf_iterator<char>(), back_inserter(code));

	if(code.empty()) throw std::runtime_error("File " + path + " empty");

	return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{{}, static_cast<uint32_t>(code.size()), reinterpret_cast<uint32_t*>(code.data())});

}

const int WIDTH = 800, HEIGHT = 600;

auto createWindow()
{
	auto window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);

	glfwWindowHint(GLFW_RESIZABLE,false);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	return unique_ptr<GLFWwindow, void(*)(GLFWwindow* ptr)>(window,glfwDestroyWindow);
}

vector<const char*> layers{"VK_LAYER_LUNARG_standard_validation"}, extensions{VK_EXT_DEBUG_REPORT_EXTENSION_NAME,VK_KHR_SURFACE_EXTENSION_NAME};

auto createInstance()
{
	vk::ApplicationInfo appInfo{"vktest", 0, "Pomme", 0, VK_API_VERSION_1_1};
	return vk::createInstanceUnique(
		vk::InstanceCreateInfo{
			{}, &appInfo,
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data()
		}
	);
}

auto createDebugThinger(vk::Instance instance)
{
	return instance.createDebugReportCallbackEXTUnique(
		vk::DebugReportCallbackCreateInfoEXT{
			vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning,
			callback_fn
		}
	);
}

auto createDevice(vk::Instance instance)
{

	auto physicalDevice = instance.enumeratePhysicalDevices().front();
	for(auto extension : physicalDevice.enumerateDeviceExtensionProperties())
	{
		cout << "DeviceExtension: " << extension.extensionName << '(' << extension.specVersion << ')' << endl;
	}


	float priorities[2] = {1.0f, 1.0f};
	vector<vk::DeviceQueueCreateInfo> queues{
		vk::DeviceQueueCreateInfo{{}, 0, 2, priorities}
	};

	vk::PhysicalDeviceFeatures features;

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

auto createComputePipeline(vk::Device device, vk::PipelineLayout pipelineLayout)
{

	auto module = createShaderModule(device, "build/shader.comp.spirv");
	return device.createComputePipelineUnique({}, 
		vk::ComputePipelineCreateInfo{
			{},
			vk::PipelineShaderStageCreateInfo{{},vk::ShaderStageFlagBits::eCompute, module.get(), "main"},
			pipelineLayout
		}
	);
}

auto createGraphicsPipeline(vk::Device device)
{
		auto vertexShader = createShaderModule(device, "build/shader.vert.sprv"), fragmentShader = createShaderModule(device, "build/shader.frag.sprv");
		
		vector<vk::PipelineShaderStageCreateInfo> stages{
			vk::PipelineShaderStageCreateInfo{},
			vk::PipelineShaderStageCreateInfo{}
		};

		vk::VertexInputBindingDescription vertexBinding{0,sizeof(float)*3,vk::VertexInputRate::eVertex};
		vk::VertexInputAttributeDescription vertexAttribute{0,0, vk::Format::eR32G32B32Sfloat, 0};
		vk::PipelineVertexInputStateCreateInfo vertexInput{{}, 1, &vertexBinding, 1, &vertexAttribute};
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{{},vk::PrimitiveTopology::ePointList,0};
		//vk::PipelineTessellationStateCreateInfo tesselation{{}, ?};
		/*vk::Viewport viewport{0.0f,0.0f, swapchainExtent.width, swapchainExtent.height, 0.0f, 1.0f};
		vk::Rect2D{{},vk::Extent2D{,}};
		vk::PipelineViewportStateCreateInfo{{}, 1, &viewport, 1, &scissor};*/

		return device.createGraphicsPipelineUnique({}, 
			vk::GraphicsPipelineCreateInfo{
				{},
				static_cast<uint32_t>(stages.size()),stages.data(),
				&vertexInput,
				&inputAssembly,
				nullptr,

			}
		);
	}

int main()
{
	for(auto& extension : vk::enumerateInstanceExtensionProperties())
	{
		cout << "Extension: " << extension.extensionName << '(' << extension.specVersion << ')' << endl;
	}
	auto window = createWindow();

	auto instance = createInstance();

	auto debugReportCallback = createDebugThinger(instance.get());

	auto [device,physicalDevice] = createDevice(instance.get());

	auto transferQueue = device->getQueue(0, 0), computeQueue = device->getQueue(0, 1);


	vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
	};

	auto descriptorSetLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo{{}, static_cast<uint32_t>(bindings.size()), bindings.data()});

	auto pipelineLayout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{{}, 1, &descriptorSetLayout.get()});

	vk::UniquePipeline computePipeline = createComputePipeline(device.get(), pipelineLayout.get());

	vk::UniquePipeline graphicsPipeline = createGraphicsPipeline(device.get());


	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device.get();

	VmaAllocator allocator;
	vmaCreateAllocator(&allocatorInfo, &allocator);

	vk::Buffer buffer;
	VmaAllocation allocation;
	{
		vk::BufferCreateInfo bufferInfo = {{}, 2*3*sizeof(float)*256, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc};
		
		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		
		VkBuffer bufferhandle;
		vmaCreateBuffer(allocator, &static_cast<const VkBufferCreateInfo&>(bufferInfo), &allocationInfo, &bufferhandle, &allocation, nullptr);

		buffer = bufferhandle;

		void* data;
		vmaMapMemory(allocator, allocation, &data);

		float* start = reinterpret_cast<float*>(data);
		float* end = start+2*3*256;
		generate(start,end, [](){return rand()/float(RAND_MAX);});

		vmaUnmapMemory(allocator,allocation);
	}

	vk::DescriptorPoolSize poolSize{vk::DescriptorType::eStorageBuffer,2};
	auto descriptorPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{{}, 2, 1, &poolSize});
	auto descriptorSets = device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo{descriptorPool.get(), 1, &descriptorSetLayout.get()});


	vk::DescriptorBufferInfo bufferInfo{buffer,allocation->GetOffset(), allocation->GetSize()};
	device->updateDescriptorSets({vk::WriteDescriptorSet{descriptorSets[0],0,0,1,vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfo}},{});

	auto commandPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{{}, 0});
	auto commandBuffers = device->allocateCommandBuffers(vk::CommandBufferAllocateInfo{commandPool.get(), vk::CommandBufferLevel::ePrimary, 1});

	for(auto cb : commandBuffers)
	{
		cb.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse});
		cb.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			{}, 
			{
				vk::MemoryBarrier{vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead}
			},
			{},
			{}
		);
		cb.bindPipeline(vk::PipelineBindPoint::eCompute,computePipeline.get());
		cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, descriptorSets, {});
		cb.dispatch(1,1,1);
		cb.end();
	}

	auto fence = device->createFenceUnique(vk::FenceCreateInfo{});

	vector<vk::SubmitInfo> repeats;
	repeats.resize(125,vk::SubmitInfo{0,nullptr, nullptr, 1, commandBuffers.data()});

	cout << "Press enter to submit:" << endl;
	cin.get();
	computeQueue.submit(repeats,fence.get());

	auto t0 = chrono::high_resolution_clock::now();
	device->waitForFences({fence.get()}, true, numeric_limits<uint32_t>::max());
	auto t1 = chrono::high_resolution_clock::now();

	chrono::duration<double,milli> dif = t1 - t0;

	cout << "125 took " << dif.count() << "ms." << endl;


	{
		void* data;
		vmaMapMemory(allocator, allocation, &data);

		float* floats = reinterpret_cast<float*>(data);
		for(int i = 0; i < 256; ++i)
		{
			cout << i << ": " <<  floats[i*6+1] << endl;
		}

		vmaUnmapMemory(allocator,allocation);
	}
	vmaDestroyBuffer(allocator, buffer, allocation);
	vmaDestroyAllocator(allocator);

	return 0;
}
