#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
	float gl_PointSize;
};

layout(location = 0) in vec2 vpos;
layout(location = 1) in vec2 pos;
layout(location = 2) in vec2 scale;
layout(location = 3) in vec3 col;

layout(location = 0) out vec3 fragColor;


vec2 scales[4] = vec2[](
    vec2(0.05, 0.1),
	vec2(0.2, 0.01),
    vec2(0.05, 0.5),
	vec2( 1, 1)
);

void main() {
    gl_Position = vec4(pos + vpos * scale,0,1);
    fragColor = col;
}