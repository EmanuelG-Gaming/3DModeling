#version 300 es
in vec3 position;
in vec3 color;
in vec3 normal;

out vec3 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPosition;
		
void main() {
    // Diffuse reflection
    vec3 lightDirection = normalize(lightPosition - position);
    float dot = dot(lightDirection, normal);
    float intensity = 0.9 * clamp(dot, 0.0, 1.0);
     
    vec3 c = color * (intensity + 0.35);
    vColor = c;
    
    gl_Position = vec4(position.xyz, 1.0) * (model * view * projection);
}