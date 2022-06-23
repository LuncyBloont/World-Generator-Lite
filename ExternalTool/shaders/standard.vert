#version 450
#extension GL_GOOGLE_include_directive: enable
#include "./lib.glsl"

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

void main() {

    fuv = vec2(uv.x, 1.0 - uv.y);
    float dp = pow(1.0 - texture(dynamicMap, fuv).a, 0.5);

	vec4 ipos = offRot * vec4((position + dp * worldWind(frame.time, normal, position)) * 
                scale.xyz * object.scale.xyz, 1.0);

	vec4 inor = offRot * vec4(normalize(normal / scale.xyz / object.scale.xyz), 0.0);

	vec4 itan = offRot * vec4(normalize(tangent * scale.xyz * object.scale.xyz), 0.0);

	vec4 ibtan = offRot * vec4(normalize(biotangent * scale.xyz * object.scale.xyz), 0.0);

    gl_Position = object.mvp * ipos;

    fragColor = vec3(1.0, 0.0, 1.0);

    mat4 mv = frame.v * object.m;

    fnormal = (mv * inor).xyz;
    fposition = (mv * ipos).xyz;
    ftangent = (mv * itan).xyz;
    fbiotangent = (mv * ibtan).xyz;
    normalScale = scale.w * object.scale.w;
    
    vpos = gl_Position.xy / gl_Position.w;
    mpos = position;
    sunDir = (frame.v * vec4(frame.sunDir, 0.0)).xyz;

    shadowPos = shadow.shadowV * object.m * ipos;
}