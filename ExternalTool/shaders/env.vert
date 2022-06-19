#version 450

layout(location = 0) out vec2 fuv;
layout(location = 1) out vec3 normal;

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform Uni {
    float time;
    float skyForce;
    float subsurface;
    float ssbase;
    vec2 fog;
    vec3 fogColor;
    vec4 resolution;
    vec4 sun;
    mat4 mvp;
    vec4 bprRSMC;
    vec3 sunDir;
    vec4 baseColor;
    vec4 etc;
    mat4 m;
    mat4 v;
    mat4 p;
    vec4 sc;
    vec4 pbrBase;
    mat4 shadowV;
    vec2 shadowBias;
} uni0;

layout(binding = 3) uniform samplerCube envTex;

void main() {
	gl_Position = vec4(uv, 0.0, 1.0);
	fuv = uv * 0.5 + 0.5;
	normal = normalize(vec3(uv.x / uni0.p[0][0], uv.y / uni0.p[1][1], -1.0));
	normal = (vec4(normal, 1.0) * uni0.v).xyz;
}