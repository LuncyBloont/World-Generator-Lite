#version 450

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fnormal;
layout(location = 2) out vec3 fposition;
layout(location = 3) out vec2 fuv;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(binding = 0) uniform Uni {
    float time;
    vec4 resolution;
} uni0;

void main() {
	float n = 0.1;
	float f = 1000.0;

	float r0 = uni0.time * 0.4, r1 = uni0.time * 0.7;
	mat3 rot = mat3(
			cos(r0), -sin(r0), 0.0,
			sin(r0), cos(r0), 0.0,
			0.0, 0.0, 1.0
		) * mat3(
			1.0, 0.0, 0.0,
			0.0, cos(r1), -sin(r1),
			0.0, sin(r1), cos(r1)
		);

	vec3 pos = rot * position;
	float z = -pos.z + 2.0;
	vec2 pxy = pos.xy * vec2(uni0.resolution.y / uni0.resolution.x, 1.0);
    gl_Position = vec4(pxy.x, -pxy.y, z * z / 100.0, z);
    fragColor = vec3(0.7, 0.5, 0.2);
    fnormal = rot * normal;
    fposition = vec3(pxy.x, -pxy.y, z);
    fuv = uv;
}