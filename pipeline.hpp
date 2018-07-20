#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#ifdef MSVC
#	pragma once
#endif

#include <boost/hana.hpp>
#include <boost/core/demangle.hpp>
#include <boost/lambda/construct.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <typeinfo>

#include "shader.hpp"

#define PERFECT_FORWARD(x) (std::forward<decltype(x)>(x))
#define ENUM_CONSTANT(enum_type, val) hana::integral_c<enum_type,enum_type::val>

namespace hana = boost::hana;

constexpr auto hana_keynum = [](auto ec)
{
	using T = std::underlying_type_t<decltype(hana::value(ec))>;
	return hana::integral_c<T,static_cast<T>(hana::value(ec))>;
};

template<typename Tpl>
constexpr auto vertex_binding(Tpl attributes, vk::VertexInputRate rate)
{
	return hana::make_tuple(attributes, rate, static_cast<uint32_t>(hana::sum<hana::integral_constant_tag<size_t>>(hana::transform(attributes, hana::sizeof_))));
}


constexpr auto attribute_formats = hana::make_map(
	hana::make_pair(hana::type_c<float>,	 ENUM_CONSTANT(vk::Format,eR32Sfloat)),
	hana::make_pair(hana::type_c<glm::vec2>, ENUM_CONSTANT(vk::Format,eR32G32Sfloat)),
	hana::make_pair(hana::type_c<glm::vec3>, ENUM_CONSTANT(vk::Format,eR32G32B32Sfloat)),
	hana::make_pair(hana::type_c<glm::vec4>, ENUM_CONSTANT(vk::Format,eR32G32B32A32Sfloat))
);

constexpr auto glsl_names = hana::make_map(
	hana::make_pair(hana_keynum(ENUM_CONSTANT(vk::Format,eR32Sfloat)), BOOST_HANA_STRING("float")),
	hana::make_pair(hana_keynum(ENUM_CONSTANT(vk::Format,eR32G32Sfloat)), BOOST_HANA_STRING("vec2")),
	hana::make_pair(hana_keynum(ENUM_CONSTANT(vk::Format,eR32G32B32Sfloat)), BOOST_HANA_STRING("vec3")),
	hana::make_pair(hana_keynum(ENUM_CONSTANT(vk::Format,eR32G32B32A32Sfloat)), BOOST_HANA_STRING("vec4"))
);

#define PASS_NAME

template<typename T, typename Binding>
constexpr auto get_attributes(T type, Binding binding)
{
	static_assert(hana::Struct<T::type>::value, "type must have concept Stuct");

	constexpr auto accessors = hana::accessors<T::type>();
#ifdef PASS_NAME
	constexpr auto names = hana::transform(accessors, hana::first);
#endif

	constexpr auto attributes = hana::transform(accessors,
		[](auto p)
		{
			return hana::type_c<std::remove_reference_t<std::invoke_result_t<decltype(hana::second(p)), T::type>>>;
		}
	);

	constexpr auto offsets = hana::fold(
		hana::drop_back(attributes),
		hana::make_tuple(hana::size_c<0>),
		[](auto state, auto attribute)
		{
			return hana::append(state, hana::back(state) + hana::sizeof_(attribute));
		}
	);
	constexpr auto bindings = hana::fill(attributes, binding);

	return hana::zip(
		bindings,
		hana::transform(attributes,hana::partial(hana::at_key, attribute_formats)),
		offsets
#ifdef PASS_NAME
		, names
#endif
	);
}

template<typename T>
constexpr auto get_size(T type)
{
	if constexpr(hana::is_a<hana::tuple_tag, T>)
		return hana::sum<hana::size_t<0>>(hana::transform(type, get_size));
	else if constexpr(hana::Struct<T>::value)
		return hana::sum<hana::size_t<0>>(hana::transform(type, [](auto p) {return hana::sizeof_(hana::second(p)); }));
	else if constexpr(hana::is_a<hana::type_tag, T>)
		return hana::sizeof_(type);
}

template<typename Tpl>
constexpr auto create_vertex_input_layout(Tpl vertexStructs)
{
	static_assert(hana::is_a<hana::tuple_tag, Tpl>, "Argument vertexStructs must be a tuple.");

	constexpr auto indices = hana::unpack(hana::make_range(hana::size_c<0>, hana::size(vertexStructs)), hana::make_tuple);


	
	auto data = hana::transform(
		hana::zip(vertexStructs, indices),
		[](auto tpl)
		{
			auto pair = tpl[hana::int_c<0>];
			static_assert(hana::is_a<hana::pair_tag, decltype(pair)>, "All elements of vertexStructs must be hana::pair<Type, hana::bool_c>");
			
			auto idx = hana::to<hana::integral_constant_tag<uint32_t>>(tpl[hana::int_c<1>]);

			auto attributes = get_attributes(hana::first(pair), idx);

			return hana::make_pair(hana::make_tuple(
				idx,
				hana::to<hana::integral_constant_tag<uint32_t>>(get_size(hana::first(pair))),
				hana::if_(
					hana::second(pair),
					ENUM_CONSTANT(vk::VertexInputRate,eInstance),
					ENUM_CONSTANT(vk::VertexInputRate,eVertex)
				)),
				attributes
			);
		}
	);

	auto bindings = hana::transform(data, hana::first);
	auto _attributes = hana::flatten(hana::transform(data, hana::second));
	auto attributes_indices = hana::unpack(hana::make_range(hana::size_c<0>, hana::size(_attributes)), hana::make_tuple);
	auto attributes = hana::transform(hana::zip(_attributes, attributes_indices), hana::fuse(hana::prepend)); // unpack(tpl,prepend)

	return hana::make_pair(bindings,attributes);

	/*using attribute_type = hana::tuple<
		hana::integral_constant<uint32_t, ...>, // relative location
		hana::integral_constant<vk::Format, ...>, // format
		hana::integral_constant<uint32_t, ...>, // offset
	>;

	using binding_type = hana::tuple<
		hana::integral_constant<uint32_t,...>, // binding id
		hana::integral_constant<uint32_t,...>, // stride
		hana::integral_constant<vk::VertexInputRate,...>, // VertexInputRate
		hana::tuple<attribute_type> // ... list of attributes
	>;

	using return_type = hana::tuple<binding_type>; // ... list of bindings*/

}


