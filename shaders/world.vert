#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
	float gl_PointSize;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 col;

void main() {
    gl_Position = vec4(pos,1);
    col = color;
}