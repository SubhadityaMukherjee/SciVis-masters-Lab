#include "texture.h"

#include <QDebug>

#include <algorithm>
#include <cmath>
#include <utility>

std::vector<Color> Texture::createGrayscaleTexture(size_t const numberOfColors)
{
    std::vector<Color> texture;
    texture.reserve(3U * numberOfColors);

    for (size_t i = 0U; i < numberOfColors; ++i)
    {
      float const value = static_cast<float>(i) / static_cast<float>(numberOfColors - 1U);
      texture.push_back({value, value, value});
    }

    return texture;
}

std::vector<Color> Texture::createRainbowTexture(size_t const numberOfColors)
{
    std::vector<Color> texture;
    texture.reserve(3U * numberOfColors);

    float const dx = 0.8F;

    for (size_t i = 0U; i < numberOfColors; ++i)
    {
        float value = static_cast<float>(i) / static_cast<float>(numberOfColors - 1U);
        value = (6.0F - 2.0F * dx) * value + dx;

        float const r = std::max(0.0F, (3.0F - std::abs(value - 4.0F) - std::abs(value - 5.0F)) / 2.0F);
        float const g = std::max(0.0F, (4.0F - std::abs(value - 2.0F) - std::abs(value - 4.0F)) / 2.0F);
        float const b = std::max(0.0F, (3.0F - std::abs(value - 1.0F) - std::abs(value - 2.0F)) / 2.0F);

        texture.push_back({r, g, b});
    }

    return texture;
}

std::vector<Color> Texture::createHeatTexture(size_t const numberOfColors)
{
    Color black = {0.0F, 0.0F, 0.0F};
    Color red =   {1.0F, 0.0F, 0.0F};
    Color white = {1.0F, 1.0F, 1.0F};
    return createThreeColorTexture(black, red, white, numberOfColors);
}

std::vector<Color> Texture::createBlueYellowTexture(const size_t numberOfColors)
{
    Color blue =   {0.0F, 0.0F, 0.9F};
    Color white =  {0.6F, 0.6F, 0.6F};
    Color yellow = {0.9F, 0.9F, 0.0F};
    return createThreeColorTexture(blue, white, yellow, numberOfColors);
}

std::vector<Color> Texture::createTwoColorTexture(Color const color1, Color const color2, size_t const numberOfColors)
{
    std::vector<Color> texture;
    texture.reserve(3U * numberOfColors);

    for (size_t i = 0U; i < numberOfColors; ++i)
    {
        float value = static_cast<float>(i) / static_cast<float>(numberOfColors - 1U);

        Color const interpolatedColor = color1 * (1.0F - value) + color2 * value;

        texture.push_back(interpolatedColor);
    }

    return texture;
}

std::vector<Color> Texture::createThreeColorTexture(Color const color1, Color const color2, Color const color3, size_t const numberOfColors)
{
    std::vector<Color> texture;
    texture.reserve(3U * numberOfColors);

    for (size_t i = 0U; i < numberOfColors / 2U; ++i)
    {
        float value = static_cast<float>(i) / static_cast<float>(numberOfColors - 1U);

        Color const interpolatedColor = color1 * (1.0F - 2.0F * value) + color2 * 2.0F * value;

        texture.push_back(interpolatedColor);
    }

    for (size_t i = numberOfColors / 2U; i < numberOfColors; ++i)
    {
        float value = static_cast<float>(i) / static_cast<float>(numberOfColors - 1U);

        Color const interpolatedColor = color2 * (1.0F - 2.0F * (value - 0.5F)) + color3 * 2.0F * (value - 0.5F);

        texture.push_back(interpolatedColor);
    }

    return texture;
}
