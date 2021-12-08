#include "isoline.h"
#include <iostream>
#include <QDebug>

Isoline::Isoline(std::vector<float> const &values,
                 size_t const DIM,
                 float const isolineRho,
                 float const cellSideLength,
                 InterpolationMethod const interpolationMethod)
:
    m_values(values),
    m_DIM(DIM),
    m_isolineRho(isolineRho),
    m_cellSideLength(cellSideLength)
{
    switch (interpolationMethod)
    {
        case InterpolationMethod::Linear:
            marchingSquaresInterpolated();
        break;

        case InterpolationMethod::None:
            marchingSquaresNonInterpolated();
        break;
    }
}


void Isoline::marchingSquaresNonInterpolated()
{
    std::vector<bool> binary;
    binary.reserve(m_values.size());

    // Create binary matrix based on m_isolinesRho
    for (size_t j = 0U; j < (m_DIM - 1U); ++j)
    {
        for (size_t i = 0U; i < (m_DIM - 1U); ++i)
        {
            binary[i + m_DIM * j] = m_values[i + m_DIM * j] > m_isolineRho;
        }
    }

    // Loop over the bottom left corner of each square
    for (size_t j = 0U; j < (m_DIM - 1U); ++j)
    {
        for (size_t i = 0U; i < (m_DIM - 1U); ++i)
        {
            size_t tableIdx = (size_t) binary[i + m_DIM * (j+1)] << 1; // v3, top left
            tableIdx = (tableIdx + (size_t) binary[(i+1) + m_DIM * (j+1)]) << 1; // v2, top right
            tableIdx = (tableIdx + (size_t) binary[(i+1) + m_DIM * j]) << 1; // v1, bottom right
            tableIdx = (tableIdx + (size_t) binary[i + m_DIM * j]); // v0, bottom left

            // For drawing offset the cells a little to the right and up to make it match the scalar field.
            QVector2D const bottomLeft{static_cast<float>(i + 1U) * m_cellSideLength,
                                       static_cast<float>(j + 1U) * m_cellSideLength};

            jumpTableNonInterpolated[tableIdx](bottomLeft);
        }
    }
}

void Isoline::marchingSquaresInterpolated()
{
    // Loop over the bottom left corner of each square
    for (size_t j = 0U; j < (m_DIM - 1U); ++j)
    {
        for (size_t i = 0U; i < (m_DIM - 1U); ++i)
        {
            // Insert code here...

            // v0 to v1 designate the corners of the current square.
            // Their values should be the corresponding indices in the 'm_values' container.
            size_t const v0 = 0U; // placeholder value
            size_t const v1 = 0U; // placeholder value
            size_t const v2 = 0U; // placeholder value
            size_t const v3 = 0U; // placeholder value

            size_t tableIdx = 0U; // placeholder value

            // For drawing offset the cells a little to the right and up to make it match the scalar field.
            QVector2D const bottomLeft{static_cast<float>(i + 1U) * m_cellSideLength,
                                       static_cast<float>(j + 1U) * m_cellSideLength};

            jumpTable[tableIdx](bottomLeft, v0, v1, v2, v3);
        }
    }
}

std::vector<QVector2D> Isoline::vertices() const
{
    return m_vertices;
}

/* The [[maybe_unused]] attribute is used to suppress the many "unused parameter" warnings
 * that would otherwise appear in an incomplete implementation.
 * These attributes may be kept once the implementation is completed (they won't affect
 * the execution), but may also be removed in case a variable _is_ used.
 */

