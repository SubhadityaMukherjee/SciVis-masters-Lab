#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>

void writeImage(std::vector<std::array<float, 4U>> image,std::string filename="line_integrated.ppm")
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

void convolute(std::vector<float> &scalarValues, std::vector<std::vector<float>> &kernel)
 {
     int m_DIM = 255U;
    std::vector<float> input(scalarValues);

    // Fill output matrix: rows and columns are i and j respectively
    for (unsigned long i = 0; i < m_DIM; ++i)
    {
         for (unsigned long j = 0; j < m_DIM; ++j)
         {
             float convoluteSum = 0.0F;

            // Kernel rows and columns are k and l respectively
             for (unsigned long k = 0; k < 3; ++k)
             {
                 for (unsigned long l = 0; l < 3; ++l)
                {
                     // Convolute here.
                     if ((i == 0 && k == 0) || (i == m_DIM-1 && k == 2)) continue; // x value is out of bounds, so ignore this field
                     if ((j == 0 && l == 0) || (j == m_DIM-1 && l == 2)) continue; // y value is out of bounds, so ignore this field
                     unsigned long x = i + k - 1;
                     unsigned long y = j + l - 1;
                     convoluteSum += input[x + m_DIM * y] * kernel[k][l];
                 }
             }
             scalarValues[i + m_DIM * j] = convoluteSum; // Add result to output matrix.
        }
    }
 }



int main()
{
    // Random gray image
    std::vector<std::array<float, 4U>> image;

    for (size_t yIdx = 0U; yIdx < 256U; ++yIdx) {
        for (size_t xIdx = 0U; xIdx < 256U; ++xIdx) {

            // image.emplace_back(std::array<float, 4U>{(float) rand()/RAND_MAX, (float) rand()/RAND_MAX, (float) rand()/RAND_MAX, (float) rand()/RAND_MAX});
            image.emplace_back(std::array<float, 4U>{0.2,0.2,0.2,0.2});
        }
    }
    
    writeImage(image);

}
