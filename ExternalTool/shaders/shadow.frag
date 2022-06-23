#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

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

void main() {
    vec2 uv = fuv;
    vec4 mainTex = texture(albedo, uv) * object.baseColor;
    if (0.5 > mainTex.a) { discard; }
}