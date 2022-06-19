#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fnormal;
layout(location = 2) in vec3 fposition;
layout(location = 3) in vec2 fuv;
layout(location = 4) in vec2 vpos;
layout(location = 5) in vec3 mpos;
layout(location = 6) in vec3 sunDir;
layout(location = 7) in vec3 ftangent;
layout(location = 8) in vec3 fbiotangent;
layout(location = 9) in float normalScale;

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

layout(binding = 1) uniform sampler2D albedo;

void main() {
    vec2 uv = fuv;
    vec4 mainTex = texture(albedo, uv) * uni0.baseColor;
    if (0.5 > mainTex.a) { discard; }
}