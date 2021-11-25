#version 330 core
// Height plot fragment shader

in float value;
in float shading;
in float heightChange;

uniform sampler1D textureSampler;

out vec4 color;

void main()
{
  vec3 gray = vec3(0.3F, 0.3F, 0.3F);

    //
    // TODO:
    // - use the height value to obtain the correct color form color map and write
    // - use the height change variable to show areas of low change in gray, but areas of large change in color
    vec3 placeholder = texture(textureSampler, value).rgb * shading * heightChange * 5.0F; // Replace this
    color = vec4(placeholder, 1.0F);
}
