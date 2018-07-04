#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

layout(binding = 0, location = 0) in vec3 pos;

float sigmoid(float x)
{
    return 1/(1+exp(-x))
}

void main() {
    gl_Position = vec4(pos, 1.0);
    fragColor = sigmoid(pos.z) * vec3(1,1,0);
}