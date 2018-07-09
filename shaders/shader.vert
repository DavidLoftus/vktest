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

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    gl_Position = vec4(pos.xy,0,1);//vec4(positions[gl_VertexIndex%4] , 0, 1.0);
	float distSquared = dot(pos,pos);
    fragColor = hsv2rgb(vec3(pow(0.0625,distSquared),1,0.5));
}