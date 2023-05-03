#version 300 es
in vec3 position;
in vec3 color;
in vec2 textureCoords;

out vec3 vColor;
out vec2 vTextureCoords;

uniform mat4 projection;
	
void main() {
    vColor = color;
    vTextureCoords = textureCoords;
    
    gl_Position = vec4(position.xyz, 1.0) * projection;
}