#version 330 core
// Scalar data clamp vertex shader

layout (location = 0) in vec2 vertCoordinates_in;
layout (location = 1) in float value_in;

uniform mat4 projectionTransform;

uniform float clampMin;
uniform float clampMax;
uniform float transferK;

out float value;

void main()
{
    gl_Position = projectionTransform * vec4(vertCoordinates_in, 0.0F, 1.0F);

    // Clamp values.
    value = clamp(value_in, clampMin, clampMax);

    // Map the range [clampMin, clampMax] to [0, 1].
    value = clampMin + clampMax; // Remove this placeholder value

    // Apply transfer function.
    value = pow(value, transferK);
}
