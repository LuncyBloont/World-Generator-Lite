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
layout(binding = 2) uniform sampler2D pbrTex;
layout(binding = 3) uniform samplerCube envTex;
layout(binding = 4) uniform sampler2D normalTex;
layout(binding = 5) uniform sampler2D shadowMap0;

void main() {
    vec3 position = fposition;
    vec3 rnormal = normalize(fnormal);// * sign(dot(fnormal, -position)); 
    vec3 tangent = normalize(ftangent);
    vec3 biotangent = normalize(fbiotangent);

    vec3 sdPos = shadowPos.xyz / shadowPos.w;
    sdPos.xy = sdPos.xy * 0.5 + 0.5;
    float shadowRaw = texture(shadowMap0, sdPos.xy).r;
    float sdfact = 1.0f;
    if (shadowRaw <= sdPos.z) {
        sdfact = mix(1.0, 0.0, clamp((sdPos.z - shadowRaw) / uni0.shadowBias.x, 0.0, 1.0));
    }

    vec2 uv = fuv;

    vec4 norData = texture(normalTex, uv);
    norData = vec4(norData.xyz * 2.0 - 1.0, norData.w);
    vec3 normal = normalize(norData.x * tangent * normalScale + 
        norData.y * biotangent * normalScale + 
        norData.z * rnormal);
    float ss = clamp(norData.w * uni0.subsurface + uni0.ssbase, 0.0, 1.0);

    vec4 mainTex = texture(albedo, uv) * uni0.baseColor;

    vec3 surface = mainTex.rgb;

    vec4 pbrData = texture(pbrTex, uv);
    float roughness = clamp(uni0.bprRSMC.x * pbrData.r + uni0.pbrBase.r, 0.0, 1.0);
    float specular = clamp(uni0.bprRSMC.y * 2.0 * pbrData.g + uni0.pbrBase.g, 0.0, 2.0);
    float metallic = clamp(uni0.bprRSMC.z * pbrData.b + uni0.pbrBase.b, 0.0, 1.0);
    float contrast = clamp(uni0.bprRSMC.w * 2.0 * pbrData.a + uni0.pbrBase.a, 0.0, 2.0);
    float fnlBase = 0.07;
    float fnlScale = 0.9;

    if (0.5 > mainTex.a) { discard; }

    surface = pow(surface, vec3(contrast));
    
    vec3 ref = reflect(normalize(position), normal);
    float fnl = fnlBase * fnlScale + 
        (1.0 - fnlBase) * fnlScale * pow(1.0 - abs(dot(normal, -normalize(position))), 6.0);
    float fnlRough = mix(roughness, roughness * 0.5, fnl);
    float roughDark = 1.0 - roughness * 0.5;
    float mirr = mix(1.0, fnl, 1.0 - metallic);
    vec3 insubface = mix(vec3(1.0), surface, metallic);
    float sl = dot(normalize(-sunDir - normalize(position)), normal);// dot(-sunDir, ref);
    vec3 spe = sdfact * roughDark * mirr * uni0.sun.xyz * uni0.sun.w * insubface *
        pow(max(0.0, sl), pow(1.0 / fnlRough, 2.0)) / 
        pow(fnlRough + 0.1, 2.0) * mix(specular, 1.0, metallic) / 2.0;
    float bl = dot(-sunDir, normal) * ss + (1 - ss);
    vec3 diff = sdfact * mix(ss * 0.5 + 0.5, 1.0, ss) * max(0.0, bl) * surface * 
        uni0.sun.xyz * uni0.sun.w * (1.0 - metallic);
    vec3 miref = roughDark * mirr * 
        cubeLod(envTex, (vec4(ref, 0.0) * uni0.v).xyz, fnlRough * maxDiffLevel) * 
        insubface * mix(specular, 1.0, metallic);
    vec3 env = surface * 
        cubeLod(envTex, (vec4(normal, 0.0) * uni0.v).xyz, maxDiffLevel) * 
        (1.0 - metallic) * (1.0 - mirr * roughDark);

    float fogP = clamp(log(length(position) * uni0.fog.x), 0.0, 1.0) * uni0.fog.y;

    int type = int(uni0.etc.x);
    if (type == 0) {
        outColor = vec4(pow(mix(diff + env + spe + miref, uni0.fogColor, fogP), vec3(1.0 / colorFix)) * brightFix, 1.0);
    } else if (type == 1) {
        outColor = vec4(vec3(fnl), 1.0);
    } else if (type == 2) {
        outColor = vec4(diff, 1.0);
    } else if (type == 3) {
        outColor = vec4(env, 1.0);
    } else if (type == 4) {
        outColor = vec4(spe, 1.0);
    } else if (type == 5) {
        outColor = vec4(miref, 1.0);
    } else if (type == 6) {
        outColor = textureLod(envTex, mpos, (cos(uni0.time) * 0.5 + 0.5) * 20.0);
    } else if (type == 7) {
        outColor = vec4(normal, 1.0);
    } else if (type == 8) {
        outColor = vec4(vec3(roughness), 1.0);
    } else if (type == 9) {
        outColor = vec4(vec3(specular), 1.0);
    } else if (type == 10) {
        outColor = vec4(vec3(metallic), 1.0);
    } else if (type == 11) {
        outColor = vec4(vec3(contrast), 1.0);
    }
}