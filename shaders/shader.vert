#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
	float gl_PointSize;
};

layout(location = 0) out vec3 fragColor;

layout(location = 0) in vec3 pos;

float sigmoid(float x)
{
    return 1/(1+exp(-x));
}

vec2 positions[4] = vec2[](
    vec2(-0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, -0.5)
);

vec3 colors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0)
);


void main() {
    gl_Position = vec4(pos.xy,0,1);//vec4(positions[gl_VertexIndex%4] , 0, 1.0);
    fragColor = sigmoid(pos.z) * vec3(1,1,0);
	gl_PointSize = 3;
}