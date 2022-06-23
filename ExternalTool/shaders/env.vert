#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

layout(location = 0) out vec2 fuv;
layout(location = 1) out vec3 normal;

layout(location = 0) in vec2 uv;

void main() {
	gl_Position = vec4(uv, 0.0, 1.0);
	fuv = uv * 0.5 + 0.5;
	normal = normalize(vec3(uv.x / frame.p[0][0], uv.y / frame.p[1][1], -1.0));
	normal = (vec4(normal, 1.0) * frame.v).xyz;
}