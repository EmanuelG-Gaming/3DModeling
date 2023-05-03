#version 300 es
precision mediump float;

in vec3 vColor;

out vec4 outColor;

void main() {
    vec4 color = vec4(vColor.xyz, 1.0);
    outColor = color;
}