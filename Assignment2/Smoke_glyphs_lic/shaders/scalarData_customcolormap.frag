#version 330 core
// Colormap fragment shader

in float value;

uniform vec3 colorMapColors[3];

out vec4 color;

void main()
{
    // Determine the considered value range
    float f_min = 0.0F;
    float f_max = 1.0F;

    // Transfer the density value to a normalized coordinate
    float x = (value - f_min) / (f_max - f_min);
    int i = int(x);

    // Use this value to access a color table
    vec3 texColor = colorMapColors[i];

    color = vec4(texColor, 1.0F);
}
