#version 330 core
// Scalar data scale vertex shader

layout (location = 0) in vec2 vertCoordinates_in;
layout (location = 1) in float value_in;

uniform mat4 projectionTransform;

uniform float rangeMin;
uniform float rangeMax;
uniform float transferK;

out float value;

void main()
{
    gl_Position = projectionTransform * vec4(vertCoordinates_in, 0.0F, 1.0F);

    // Map values from [rangeMin, rangeMax] to [0, 1].
    float v = value_in + rangeMin + rangeMax; // Remove this placeholder calculation

    // Apply transfer function.
    value = pow(v, transferK);
}

