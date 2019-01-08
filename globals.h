#ifdef _MSC_VER
#	pragma once
#endif
#ifndef GLOBALS_H
#define GLOBALS_H

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

struct vkRenderCtx_t
{
	vk::Instance instance;

	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physicalDeviceProperties;

	vk::Device device;

	VmaAllocator allocator;

	uint32_t queueFamily;
	vk::Queue queue;

	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;

	vk::RenderPass renderPass;

	vk::CommandPool commandPool;
};

extern vkRenderCtx_t vkRenderCtx;



template<typename T>
struct VmaAlloc
{
	VmaAllocation allocation;
	T value;


	VmaAlloc& operator=(std::nullptr_t)
	{
		allocation = nullptr;
		value = nullptr;
		return *this;
	}

	operator bool() const
	{
		return value;
	}

	bool operator==(const VmaAlloc& other) const
	{
		return value == other.value;
	}

	bool operator!=(const VmaAlloc& other) const
	{
		return value != other.value;
	}

};

template<typename T>
using UniqueVmaAlloc = vk::UniqueHandle<VmaAlloc<T>,vk::DispatchLoaderStatic>;
using UniqueVmaAllocator = vk::UniqueHandle<VmaAllocator,vk::DispatchLoaderStatic>;


template<typename T, typename Dispatch = vk::DispatchLoaderStatic>
class UniqueVector :
	public vk::UniqueHandleTraits<T,Dispatch>::deleter
{
	using Vector = std::vector<T>;
	using Deleter = typename vk::UniqueHandleTraits<T,Dispatch>::deleter;

	using size_type = typename Vector::size_type;

private:
	Vector m_value;

public:
	UniqueVector(Vector data = Vector(), const Deleter& deleter = Deleter()) :
		Deleter(deleter),
		m_value(std::move(data))
	{}

	UniqueVector(const UniqueVector&) = delete;

	UniqueVector(UniqueVector&& other) :
		Deleter(std::move(static_cast<Deleter&>(other))),
		m_value(other.release())
	{}

	~UniqueVector()
	{
		std::for_each(m_value.begin(), m_value.end(), [this](auto x) { Deleter::destroy(x); });
	}

	UniqueVector& operator=(const UniqueVector&) = delete;

	UniqueVector& operator=(UniqueVector&& other)
	{
		reset(other.release());
		*static_cast<Deleter*>(this) = std::move(static_cast<Deleter&>(other));
		return *this;
	}

	T& operator[](size_type i)
	{
		return m_value[i];
	}

	const T& operator[](size_type i) const
	{
		return m_value[i];
	}

	explicit operator bool() const
	{
		return m_value.operator bool();
	}

	const Vector* operator->() const
	{
		return &m_value;
	}

	Vector* operator->()
	{
		return &m_value;
	}

	const Vector& operator*() const
	{
		return m_value;
	}

	Vector& operator*()
	{
		return m_value;
	}

	const Vector& get() const
	{
		return m_value;
	}

	Vector& get()
	{
		return m_value;
	}

	void reset(Vector value = Vector())
	{
		std::for_each(m_value.begin(), m_value.end(), [this](auto x) { Deleter::destroy(x); });
		m_value = std::move(value);
	}

	Vector release()
	{
		return std::exchange(m_value, Vector());
	}

	void swap(UniqueVector& rhs)
	{
		std::swap(m_value, rhs.m_value);
		std::swap(static_cast<Deleter&>(*this), static_cast<Deleter&>(rhs));
	}

};

namespace std
{
	template<typename T>
	void swap(UniqueVector<T>& lhs, UniqueVector<T>& rhs)
	{
		lhs.swap(rhs);
	}
}

namespace vk
{
	template<typename T>
	class VmaDestroy {};

	template<>
	class VmaDestroy<NoParent>
	{
	public:
		VmaDestroy(Optional<const AllocationCallbacks> = nullptr) {}

	protected:
		void destroy(VmaAllocator allocator)
		{
			vmaDestroyAllocator(allocator);
		}
	};

	template<>
	class VmaDestroy<VmaAllocator>
	{
	public:
		VmaDestroy(VmaAllocator allocator = nullptr, Optional<const AllocationCallbacks> = nullptr) : m_allocator(allocator) {}

	protected:
		void destroy(const VmaAlloc<vk::Buffer>& alloc)
		{
			vmaDestroyBuffer(m_allocator, static_cast<VkBuffer>(alloc.value), alloc.allocation);
		}
		void destroy(const VmaAlloc<vk::Image>& alloc)
		{
			vmaDestroyImage(m_allocator, static_cast<VkImage>(alloc.value), alloc.allocation);
		}

	private:
		VmaAllocator m_allocator;
	};


	template<typename Dispatch>
	struct UniqueHandleTraits<VmaAllocator,Dispatch>
	{
		using deleter = VmaDestroy<NoParent>;
	};

	template<typename T,typename Dispatch>
	struct UniqueHandleTraits<VmaAlloc<T>,Dispatch>
	{
		using deleter = VmaDestroy<VmaAllocator>;
	};
}

size_t align_offset(size_t offset, size_t alignment);

#endif
