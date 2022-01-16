#include "lic.h"

#include <QDebug>

#include<vector>
#include<numeric>
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>

Lic::Lic(unsigned int xDim, unsigned int yDim)
    :
      dim_x(xDim),
      dim_y(yDim)
{
    texture = std::vector<float>(dim_x * dim_y, 0.0F);

    boxFilterLUT_1 = std::vector<float>(); // Look-up-table filter
    boxFilterLUT_2 = std::vector<float>(); // Look-up-table filter
    texture = generateNoiseTexture(texture); // Generate original noise texture

    for (int i = 0; i < LUT_FILTER_SIZE; ++i)
    {
        boxFilterLUT_1.push_back(i);
        boxFilterLUT_2.push_back(i);
    }

    qDebug() << "LIC Constructed";
}

///		normalize the vector field     ///
void Lic::normalizeVectors(std::vector<float>* vectorField_x, std::vector<float>* vectorField_y)
{
    size_t length = vectorField_x->size();

    for (size_t i = 0; i < length; ++i)
    {
        // Scale each vector to length 1
        float factor = sqrt(pow((*vectorField_x)[i], 2.0F) + pow((*vectorField_y)[i], 2.0F));
        (*vectorField_x)[i] /= factor;
        (*vectorField_y)[i] /= factor;
    }
//    float totals_x = std::inner_product( vectorField_x->begin(), vectorField_x->end(), vectorField_x->begin(), 0 );
//    float totals_y = std::inner_product( vectorField_y->begin(), vectorField_y->end(), vectorField_y->begin(), 0 );
//    std::for_each(vectorField_x->begin(), vectorField_x->end(), [totals_x](float &c){ c /= totals_x; });
//    std::for_each(vectorField_y->begin(), vectorField_y->end(), [totals_y](float &c){ c /= totals_y; });
}


///		make white noise as the LIC input texture     ///
std::vector<float> Lic::generateNoiseTexture(std::vector<float> texture)
{
    // Seed random number generator
    srand(time(NULL));

    // Generate random values: black or white
    generate(begin(texture), end(texture), []() { return static_cast<float>(rand() % 256); });
    return texture;
}

std::vector<uint8_t> Lic::mapFlowToTexture(std::vector<float> vectorField_x, std::vector<float> vectorField_y, std::vector<float> texture_in)
{
    std::vector<uint8_t> newTexture(dim_x*dim_y);
    size_t L = 4; // Fixed length to determine streamline

    for (size_t i = 0; i < dim_x; ++i)
    {
        for (size_t j = 0; j < dim_y; ++j)
        {
            // Find the streamline
            std::vector<std::vector<size_t>> streamline; // (L+1) x 2 matrix
            streamline.resize(L+1);

            streamline[0] = std::vector<size_t>{i, j}; // current pixel

            size_t x = i;
            size_t y = j;
            for (size_t k = 0; k < L; ++k) // Forward
            {
                float vecX = vectorField_x[y * dim_x + x];
                float vecY = vectorField_y[y * dim_x + x];

                // Divide the 'circle' around this pixel into eight segments, each one representing the angle to one of the neighbouring pixels.
                // We can see this circle as the unit circle.

                // The vector points to the neighbour on the right when the angle of the vector from the x-axis is smaller than pi/8, i.e. when x > sqrt(3)/2
                if (vecX >= sqrt(3)/2)
                {
                    x = x+1;
                }
                // The vector points to the upper right or lower right neighbour when the angle is in between +/- pi/8 and +/- 3/8*pi.
                else if (vecX >= 1/2)
                {
                    if (vecY > 0) // upper right
                    {
                        x = x+1;
                        y = y+1;
                    }
                    else // lower right
                    {
                        x = x+1;
                        y = y-1;
                    }
                }
                // The vector points to the upper or lower neighbour when the angle is in between +/- 3/8*pi and +/- 5/8*pi.
                else if (vecX >= -1/2)
                {
                    if (vecY > 0) // upper
                    {
                        y = y+1;
                    }
                    else // lower
                    {
                        y = y-1;
                    }
                }
                // The vector points to the upper or lower left neighbour when the angle is in between +/- 5/8 and +/- 7/8*pi.
                else if (vecX >= -sqrt(3)/2)
                {
                    if (vecY > 0) // upper left
                    {
                        x = x-1;
                        y = y+1;
                    }
                    else // lower left
                    {
                        x = x-1;
                        y = y-1;
                    }
                }
                else // -1 > x > -sqrt(3)/2: the vector points to the left neighbour.
                {
                    x = x-1;
                }

                streamline[1 + k] = std::vector<size_t>{i, j};
            }

            // Apply kernel: simple box blur, i.e. each pixel has a value equal to the average value of its neighbours.
            float sum = 0.0F;
            size_t count = 0;
            for (std::vector<size_t> coordinates : streamline)
            {
                if (0 <= coordinates[0] && coordinates[0] < dim_x && 0 <= coordinates[1] && coordinates[1] < dim_y)
                {
                    sum += texture_in[coordinates[1] + coordinates[0] * dim_y]; // column-major
                    count++;
                }
            }
            newTexture[j + i * dim_y] = static_cast<uint8_t>(sum / static_cast<float>(count)); // column-major, which is needed by GLSL.
        }
    }

    return newTexture;
}

std::vector<uint8_t> Lic::updateTexture(std::vector<float> vectorField_x, std::vector<float> vectorField_y, std::vector<float> texture_in)
{
    //you shouldn't need to edit this!

    normalizeVectors(&vectorField_x, &vectorField_y);
    std::vector<uint8_t> const texture_out = mapFlowToTexture(vectorField_x, vectorField_y, texture_in);

    return texture_out;
}

std::vector<uint8_t> Lic::updateTexture(std::vector<float> vectorField_x, std::vector<float> vectorField_y, std::vector<float> texture_in, unsigned int newDim_x, unsigned int newDim_y)
{
    //you shouldn't need to edit this!

    int dims = newDim_x * newDim_y;
    if (dims != vectorField_x.size() || dims != vectorField_y.size())
    {
        qDebug() << "Dimension mismatch between LIC dimensions and incoming vector fields, aborting";
        std::vector<uint8_t> failed_texture;
        return failed_texture;
    }

    setXDim(newDim_x);
    setYDim(newDim_y);

    normalizeVectors(&vectorField_x, &vectorField_y);
    std::vector<uint8_t> const texture_out = mapFlowToTexture(vectorField_x, vectorField_y, texture_in);

    return texture_out;
}

void Lic::resetTexture()
{
    texture = generateNoiseTexture(texture);
}

void Lic::resetTexture(unsigned int newXDim, unsigned int newYDim)
{
    setXDim(newXDim);
    setYDim(newYDim);

    texture = std::vector<float>(dim_x * dim_y, 0.0F);

    texture = generateNoiseTexture(texture);
}
