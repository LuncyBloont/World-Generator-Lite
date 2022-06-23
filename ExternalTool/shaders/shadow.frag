#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

layout(location = 3) in vec2 fuv;
layout(location = 4) in vec3 vpos;

void main() {
    vec2 uv = fuv;
    vec4 mainTex = texture(albedo, uv) * object.baseColor;
    if (object.clip > mainTex.a) { discard; }
    if (object.clip == 0.0 && hash2(vpos.xy / vpos.z) > mainTex.a) { discard; }
}