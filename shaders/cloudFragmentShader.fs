#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;      // Position of the orange light source (e.g., the sun)
uniform vec3 viewPos;       // Camera position
uniform vec3 lightColour;   // Sunlight color (e.g., vec3(1.0, 0.6, 0.2))
uniform float alpha;        // Cloud transparency (e.g., 0.2)
uniform float blendCoeff;   // Lighting blend coefficient

void main() 
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);

    // Lambertian diffuse lighting
    float diff = max(dot(N, L), 0.0);

    // Henyey-Greenstein Scattering Approximation
    float g = 0.1;  // Tweak for different scattering effects
    float cosTheta = dot(L, V);
    float scattering = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);

    // Fresnel Rim Lighting Effect
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 4.0);

    // Final Colour: mix white cloud with sun tint
    vec3 cloudColour = blendCoeff * vec3(1.0) + (1.0 - blendCoeff) * lightColour;
    vec3 finalColour = cloudColour * (diff + scattering + fresnel);

    FragColor = vec4(finalColour, alpha);
}
