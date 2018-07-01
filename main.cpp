#include <iostream>
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

using namespace std;

using buffer_handle = size_t;
using memory_handle = size_t;

vk::UniqueInstance instance;
vk::UniqueDevice device;

template<typename T, typename U>
T pad_up(T offset, U alignment)
{
	if(alignment == 0 || (offset & (alignment-1)) == 0)
		return offset;
	
	return (offset & !(alignment-1)) + alignment;
}

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

	friend buffer create_buffer(size_t, size_t, uint32_t, uint32_t);
};

struct vk_buffer
{
	vk_buffer(uint32_t flags) :
		m_flags(flags),
		m_size(0)
	{
	}

	uint32_t m_flags;

	vk::Buffer m_handle;

	memory_handle m_memory;
	vk::DeviceSize m_offset;
	vk::DeviceSize m_size;
};

vector<vk_buffer> buffers;

buffer create_buffer(size_t size, size_t alignment, uint32_t usage, uint32_t queueFlags)
{
	uint32_t flags = usage << 9 | queueFlags;

	auto it = std::find_if(buffers.begin(), buffers.end(), [flags](const vk_buffer& buffer){
		return buffer.m_flags == flags && !buffer.m_handle;
	});

	if(it == buffers.end())
	{
		buffers.emplace_back(flags);
		it = prev(buffers.end());
	}

	size_t offset = pad_up(it->m_size, alignment);
	it->m_size = offset+size;

	return buffer(distance(buffers.begin(),it), offset, size);
}

struct vk_memory
{
	uint32_t m_flags;
	size_t m_size;
};

vector<vk_memory> memory;

void commit_allocs()
{
	for(auto& buffer : buffers)
	{
		if(!buffer.m_handle)
		{

			VkBufferCreateInfo createInfo = {};

			buffer.m_handle = device->createBuffer({});
		}
	}

}

int main()
{

	cout << "Hello world" << endl;

	vk::ApplicationInfo appInfo{"vktest", 0, "Pomme", 0, VK_API_VERSION_1_1};

	vector<const char*> layers{"VkLayer_standard_validation"}, extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	instance = vk::createInstanceUnique(
		vk::InstanceCreateInfo{
			{}, &appInfo,
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data()
		}
	);

	auto physicalDevice = instance->enumeratePhysicalDevices().front();

	vector<vk::DeviceQueueCreateInfo> queues{
		{}
	};
	vk::PhysicalDeviceFeatures features;
	
	physicalDevice.createDeviceUnique(
		vk::DeviceCreateInfo{{},
			static_cast<uint32_t>(queues.size()),	  queues.data(),
			static_cast<uint32_t>(layers.size()),	  layers.data(),
			static_cast<uint32_t>(extensions.size()), extensions.data(),
			&features
		}
	);

	return 0;
}

