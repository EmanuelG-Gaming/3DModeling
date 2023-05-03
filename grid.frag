#version 300 es
precision mediump float;

in vec3 vColor;


out vec4 outColor;

const vec4 fogColor = vec4(0.4, 0.5, 0.9, 1.0); 
const float fogIntensity = 0.001;

void main() {
    float z = (gl_FragCoord.z / gl_FragCoord.w);
    float fog = clamp(exp(-fogIntensity * z * z), 0.2, 1.0);
     
    vec4 color = vec4(vColor.xyz, 1.0);
    outColor = mix(fogColor, color, fog);
}  