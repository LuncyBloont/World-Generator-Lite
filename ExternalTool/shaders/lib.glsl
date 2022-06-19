
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
    return fract(sin(dot(uv, vec2(4.5421, 17.3307)) * 2369.4394) + 470.3441);
}