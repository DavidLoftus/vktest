#ifdef _MSC_VER
#	pragma once
#endif
#ifndef STDAFX_H
#define STDAFX_H

#include <stdlib.h>
#include <math.h>


#ifdef WIN32
#	include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <chrono>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <vk_mem_alloc.h>
#include <termcolor.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#endif
