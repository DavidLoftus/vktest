#include "globals.h"

vkRenderCtx_t vkRenderCtx;


size_t align_offset(size_t offset, size_t alignment)
{
	if (alignment == 0) return offset;
	size_t r = offset & (alignment - 1);
	return (r == 0) ? offset : offset - r + alignment;
}
