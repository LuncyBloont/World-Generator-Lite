#version 450

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fnormal;
layout(location = 2) out vec3 fposition;
layout(location = 3) out vec2 fuv;
layout(location = 4) out vec2 vpos;
layout(location = 5) out vec3 mpos;
layout(location = 6) out vec3 sunDir;
layout(location = 7) out vec3 ftangent;
layout(location = 8) out vec3 fbiotangent;
layout(location = 9) out float normalScale;
layout(location = 10) out vec4 shadowPos;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 biotangent;
layout(location = 4) in vec2 uv;
layout(location = 5) in vec4 data0;
layout(location = 6) in vec4 data1;

layout(location = 7) in mat4 offRot;
layout(location = 11) in vec4 scale;

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

void main() {

	vec4 ipos = offRot * vec4(position * scale.xyz * uni0.sc.xyz, 1.0);

	vec4 inor = offRot * vec4(normalize(normal / scale.xyz / uni0.sc.xyz), 0.0);

	vec4 itan = offRot * vec4(normalize(tangent * scale.xyz * uni0.sc.xyz), 0.0);

	vec4 ibtan = offRot * vec4(normalize(biotangent * scale.xyz * uni0.sc.xyz), 0.0);

    gl_Position = uni0.mvp * ipos;

    fragColor = vec3(1.0, 0.0, 1.0);

    mat4 mv = uni0.v * uni0.m;

    fnormal = (mv * inor).xyz;
    fposition = (mv * ipos).xyz;
    ftangent = (mv * itan).xyz;
    fbiotangent = (mv * ibtan).xyz;
    normalScale = scale.w * uni0.sc.w;
    fuv = vec2(uv.x, 1.0 - uv.y);
    
    vpos = gl_Position.xy / gl_Position.w;
    mpos = position;
    sunDir = (uni0.v * vec4(uni0.sunDir, 0.0)).xyz;

    shadowPos = uni0.shadowV * uni0.m * ipos;
}