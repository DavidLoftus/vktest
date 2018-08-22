#include "stdafx.h"
#include "buffer.h"

#include "renderer.h"

VmaAlloc<vk::Buffer> buffer::createCombinedBuffer(const std::vector<buffer*>& buffers, VmaMemoryUsage memUsage, vk::BufferUsageFlags bufferUsage)
{
	vk::DeviceSize size = 0;
	for (auto buffer : buffers)
	{
		if (buffer->alignment() != 0)
			size = align_offset(size, buffer->alignment());

		buffer->m_offset = size;

		size = buffer->m_offset + buffer->m_size;
		bufferUsage |= buffer->buffer_usage();
	}

	auto alloc = Renderer::createBuffer(size, bufferUsage, memUsage);

	for (auto buffer : buffers)
	{
		buffer->m_handle = alloc.value;
	}

	return alloc;
}

void buffer::updateBuffers(const VmaAlloc<vk::Buffer>& alloc, const std::vector<buffer*>& buffers, const std::vector<void*> bufferData)
{
	VmaAllocationInfo info;
	vmaGetAllocationInfo(vkRenderCtx.allocator, alloc.allocation, &info);


	auto memoryProperties = vkRenderCtx.physicalDevice.getMemoryProperties();

	if (memoryProperties.memoryTypes[info.memoryType].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
	{
		intptr_t data;
		vmaMapMemory(vkRenderCtx.allocator, alloc.allocation, reinterpret_cast<void**>(&data));

		for (size_t i = 0; i < buffers.size(); ++i)
		{
			memcpy(reinterpret_cast<void*>(data + buffers[i]->m_offset), bufferData[i], buffers[i]->m_size);
		}

		vmaUnmapMemory(vkRenderCtx.allocator, alloc.allocation);
	}
	else
	{
		vk::DeviceSize totalSize = 0;
		std::vector<vk::BufferCopy> copyData;

		for (size_t i = 0; i < buffers.size(); ++i)
		{
			copyData.emplace_back(totalSize, buffers[i]->m_offset, buffers[i]->m_size);
			totalSize += buffers[i]->m_size;
		}

		auto stagingBuffer = Renderer::createBufferUnique(totalSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

		intptr_t data;
		vmaMapMemory(vkRenderCtx.allocator, stagingBuffer->allocation, reinterpret_cast<void**>(&data));

		for (size_t i = 0; i < buffers.size(); ++i)
		{
			memcpy(reinterpret_cast<void*>(data+copyData[i].srcOffset), bufferData[i], buffers[i]->m_size);
		}

		vmaUnmapMemory(vkRenderCtx.allocator, stagingBuffer->allocation);

		Renderer::copyBuffer(*stagingBuffer, alloc, copyData);
	}

}


template<typename IndexType>
vk::DeviceSize index_buffer<IndexType>::alignment()
{
	return sizeof(IndexType);
}

template<typename IndexType>
vk::BufferUsageFlags index_buffer<IndexType>::buffer_usage()
{
	return vk::BufferUsageFlagBits::eIndexBuffer;
}

template<typename T>
vk::IndexType index_type();

template<>
vk::IndexType index_type<uint32_t>() { return vk::IndexType::eUint32; };
template<>
vk::IndexType index_type<uint16_t>() { return vk::IndexType::eUint16; };

template<typename IndexType>
void index_buffer<IndexType>::bind(vk::CommandBuffer cmd)
{
	cmd.bindIndexBuffer(m_handle, m_offset, index_type<IndexType>() );
}

template class index_buffer<uint16_t>;
template class index_buffer<uint32_t>;