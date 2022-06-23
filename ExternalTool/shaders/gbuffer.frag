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
layout(location = 10) in vec4 shadowPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 position = fposition;
    vec3 rnormal = normalize(fnormal);
    vec3 tangent = normalize(ftangent);
    vec3 biotangent = normalize(fbiotangent);

    vec2 uv = fuv;

    vec4 norData = texture(normalTex, uv);
    norData = vec4(norData.xyz * 2.0 - 1.0, norData.w);
    vec3 normal = normalize(norData.x * tangent * normalScale + 
        norData.y * biotangent * normalScale + 
        norData.z * rnormal);
    float ss = clamp(norData.w * object.subsurface + object.ssbase, 0.0, 1.0);

    vec4 mainTex = texture(albedo, uv) * object.baseColor;

    vec3 surface = mainTex.rgb;

    vec4 pbrData = texture(pbrTex, uv);
    float roughness = clamp(object.pbrRSMC.x * pbrData.r + object.pbrBase.r, 0.0, 1.0);
    float specular = clamp(object.pbrRSMC.y * 2.0 * pbrData.g + object.pbrBase.g, 0.0, 2.0);
    float metallic = clamp(object.pbrRSMC.z * pbrData.b + object.pbrBase.b, 0.0, 1.0);
    float contrast = clamp(object.pbrRSMC.w * 2.0 * pbrData.a + object.pbrBase.a, 0.0, 2.0);

    if (0.5 > mainTex.a) { discard; }

    surface = pow(surface, vec3(contrast));

    int type = int(frame.etc.x);
    if (type == 0) {
        outColor = vec4(surface, 1.0);
    } else if (type == 1) {
        outColor = vec4(normal, ss);
    } else if (type == 2) {
        outColor = vec4(roughness, specular / 2.0, metallic, 1.0);
    } else if (type == 3) {
        outColor = vec4(position.z);
    } else if (type == 4) {
        outColor = vec4(uv, vpos);
    } else if (type == 5) {
        
    }   
}