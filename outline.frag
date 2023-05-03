#version 300 es
precision mediump float;

in vec3 vColor;

out vec4 outColor;

uniform float uTime;
void main()
{
     float fTime = uTime * 2.0;
     float d = (sin(fTime) + 1.0) * 0.5 + 0.1;
     vec4 color = vec4(vColor, 1.0);
     color.xyz *= d;
     
     outColor = color;
}