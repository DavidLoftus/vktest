#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
	float gl_PointSize;
};

layout(set = 0, binding = 0) uniform RenderData
{
	mat4 projection;
	mat4 view;
} render;

layout(set = 1, binding = 0) uniform ObjectRenderData
{
	mat4 model;
} object;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 col;

void main() {
    gl_Position = render.projection * render.view * object.model * vec4(pos,1);
    col = color;
}