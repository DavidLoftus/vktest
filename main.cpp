#include <iostream>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.hpp>


#include "termcolor.hpp"

using namespace std;

using buffer_handle = size_t;
using memory_handle = size_t;

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
		}
	}

}

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
		os << termcolor::dark << "UNKNOWN";

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
	std::cout << '[' << vk::DebugReportFlagsEXT(objectType) << "]\t" << pMessage << endl;
	return false;
}

int main()
{

	cout << "Hello world" << endl;

	vk::ApplicationInfo appInfo{"vktest", 0, "Pomme", 0, VK_API_VERSION_1_1};

	vector<const char*> layers{"VK_LAYER_LUNARG_standard_validation"}, extensions{VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

	auto instance = vk::createInstanceUnique(
		vk::InstanceCreateInfo{
			{}, &appInfo,
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data()
		}
	);

	auto debugReportCallback = instance->createDebugReportCallbackEXTUnique(
		vk::DebugReportCallbackCreateInfoEXT{
			vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning,
			callback_fn
		}
	);

	auto physicalDevice = instance->enumeratePhysicalDevices().front();

	auto extensionList = physicalDevice.enumerateDeviceExtensionProperties();
	for(auto& extension : extensionList)
	{
		cout << extension.extensionName << endl;
	}

	auto familyProperties = physicalDevice.getQueueFamilyProperties();

	int i = 0;
	for(auto& family : familyProperties)
	{
		cout << i++ << ' ' << family.queueCount << ' ' << vk::to_string(family.queueFlags) << endl;
	}

	vector<vk::DeviceQueueCreateInfo> queues{
		vk::DeviceQueueCreateInfo{{}, 0, 0, nullptr}
	};

	vk::PhysicalDeviceFeatures features;

	extensions.erase(extensions.begin());
	
	auto device = physicalDevice.createDeviceUnique(
		vk::DeviceCreateInfo{{},
			static_cast<uint32_t>(queues.size()),	  queues.data(),
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data(),
			&features
		}
	);

	buffer buf0 = create_buffer(1024, 256, vk::BufferUsageFlagBits::eVertexBuffer, GRAPHICS | TRANSFER);

	commit_allocs(device.get());

	device->destroy(buffers[0].m_handle);

	return 0;
}
