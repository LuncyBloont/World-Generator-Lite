#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

layout(location = 0) out vec4 col;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;

void main() {
    float z = frame.p[3][2] / frame.p[2][2];
    float fogP = clamp(log(1000.0 * frame.fog.x), 0.0, 1.0) * frame.fog.y;
	col = vec4(cubeSky(envTex, normal, fogP, frame.fogColor), 1.0);
}