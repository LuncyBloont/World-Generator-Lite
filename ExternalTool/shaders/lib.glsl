
const float colorFix = 2.2;
const float brightFix = 1.0;
const float maxDiffLevel = 20.0;

vec3 cubeSky(samplerCube tex, vec3 normal, float fogmix, vec3 fogc) {
	return pow(mix(texture(tex, normal.xzy).xyz, fogc, fogmix), vec3(1.0 / colorFix)) * brightFix;
}

vec3 cubeSkyLod(samplerCube tex, vec3 normal, float lod, float fogmix, vec3 fogc) {
	return pow(mix(textureLod(tex, normal.xzy, lod).xyz, fogc, fogmix), vec3(1.0 / colorFix)) * brightFix;
}

vec3 cube(samplerCube tex, vec3 normal) {
	return texture(tex, normal.xzy).xyz;
}

vec3 cubeLod(samplerCube tex, vec3 normal, float lod) {
	return textureLod(tex, normal.xzy, lod).xyz;
}

float hash2(vec2 uv) {
    return fract(sin(dot(uv, vec2(21.5421, 21.3307)) * 96.4394) + 3.3441);
}

float sampleShadow(sampler2DArray shadowMap, vec4 shadowPos, vec2 bias, vec2 offset) {
	vec3 sdPos = shadowPos.xyz / shadowPos.w;
    vec3 sdUVW = vec3(sdPos.xy, 0.0);
    float sz = sdPos.z;

    for (float i = 0.0; i < 4.0; i += 1.0) {
    	if (abs(sdUVW.x + offset.x * 2.0) < 1.0 && abs(sdUVW.y + offset.y * 2.0) < 1.0 && (sz > 0.0 && sz < 1.0)) {
    		break;
    	}
    	sdUVW = vec3(sdUVW.xy * 0.5, sdUVW.z + 1.0);
    	sz = (sz - 0.5) * 0.5 + 0.5;
    }

    sdUVW.xy = sdUVW.xy * 0.5 + 0.5 + offset;
    
    float shadowRaw = texture(shadowMap, sdUVW).r;

    float sdfact = 1.0;
    if (shadowRaw <= sz) {
        sdfact = mix(1.0, 0.0, clamp((sz - shadowRaw) / bias.x, 0.0, 1.0));
    }
    return sdfact;
}

vec3 worldWind(float time, vec3 wnormal, vec3 wpos) {
    wpos *= 18.0;
    time *= 9.0;
    return 0.001 * (vec3(cos(time * 0.3 + wpos.y) + sin(time * 0.6 + 0.412 + wpos.z), 
                        sin(time * 0.5 - 3.21 + wpos.x) + cos(time * 0.7 + 6.412 + wpos.z), 
                        cos(time * 0.62 + 7.301 + wpos.x) - sin(time * 0.41 - 0.166 + wpos.y)) +
                        wnormal * cos(time * 0.513 - 0.6881) * 0.1);
}

layout(binding = 0) uniform UniFrame {
    float time;
    float skyForce;
    vec2 fog;
    vec3 fogColor;
    vec4 resolution;
    vec4 sun;
    vec3 sunDir;
    mat4 v;
    mat4 p;
    vec4 etc;
    vec2 shadowBias;
} frame;

layout(binding = 1) uniform UniObject {
	float subsurface;
	float ssbase;
    float clip;
	mat4 mvp;
	vec4 pbrRSMC;
	vec4 baseColor;
	mat4 m;
	vec4 scale;
	vec4 pbrBase;
} object;

layout(binding = 2) uniform UniShadow {
	mat4 shadowV;
} shadow;

layout(binding = 3) uniform sampler2D albedo;
layout(binding = 4) uniform sampler2D pbrTex;
layout(binding = 5) uniform samplerCube envTex;
layout(binding = 6) uniform sampler2D normalTex;

layout(binding = 7) uniform sampler2DArray shadowMap;
layout(binding = 8) uniform sampler2D dynamicMap;