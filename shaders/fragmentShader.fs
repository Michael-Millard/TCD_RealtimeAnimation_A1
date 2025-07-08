#version 330 core

in vec3 FragPos;    // Fragment position in world space
in vec3 Normal;     // Normal vector at the fragment
in vec2 TexCoords;
in vec3 LightDir;   // Direction vector from fragment to light source
in vec3 ViewDir;    // Direction vector from fragment to the camera

out vec4 FragColor; // Output colour

uniform sampler2D textureDiffuse1;
uniform vec3 lightColour;           // Light colour
uniform vec3 ambient;               // Ambient light
uniform float specularExponent;     // Specular exponent

void main() 
{
    // Normalize input vectors
    vec3 N = normalize(Normal);
    vec3 L = normalize(LightDir);
    vec3 V = normalize(ViewDir);

    vec3 colour = vec3(0.0);

    // Blinn Lighting
    vec3 H = normalize(L + V); // Halfway vector
    float spec = pow(max(dot(N, H), 0.0), specularExponent);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColour;
    vec3 specular = spec * lightColour;
    colour = ambient + diffuse + specular;

    vec3 texColour = texture(textureDiffuse1, TexCoords).rgb;
    FragColor = vec4(colour * texColour, 1.0);
}

