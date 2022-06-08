#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fnormal;
layout(location = 2) in vec3 fposition;
layout(location = 3) in vec2 fuv;
layout(location = 4) in vec2 vpos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform Uni {
    float time;
    float skyForce;
    vec2 fog;
    vec4 resolution;
    vec4 sun;
    mat4 mvp;
    vec4 bprRSMC;
} uni0;

layout(binding = 1) uniform sampler2D albedo;

float hash2(vec2 uv) {
    return fract(sin(dot(uv, vec2(4.5421, 17.3307)) * 2369.4394) + 470.3441);
}

void main() {
    vec3 normal = normalize(fnormal); 
    vec3 position = fposition;
    vec2 uv = fuv;

    vec4 mainTex = pow(texture(albedo, uv), vec4(1.0 / 2.2));

    vec3 light = normalize(vec3(-1.5, -1.6, 0.7));
    vec3 lightCol = vec3(1.0, 0.96, 0.85) * 1.4;
    vec3 envCol = vec3(0.25, 0.4, 0.65);
    vec3 emission = vec3(0.95, 0.23, 0.23) * abs(cos(uni0.time));
    vec3 surface = mainTex.rgb;

    if (hash2(vpos) > mainTex.a) { discard; }
    
    vec3 diff = max(0.0, dot(-light, normal)) * surface * lightCol;
    vec3 env = surface * envCol;
    vec3 ref = reflect(normalize(position), normal);
    vec3 specular = lightCol * pow(max(0.0, dot(-light, ref)), 64.0);

    outColor = vec4(pow(diff + env + specular, vec3(1.0 / 2.2)), 1.0);
}