// Interpolated cases
void Isoline::case0([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case1(QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    size_t const v3)
{
    // General interpolation
    float const deltaX = (m_isolineRho - m_values[v0]) / (m_values[v1] - m_values[v0]);
    float const deltaY = (m_isolineRho - m_values[v0]) / (m_values[v3] - m_values[v0]);

    // Add the start and end coordinates of the line,
    // based on an offset of the location of the bottom-left corner
    // and as a point somewhere along the length of the side of a cell/square.
    m_vertices.emplace_back(bottomLeft + QVector2D{deltaX * m_cellSideLength, 0.0F});
    m_vertices.emplace_back(bottomLeft + QVector2D{0.0F, deltaY * m_cellSideLength});
}

void Isoline::case2([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case3([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case4([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case5([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case6([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case7([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case8([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case9([[maybe_unused]] QVector2D const &bottomLeft,
                    [[maybe_unused]] size_t const v0,
                    [[maybe_unused]] size_t const v1,
                    [[maybe_unused]] size_t const v2,
                    [[maybe_unused]] size_t const v3)
{
}

void Isoline::case10([[maybe_unused]] QVector2D const &bottomLeft,
                     [[maybe_unused]] size_t const v0,
                     [[maybe_unused]] size_t const v1,
                     [[maybe_unused]] size_t const v2,
                     [[maybe_unused]] size_t const v3)
{
}

void Isoline::case11([[maybe_unused]] QVector2D const &bottomLeft,
                     [[maybe_unused]] size_t const v0,
                     [[maybe_unused]] size_t const v1,
                     [[maybe_unused]] size_t const v2,
                     [[maybe_unused]] size_t const v3)
{
}

void Isoline::case12([[maybe_unused]] QVector2D const &bottomLeft,
                     [[maybe_unused]] size_t const v0,
                     [[maybe_unused]] size_t const v1,
                     [[maybe_unused]] size_t const v2,
                     [[maybe_unused]] size_t const v3)
{
}

void Isoline::case13([[maybe_unused]] QVector2D const &bottomLeft,
                     [[maybe_unused]] size_t const v0,
                     [[maybe_unused]] size_t const v1,
                     [[maybe_unused]] size_t const v2,
                     [[maybe_unused]] size_t const v3)
{
}

void Isoline::case14([[maybe_unused]] QVector2D const &bottomLeft,
                     [[maybe_unused]] size_t const v0,
                     [[maybe_unused]] size_t const v1,
                     [[maybe_unused]] size_t const v2,
                     [[maybe_unused]] size_t const v3)
{
}

void Isoline::case15([[maybe_unused]] QVector2D const &bottomLeft,
                     [[maybe_unused]] size_t const v0,
                     [[maybe_unused]] size_t const v1,
                     [[maybe_unused]] size_t const v2,
                     [[maybe_unused]] size_t const v3)
{
}


// NonInterpolated cases
void Isoline::case0NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
}

void Isoline::case1NonInterpolated(QVector2D const &bottomLeft)
{
    QVector2D const v_out_0{bottomLeft + QVector2D{0.5F * m_cellSideLength, 0.0F}};
    QVector2D const v_out_1{bottomLeft + QVector2D{0.0F, 0.5F * m_cellSideLength}};

    m_vertices.push_back(v_out_0);
    m_vertices.push_back(v_out_1);
}

void Isoline::case2NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    QVector2D const v_out_0{bottomLeft + QVector2D{0.5F * m_cellSideLength, 0.0F}};
    QVector2D const v_out_1{bottomLeft + QVector2D{1.0F * m_cellSideLength, 0.5F * m_cellSideLength}};

    m_vertices.push_back(v_out_0);
    m_vertices.push_back(v_out_1);
}

void Isoline::case3NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    QVector2D const v_out_0{bottomLeft + QVector2D{0.0F, 0.5F * m_cellSideLength}};
    QVector2D const v_out_1{bottomLeft + QVector2D{1.0F * m_cellSideLength, 0.5F * m_cellSideLength}};

    m_vertices.push_back(v_out_0);
    m_vertices.push_back(v_out_1);
}

void Isoline::case4NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    QVector2D const v_out_0{bottomLeft + QVector2D{1.0F * m_cellSideLength, 0.5F * m_cellSideLength}};
    QVector2D const v_out_1{bottomLeft + QVector2D{0.5F * m_cellSideLength, 1.0F * m_cellSideLength}};

    m_vertices.push_back(v_out_0);
    m_vertices.push_back(v_out_1);
}

void Isoline::case5NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 1 plus case 4
    Isoline::case1NonInterpolated(bottomLeft);
    Isoline::case4NonInterpolated(bottomLeft);
}

void Isoline::case6NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    QVector2D const v_out_0{bottomLeft + QVector2D{0.5F * m_cellSideLength, 0.0F}};
    QVector2D const v_out_1{bottomLeft + QVector2D{0.5F * m_cellSideLength, 1.0F * m_cellSideLength}};

    m_vertices.push_back(v_out_0);
    m_vertices.push_back(v_out_1);
}

void Isoline::case7NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    QVector2D const v_out_0{bottomLeft + QVector2D{0.0F, 0.5F * m_cellSideLength}};
    QVector2D const v_out_1{bottomLeft + QVector2D{0.5F * m_cellSideLength, 1.0F * m_cellSideLength}};

    m_vertices.push_back(v_out_0);
    m_vertices.push_back(v_out_1);
}

void Isoline::case8NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 7
    Isoline::case7NonInterpolated(bottomLeft);
}

void Isoline::case9NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 6
    Isoline::case6NonInterpolated(bottomLeft);
}

void Isoline::case10NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 2 plus case 7
    Isoline::case2NonInterpolated(bottomLeft);
    Isoline::case7NonInterpolated(bottomLeft);
}

void Isoline::case11NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 4
    Isoline::case4NonInterpolated(bottomLeft);
}

void Isoline::case12NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 3
    Isoline::case3NonInterpolated(bottomLeft);
}

void Isoline::case13NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 2
    Isoline::case2NonInterpolated(bottomLeft);
}

void Isoline::case14NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Same as case 1
    Isoline::case1NonInterpolated(bottomLeft);
}

void Isoline::case15NonInterpolated([[maybe_unused]] QVector2D const &bottomLeft)
{
    // Nothing, same as case 0
}
