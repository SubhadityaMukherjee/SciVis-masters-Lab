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
    // - Use the height value to obtain the correct color from the color map.
    // - Use the height change variable to show areas of low change in gray and areas of large change in color (use linear interpolation).
    vec3 placeholder = texture(textureSampler, value).rgb * shading * heightChange * 5.0F; // Replace this
    //vec3 placeholder = texture(textureSampler, value).rgb * shading * heightChange * 5.0F; // Replace this
    //vec3 colors = texture(textureSampler, value).rgb;
    color = vec4(placeholder, 1.0F);
}
