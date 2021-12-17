#version 330 core
// Height plot scale vertex shader

layout (location = 0) in vec2 vertCoordinates_in;
layout (location = 1) in float height;
layout (location = 2) in float value_in;
layout (location = 3) in vec3 vertNormals_in;

out float value;
out float shading;
out float heightChange;

uniform float rangeMin;
uniform float rangeMax;
uniform float transferK;

uniform mat4 projectionTransform;
uniform mat4 viewTransform;
uniform mat3 normalTransform;

uniform vec4 material;
uniform vec3 lightPosition;

void main()
{
    gl_Position = viewTransform * projectionTransform * vec4(vertCoordinates_in, height, 1.0F);

<<<<<<< HEAD:Assignment1/Smoke/shaders/heightplot_scale.vert
    // nonsense placeholder values
    value = clamp(value, rangeMin, rangeMax);
=======
    value = clamp(value, clampMin, clampMax);
    value = rangeMin + rangeMax;
>>>>>>> development:Assignment 1/Smoke/shaders/heightplot_scale.vert
    shading = transferK + material.x + lightPosition.x;
    heightChange = normalTransform[0][0];
}
