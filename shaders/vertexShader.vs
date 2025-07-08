#version 330 core

layout(location = 0) in vec3 aPos;          // Vertex position
layout(location = 1) in vec3 aNormal;       // Vertex normal
layout(location = 2) in vec2 aTexCoords;    // Texture coordinates

uniform mat4 model;      // Model matrix
uniform mat4 view;       // View matrix
uniform mat4 projection; // Projection matrix
uniform vec3 lightPos;   // Light position in world space
uniform vec3 viewPos;    // Camera (view) position in world space

out vec3 FragPos;    // Fragment position in world space
out vec3 Normal;     // Normal vector in world space
out vec3 LightDir;   // Direction vector from fragment to light source
out vec3 ViewDir;    // Direction vector from fragment to camera
out vec2 TexCoords;  // To pass texture coordinates to fragment shader

void main() 
{
    // Calculate position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Transform normal to world space and normalize
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Texture coordinates
    TexCoords = aTexCoords;

    // Calculate light direction vector
    LightDir = normalize(lightPos - FragPos);

    // Calculate view direction vector
    ViewDir = normalize(viewPos - FragPos);

    // Transform vertex position into clip space
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
