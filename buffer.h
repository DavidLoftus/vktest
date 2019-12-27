#ifdef _MSC_VER
#	pragma once
#endif
#ifndef BUFFER_H
#define BUFFER_H


#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "globals.h"


class buffer
{
public:
	buffer() = default;
	buffer(vk::DeviceSize size) : m_size(size) {}

	virtual ~buffer() = default;

	virtual vk::DeviceSize alignment() = 0;
	virtual vk::BufferUsageFlags buffer_usage() = 0;

	static VmaAlloc<vk::Buffer> createCombinedBuffer(const std::vector<buffer*>& buffers, VmaMemoryUsage memUsage, vk::BufferUsageFlags bufferUsage = {});
	static UniqueVmaAlloc<vk::Buffer> createCombinedBufferUnique(const std::vector<buffer*>& buffers, VmaMemoryUsage memUsage, vk::BufferUsageFlags bufferUsage = {})
	{
		return UniqueVmaAlloc<vk::Buffer>(createCombinedBuffer(buffers, memUsage, bufferUsage), vkRenderCtx.allocator);
	}
	
	static void updateBuffers(const VmaAlloc<vk::Buffer>& alloc, const std::vector<buffer*>& buffers, const std::vector<void*> data);


protected:
	vk::Buffer m_handle;
	vk::DeviceSize m_offset;
	vk::DeviceSize m_size;
};


template<typename Vertex>
class vertex_buffer : public buffer
{
public:
	vertex_buffer() = default;
	explicit vertex_buffer(size_t nVertices) : buffer(nVertices * sizeof(Vertex)) {}

	virtual vk::DeviceSize alignment()
	{
		return 0;
	}

	virtual vk::BufferUsageFlags buffer_usage()
	{
		return vk::BufferUsageFlagBits::eVertexBuffer;
	}

	void bind(vk::CommandBuffer cmd, uint32_t binding)
	{
		cmd.bindVertexBuffers(binding, { m_handle }, { m_offset });
	}
};

template<typename IndexType = uint16_t>
class index_buffer : public buffer
{
public:
	index_buffer() = default;
	explicit index_buffer(size_t nIndices) : buffer(nIndices * sizeof(IndexType)) {}

	virtual vk::DeviceSize alignment();
	virtual vk::BufferUsageFlags buffer_usage();

	void bind(vk::CommandBuffer cmd);
};

template<typename UniformData>
class uniform_buffer : public buffer
{
	vk::DeviceSize padding_size()
	{
		return align_offset(sizeof(UniformData), alignment());
	}

public:
	uniform_buffer() = default;
	explicit uniform_buffer(size_t nBuffers)
	{
		m_size = nBuffers * padding_size();
	}

	vk::DeviceSize alignment()
	{
		return vkRenderCtx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	}
	vk::BufferUsageFlags buffer_usage()
	{
		return vk::BufferUsageFlagBits::eUniformBuffer;
	}

	vk::DescriptorBufferInfo bufferInfo(size_t idx = 0)
	{
		return vk::DescriptorBufferInfo{ m_handle, m_offset + idx * padding_size(), sizeof(UniformData) };
	}

};

#endif