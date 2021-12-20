#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>
// #include <Magick++.h>
// #include <opencv2/opencv.hpp> 

// using namespace Magick; 
// using namespace cv;

// Define colors for the colormap
float const colorNode0[3U] = { 0.0F, 0.0F, 1.0F };  // blue
float const colorNode1[3U] = { 0.0F, 1.0F, 0.0F };  // white
float const colorNode2[3U] = { 1.0F, 0.0F, 0.0F };  // red

float composedColor[4U] = { 0.0F };
// Number of maximum raycasting samples per ray
const float sampleNum = 150.0F;

// TODO: modify the transferFunction and add necessary functions

std::array<float, 4U> transferFunction(float value)
{
    value /= 255.0F; // to range [0...1]
    
    float alpha = value * 0.1F; // value; ???

    if (value < 0.2F)
        alpha = 0.0F;
	
    float color0[3U] = { 0.0F };
    std::copy(std::begin(colorNode0), std::end(colorNode0), std::begin(color0));
    float color1[3U] = { 0.0F };
    std::copy(std::begin(colorNode1), std::end(colorNode1), std::begin(color1));
    
    float t = 0.0F;
    if (value < 0.5F)
    {
        t = 2.0F * value;
    }
    else
    {
        t = 2.0F * (value - 0.5F);
        std::copy(std::begin(colorNode1), std::end(colorNode1), std::begin(color0));
        std::copy(std::begin(colorNode2), std::end(colorNode2), std::begin(color1));
    }
    
    std::array<float, 4U> color;

    color[3U] = alpha;
    
    for (size_t idx = 0U; idx < 3U; ++idx) // rgb
        color[idx] = color0[idx] * (1.0F - t) + color1[idx] * t;
    
    return color;
}

void writeImage(std::vector<std::array<float, 4U>> image,std::string filename="pre-integrated.ppm")
{
    size_t const xDim = 256U;
    size_t const yDim = 256U;

    std::ofstream outStream{filename};
    outStream.exceptions(std::ofstream::failbit);

    try
    {
        outStream << "P3\n" << xDim << ' ' << yDim << "\n255\n";
        for (size_t yIdx = 0U; yIdx < yDim; ++yIdx)
        {
            for (size_t xIdx = 0U; xIdx < xDim; ++xIdx)
            {
                size_t const imageIdx = (yIdx * xDim) + xIdx;
                for (size_t rgbIdx = 0U; rgbIdx < 3U; ++rgbIdx)
                {
                    auto const valAsFloat = image[imageIdx][rgbIdx];
                    auto const valAsUint = std::clamp(static_cast<unsigned int>(valAsFloat * 255.0F), 0U, 255U);
                    outStream << valAsUint
                              << (xIdx == (xDim - 1U) and rgbIdx == 2U ? '\n' : ' ');

                }
            }
            outStream << '\n';
        }
    }
    catch (std::ios_base::failure const &fail)
    {
        std::cout << "Exception thrown when writing to file: " << fail.what() << '\n';
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Correct opacity for the current sampling rate
 *
 * @param alpha: input opacity.
 * @param samplingRatio: the ratio between current sampling rate and the original.
 */
float opacityCorrection(float alpha, float samplingRatio)
{
    float a_corrected = 1.0F - pow(1.0F - alpha, samplingRatio);
    return a_corrected;
}

/**
 * Pre-integration composition, based on accumulation
 *
 * @param sample: current sample value.
 * @param samplingRatio: the ratio between current sampling rate and the original. (ray step)
 * @param composedColor: blended color (both input and output)
 */
std::array<float, 4U> accumulation(float transferValue, float samplingRatio, std::array<float, 4U> composedColor)
{
    // Get color from transfer function and correct opacity
    std::array<float, 4U> color = transferFunction(transferValue);
    color[3] = opacityCorrection(color[3], samplingRatio);
    float alpha_i = composedColor[3];
    
    // Easier naming
    std::array<float, 3U> composedXYZ;
    std::copy(composedColor.begin(), composedColor.begin() + 3, composedXYZ.begin());
    std::array<float, 3U> colorXYZ;
    std::copy(color.begin(), color.begin() + 3, colorXYZ.begin());
    
    // Calculate new values
    std::array<float, 3U> color_new;
    std::transform(composedXYZ.begin(), composedXYZ.end(), colorXYZ.begin(), color_new.begin(), [alpha_i, color](float &first, float &second) {
        return first + (1.0F - alpha_i) * color[3] * second;
    }); // composedXYZ + (1.0F - alpha_i) * color[3] * colorXYZ;
    float alpha_new = alpha_i + (1.0F - alpha_i) * color[3];
    
    // Store as result
    std::copy(color_new.begin(), color_new.end(), composedColor.begin());
    composedColor[3] = alpha_new;
    
    return composedColor;
}

std::array<float, 4U> preintegration(size_t xIdx, size_t yIdx)
{
    float s_f = static_cast<float>(xIdx);
    float s_b = static_cast<float>(yIdx);
    
    std::array<float, 4U> composedColor;

    for (size_t i = 0; i < sampleNum; ++i)
    {
        float samplingRatio = i / sampleNum;
    
        // 1. Sampling: interpolation of the two scalars
        float s_L = s_b + samplingRatio * (s_f - s_b);

        // 2. Classification: apply transfer function
        std::array<float, 4U> color = transferFunction(s_L);

        // 3. Compositing: perform pre-integration/accumulation
        composedColor = accumulation(s_L, samplingRatio, composedColor);
    }
    
    return composedColor;
}

int main()
{
    // 2D color image
//    float color2D[256U][256U][4U] = { 0.0F };
    std::vector<std::array<float, 4U>> image;

    for (size_t yIdx = 0U; yIdx < 256U; ++yIdx) {
        for (size_t xIdx = 0U; xIdx < 256U; ++xIdx) {
            // DONE: add pre-integration table calculations
            std::array<float, 4U> composedColor = preintegration(xIdx, yIdx);

            // DONE: convert obtained array to an image
            image.emplace_back(composedColor);
        }
    }
    
    writeImage(image);

}
