#version 450

precision highp float;

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    fragColor = vec4(fragCoord / iResolution.xy, 1.0, 1.0);
}