#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <cmath>
#include <vector>

#include <QDebug>

namespace interpolation
{
    /* Input
     * values: You may assume this is of the type std::vector<float>. This contains the values (e.g. densities) to be interpolated.
     * sideSize: The input size of the square matrix "values". This is equal to m_DIM in the Simulation and Visualization classes.
     * xMax, yMax: The desired dimensions of the output vector. xMax is the horizontal size (number of columns), yMax is the vertical size (number of rows).
     *
     * Output
     * interpolatedValues: A 1D row-mayor container of std::vector<float> type containing the interpolated values.
     */
    template <typename inVector>
    std::vector<float> interpolateSquareVector(inVector const &values, size_t const sideSize, size_t const xMax, size_t const yMax)
    {
        std::vector<float> interpolatedValues;
        
        // Prepare conversion from sideSize x sideSize matrix 'values' to a xMax x yMax grid 'interpolatedValues'.
        interpolatedValues.resize(xMax * yMax);

        float const cellWidth = static_cast<float>(sideSize) / static_cast<float>(xMax+1); // The number of columns of "values" per glyph
        float const cellHeight = static_cast<float>(sideSize) / static_cast<float>(yMax+1); // The number of rows of "values" per glyph

        // Convert the sideSize x sideSize matrix 'values' to an xMax x yMax grid 'interpolatedValues'.
        for(float i = 0.0F; i < xMax; ++i)
        {
            for(float j = 0.0F; j < yMax; ++j)
            {
                // Bilinear interpolation via the weighted mean: https://en.wikipedia.org/wiki/Bilinear_interpolation#Weighted_mean
                float x = i * cellWidth;
                float y = j * cellHeight;

                size_t x1 = static_cast<size_t>(x);
                size_t y1 = static_cast<size_t>(y);
                size_t x2 = x1 + 1U;
                size_t y2 = y1 + 1U;

                float fx1 = static_cast<float>(x1);
                float fx2 = static_cast<float>(x2);
                float fy1 = static_cast<float>(y1);
                float fy2 = static_cast<float>(y2);

                float w11 = (fx2 - x) * (fy2 - y) / ((fx2 - fx1) * (fy2 - y));
                float w12 = (fx2 - x) * (y - fy1) / ((fx2 - fx1) * (fy2 - y));
                float w21 = (x - fx1) * (fy2 - y) / ((fx2 - fx1) * (fy2 - y));
                float w22 = (x - fx1) * (y - fy1) / ((fx2 - fx1) * (fy2 - y));

                interpolatedValues[j * xMax + i] = w11 * values[xMax * y1 + x1]
                                                 + w12 * values[xMax * y2 + x1]
                                                 + w21 * values[xMax * y1 + x2]
                                                 + w22 * values[xMax * y2 + x2]; // Store in row-major format
            }
        }

        return interpolatedValues;
    }
};

#endif // INTERPOLATION_H
