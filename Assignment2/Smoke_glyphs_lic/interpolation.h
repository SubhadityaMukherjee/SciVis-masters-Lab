#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <cmath>
#include <vector>

#include <QDebug>

#include <iostream>
namespace interpolation
{
    /* Input
     * values: You may assume this is of the type std::vector<float>. This contains the values (e.g. densities) to be interpolated.
     * sideSize: The input size of the square matrix "values". This is equal to m_DIM in the Simulation and Visualization classes.
     * xMax, yMax: The desired dimensions of the output vector. xMax is the horizontal size (number of columns), yMax is the vertical size (number of rows).
     *
     * Output
     * interpolatedValues: A 1D row-major container of std::vector<float> type containing the interpolated values.
     */
    template <typename inVector>
    std::vector<float> interpolateSquareVector(inVector const &values, size_t const sideSize, size_t const xMax, size_t const yMax)
    {
        std::vector<float> interpolatedValues;
        
        // Prepare conversion from sideSize x sideSize matrix 'values' to a xMax x yMax grid 'interpolatedValues'.
        interpolatedValues.reserve(xMax * yMax);
        size_t const cellWidth = sideSize / xMax; // The number of columns of "values" per glyph
        size_t const cellHeight = sideSize / yMax; // The number of rows of "values" per glyph

        // Convert the sideSize x sideSize matrix 'values' to an xMax x yMax grid 'interpolatedValues'.
        size_t newY = sideSize * cellHeight;
        for(size_t i = 0; i < xMax; ++i)
        {
            for(size_t j = 0; j < yMax; ++j)
            {
                // Use the four points of the cell for interpolation
                float v0 = values[newY * j + i * cellWidth]; // bottomleft
                float v1 = values[newY * j + (i+1) * cellWidth]; // bottomright
                float v2 = values[newY * (j+1) + (i+1) * cellWidth]; // topright
                float v3 = values[newY * (j+1) + i * cellWidth]; // topleft

                interpolatedValues[j * yMax + i] = (v0 + v1 + v2 + v3) / 4;
            }
        }

        return interpolatedValues;
    }
};

#endif // INTERPOLATION_H
