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

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform Uni {
    float time;
    float skyForce;
    vec2 fog;
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
    mat4 nm;
} uni0;

layout(binding = 1) uniform sampler2D albedo;
layout(binding = 2) uniform sampler2D pbrTex;
layout(binding = 3) uniform samplerCube envTex;

float hash2(vec2 uv) {
    return fract(sin(dot(uv, vec2(4.5421, 17.3307)) * 2369.4394) + 470.3441);
}

void main() {
    vec3 normal = normalize(fnormal); 
    vec3 position = fposition;
    vec2 uv = fuv;

    vec4 mainTex = pow(texture(albedo, uv), vec4(1.0 / 2.2)) * uni0.baseColor;

    vec3 surface = mainTex.rgb;

    float roughness = uni0.bprRSMC.x;
    float specular = uni0.bprRSMC.y * 2.0;
    float metallic = uni0.bprRSMC.z;
    float contrast = uni0.bprRSMC.w * 2.0;
    float fnlBase = 0.05;
    float fnlScale = 0.76;

    if (hash2(vpos) > mainTex.a) { discard; }

    surface = pow(surface, vec3(contrast));
    
    vec3 ref = reflect(normalize(position), normal);
    float fnl = fnlBase * fnlScale + 
        (1.0 - fnlBase) * fnlScale * pow(1.0 - abs(dot(normal, -normalize(position))), 6.0);
    float mirr = mix(1.0, fnl, 1.0 - metallic);
    vec3 insubface = mix(vec3(1.0), surface, metallic);
    vec3 spe = 30.0 * mirr * uni0.sun.xyz * uni0.sun.w * insubface *
        pow(max(0.0, dot(-sunDir, ref)), pow(1.0 / roughness, 2.0)) * 
        pow(1.0 - roughness, 2.0) * specular;
    vec3 diff = max(0.0, dot(-sunDir, normal)) * surface * uni0.sun.xyz * uni0.sun.w * (1.0 - metallic);
    vec3 miref = mirr * 
        cubeLod(envTex, (vec4(ref, 0.0) * uni0.v).xyz, roughness * 10.0) * 
        insubface * specular;
    vec3 env = surface * 
        cubeLod(envTex, (vec4(normal, 0.0) * uni0.v).xyz, 10.0) * 
        (1.0 - metallic) * (1.0 - mirr);

    int type = int(uni0.etc.x);
    if (type == 0) {
        outColor = vec4(pow(diff + env + spe + miref, vec3(1.0 / 2.2)), 1.0);
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
    }
}