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
<<<<<<< HEAD:Assignment1/Smoke/shaders/heightplot.frag
//    vec3 placeholder = texture(textureSampler, value).rgb * shading * heightChange * 5.0F; // Replace this
    //vec3 placeholder = texture(textureSampler, value).rgb * shading * heightChange * 5.0F; // Replace this
//  vec3 placeholder = mix(textureSampler, gray).rgb;
 //   vec3 color = texture(textureSampler, value).rgb;

    color = texture(textureSampler, value).rgb * shading * heightChange * 5.0F;
    color = vec3(1.0F,1.0F,1.0F,);
    return color;
=======
    vec3 placeholder = texture(textureSampler, value).rgb; // Replace this

    // Linear interpolation for shading based on height change
    placeholder = placeholder * shading * heightChange * 5.0F;

    color = vec4(placeholder, 1.0F);
>>>>>>> development:Assignment 1/Smoke/shaders/heightplot.frag
}
