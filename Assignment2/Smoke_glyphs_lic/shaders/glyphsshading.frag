#version 330 core
// glyphsshading fragment shader

in vec3 vertPosition;
in float value;

uniform sampler1D textureSampler;

out vec4 color;

const vec3 vertNormal = vec3(0.0F, 0.0F, 1.0F);
const vec3 viewPosition = vec3(300.0F, 300.0F, 200.0F);
const vec3 lightPosition = vec3(300.0F, 300.0F, 200.0F);
const vec4 materialCoefficients = vec4(0.5F, 0.5F, 0.8F, 1.0F);

void main()
{
    vec3 materialColor = texture(textureSampler, value).rgb;

    vec3 L = normalize(lightPosition - viewPosition);
    vec3 N = normalize(vertNormal);
    vec3 V = normalize(viewPosition);
    vec3 R = normalize(2 * dot(N, L) * N - L);

    // PHONG SHADING
    vec4 ambient = materialCoefficients[0] * vec4(materialColor, 1.0F);
    vec4 diffuse = materialCoefficients[1] * dot(L, N) * vec4(materialColor, 1.0F);
    vec4 specular = materialCoefficients[2] * pow(dot(R, V), materialCoefficients[3]) * vec4(1.0F);

    color = ambient; // + diffuse + specular;
    color = color / color[3]; // Make sure opacity = 1.0F
}
