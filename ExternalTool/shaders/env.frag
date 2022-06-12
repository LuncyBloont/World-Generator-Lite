#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

layout(location = 0) out vec4 col;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;

layout(binding = 0) uniform Uni {
    float time;
    float skyForce;
    vec2 fog;
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
    mat4 nm;
} uni0;

layout(binding = 3) uniform samplerCube envTex;

void main() {
	col = vec4(cubeSky(envTex, normal), 1.0);
}