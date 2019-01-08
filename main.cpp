#include "stdafx.h"

#include <math.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <termcolor.hpp>

#include "renderer.h"
#include "scene.h"

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

int main()
{
	Renderer renderer;

	try {
		renderer.init();

		Scene myscene = Scene::Load("../myscene.txt");
		renderer.loadScene(myscene);

		renderer.loop();

	}
	catch (std::runtime_error e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
