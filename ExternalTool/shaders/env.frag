#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

layout(location = 0) out vec4 col;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;

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
    float z = uni0.p[3][2] / uni0.p[2][2];
    float fogP = clamp(log(1000.0 * uni0.fog.x), 0.0, 1.0) * uni0.fog.y;
	col = vec4(cubeSky(envTex, normal, fogP, uni0.fogColor), 1.0);
}