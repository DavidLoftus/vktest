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

#include <boost/hana.hpp>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>
#include <termcolor.hpp>
#include <stb_image.h>

#endif