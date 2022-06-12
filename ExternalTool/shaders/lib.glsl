
vec3 cubeSky(samplerCube tex, vec3 normal) {
	return pow(texture(tex, normal.xzy).xyz * 2.0, vec3(1.0 / 2.2));
}

vec3 cubeSkyLod(samplerCube tex, vec3 normal, float lod) {
	return pow(textureLod(tex, normal.xzy, lod).xyz * 2.0, vec3(1.0 / 2.2));
}

vec3 cube(samplerCube tex, vec3 normal) {
	return texture(tex, normal.xzy).xyz * 2.0;
}

vec3 cubeLod(samplerCube tex, vec3 normal, float lod) {
	return textureLod(tex, normal.xzy, lod).xyz * 2.0;
}