auto dump_layout = [](auto layout)
{
	hana::for_each(hana::second(layout), [](auto attribute)
	{
		std::cout
			<< "layout(location = " << hana::value(attribute[hana::int_c<0>]) << ") "
			<< hana::to<const char*>(glsl_names[hana_keynum(attribute[hana::int_c<2>])]) << ' '
#ifdef PASS_NAME
			<< hana::to<const char*>(attribute[hana::int_c<4>])
#else
			<< "someVar"
#endif
			<< "; // offset = " << hana::value(attribute[hana::int_c<3>])
			<< ", binding = " << hana::value(attribute[hana::int_c<1>])
			<< std::endl;
	});
};

template<typename T>
constexpr auto to_array = [](auto&& ...x)
{
	return std::array<T, sizeof...(x)>{ {std::forward<decltype(x)>(x)...} };
};

template<typename PipelineInfo>
struct graphics_pipeline
{
	PipelineInfo info;
	vk::UniquePipeline m_pipeline;

	void create(vk::Device device, vk::RenderPass renderPass, std::vector<std::string> shaders)
	{

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
		std::vector<vk::UniqueShaderModule> shaderModules;

		shaderStages.reserve(shaders.size());
		shaderModules.reserve(shaders.size());
		for (auto& file : shaders)
		{
			shaderModules.push_back(create_shader(device, file));
			shaderStages.emplace_back(vk::PipelineShaderStageCreateFlags{}, get_shader_stage(file), shaderModules.back().get(), "main");
		}


		auto vertexLayout = info[hana::int_c<0>];

		auto bindingInfo = hana::unpack(
			hana::transform(
				hana::first(vertexLayout),
				hana::fuse(hana::on([](uint32_t a, uint32_t b, vk::VertexInputRate c) {return vk::VertexInputBindingDescription{ a,b,c }; }, hana::value_of))
			),
			to_array<vk::VertexInputBindingDescription>
		);

		auto attributeInfo = hana::unpack(
			hana::transform(
				hana::second(vertexLayout),
#ifdef PASS_NAME
				hana::compose(
#endif
					hana::fuse(hana::on([](uint32_t a, uint32_t b, vk::Format c, uint32_t d) {return vk::VertexInputAttributeDescription{ a,b,c,d }; }, hana::value_of))
#ifdef PASS_NAME
					, hana::drop_back
				)
#endif
			),
			to_array<vk::VertexInputAttributeDescription>
		);

		vk::PipelineVertexInputStateCreateInfo vertexInputState{
			{},
			static_cast<uint32_t>(bindingInfo.size()), bindingInfo.data(),
			static_cast<uint32_t>(attributeInfo.size()), attributeInfo.data()
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{ {}, hana::value(info[hana::int_c<1>]) };

		//vk::PipelineTessellationStateCreateInfo tessellationState{};

		vk::Viewport viewport{ 0.0f, 0.0f, (float)swapchainExtent.width, (float)swapchainExtent.height, 0.0f, 1.0f };
		vk::Rect2D scissor{ {}, swapchainExtent };

		vk::PipelineViewportStateCreateInfo viewportState{ {}, 1, &viewport, 1, &scissor };

		vk::PipelineRasterizationStateCreateInfo rasterizationState{ {}, false, false, info[hana::int_c<2>], vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f };

		vk::PipelineMultisampleStateCreateInfo multisampleState{};

		vk::PipelineDepthStencilStateCreateInfo depthStencilState{};

		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments{
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

		auto pipelineLayout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{});

		vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
			{},
			static_cast<uint32_t>(shaderStages.size()), shaderStages.data(),
			&vertexInputState,
			&inputAssemblyState,
			nullptr,
			&viewportState,
			&rasterizationState,
			&multisampleState,
			&depthStencilState,
			&colorBlendState,
			nullptr,
			pipelineLayout.get(),
			renderPass
		};

		m_pipeline = device.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo);

	}

};

constexpr auto make_graphics_pipeline = [](auto vertexLayout, auto topology, auto polygonMode) constexpr
{
	static_assert(hana::is_a<hana::pair_tag>(vertexLayout), "Argument #1 (vertexLayout) must be a pair.");
	static_assert(hana::is_a<hana::integral_constant_tag<vk::PrimitiveTopology>>(topology), "Argument #2 (topology) must be an integral_constant<vk::PrimitiveTopology>.");
	static_assert(hana::is_a<hana::integral_constant_tag<vk::PolygonMode>>(polygonMode), "Argument #3 (polygonMode) must be an integral_constant<vk::PolgygonMode>.");

	return graphics_pipeline<decltype(hana::make_tuple(vertexLayout, topology, polygonMode))>();
};

#endif