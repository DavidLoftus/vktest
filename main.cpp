#include "stdafx.h"

#include <math.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include <boost/hana/define_struct.hpp>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <termcolor.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "shader.hpp"
#include "renderer.hpp"
#include "scene.hpp"

std::ostream& operator<<(std::ostream& os, vk::DebugReportFlagsEXT flags)
{
	bool multiple = false;

	if (flags & vk::DebugReportFlagBitsEXT::eDebug)
	{
		if (multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::dark << termcolor::bold << "DEBUG";
	}
	if (flags & vk::DebugReportFlagBitsEXT::eError)
	{
		if (multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::red << termcolor::bold << "ERROR";
	}
	if (flags & vk::DebugReportFlagBitsEXT::eInformation)
	{
		if (multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::dark << "INFO";
	}
	if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
	{
		if (multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::yellow << "PERFWARN";
	}
	if (flags & vk::DebugReportFlagBitsEXT::eWarning)
	{
		if (multiple)
			os << termcolor::reset << ' ';
		multiple = true;
		os << termcolor::red << "WARNING";
	}

	if (!multiple)
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

auto createComputePipeline(vk::Device device, vk::PipelineLayout pipelineLayout)
{

	auto shader = Shader("shaders/shader.comp.spv");
	return device.createComputePipelineUnique({},
		vk::ComputePipelineCreateInfo{
			{},
			shader.getStageInfo(),
			pipelineLayout
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


/*auto idx_tpl = hana::to<hana::tuple_tag>(hana::range_c<INDEX_TYPE, 0, NUMPOINTS>);
auto vtx_tpl = hana::append(hana::transform(idx_tpl, [](auto i)
{
	constexpr double angle = hana::value(i) * (2 * M_PI / NUMPOINTS);
	return vertex_data{ glm::vec2(std::cos(angle), std::sin(angle)) };
}), vertex_data{ glm::vec2() });

auto vertices = hana::unpack(vtx_tpl, to_array<vertex_data>);
auto indices = hana::unpack(hana::flatten(hana::transform(idx_tpl, [](auto i) {
	return hana::make_tuple(static_cast<INDEX_TYPE>(NUMPOINTS), hana::value(i), (hana::value(i) + 1) % NUMPOINTS);
})), to_array<INDEX_TYPE>);*/

std::vector<vertex_data> vertices;
std::vector<INDEX_TYPE> indices;

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
	indicesOffset = pad_up(instancesOffset + instancesSize, sizeof(INDEX_TYPE));


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
	auto commandPool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo{ {}, 0 });
	auto commandBuffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{ commandPool.get(), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(size) });

	for (size_t i = 0; i < commandBuffers.size(); ++i)
	{
		auto cb = commandBuffers[i];

		cb.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eSimultaneousUse });
		func(cb, i);
		cb.end();
	}

	return std::make_pair(std::move(commandPool), std::move(commandBuffers));
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

	auto imageAvailableSemaphore = vkRenderCtx.device.createSemaphoreUnique({}), imageReadySemaphore = vkRenderCtx.device.createSemaphoreUnique({});
	auto renderFinishedFence = vkRenderCtx.device.createFenceUnique({});

	glfwSetMouseButtonCallback(nullptr, mousebuttonfun);

	render = false;

	while (!glfwWindowShouldClose(nullptr))
	{

		auto result = vkRenderCtx.device.acquireNextImageKHR(nullptr, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore.get(), nullptr);
		uint32_t idx = result.value;

		//std::cout << idx << ':' << commandBuffers.size() << std::endl;

		vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vkRenderCtx.queue.submit({
			vk::SubmitInfo{
				1, &imageAvailableSemaphore.get(), &waitStage,
				1, nullptr,//&commandBuffers[idx],
				1, &imageReadySemaphore.get()
			}
			}, renderFinishedFence.get());

		vkRenderCtx.queue.presentKHR(
			vk::PresentInfoKHR{
				1, &imageReadySemaphore.get(),
				1, nullptr,
				&idx
			}
		);

		vkRenderCtx.device.waitForFences({ renderFinishedFence.get() }, true, std::numeric_limits<uint64_t>::max());
		vkRenderCtx.device.resetFences({ renderFinishedFence.get() });


		glfwPollEvents();

	}

	return 0;
}

int main()
{
	Renderer renderer;

	try {
		renderer.init();

		Scene myscene = Scene::Load("myscene.txt");
		renderer.loadScene(myscene);

		renderer.loop();

	}
	catch (std::runtime_error e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}