#version 300 es
in vec3 position;
in vec3 color;
in vec3 normal;

out vec4 vPosition;
out vec3 vColor;
out vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
		
void main() {
    vec4 pos = vec4(position.xyz, 1.0);
    
    vColor = color;
    vNormal = normal;
    vPosition = pos;
    
    gl_Position = pos * (model * view * projection);
}    