#ifndef ISOLINE_H
#define ISOLINE_H

#include <QVector2D>

#include <array>
#include <functional>
#include <optional>
#include <vector>

using namespace std::placeholders;

class Isoline
{
    std::vector<QVector2D> m_vertices;
    std::vector<float> m_values;
    size_t const m_DIM;
    float const m_isolineRho;
    float const m_cellSideLength;
    QVector2D const m_vertex0;

    // Marching squares functions.
    void marchingSquaresInterpolated();
    void marchingSquaresNonInterpolated();

    void case0( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case1( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case2( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case3( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case4( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case5( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case6( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case7( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case8( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case9( QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case10(QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case11(QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case12(QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case13(QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case14(QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);
    void case15(QVector2D const &bottomLeft, size_t const v0, size_t const v1, size_t const v2, size_t const v3);

    void case0NonInterpolated( QVector2D const &bottomLeft);
    void case1NonInterpolated( QVector2D const &bottomLeft);
    void case2NonInterpolated( QVector2D const &bottomLeft);
    void case3NonInterpolated( QVector2D const &bottomLeft);
    void case4NonInterpolated( QVector2D const &bottomLeft);
    void case5NonInterpolated( QVector2D const &bottomLeft);
    void case6NonInterpolated( QVector2D const &bottomLeft);
    void case7NonInterpolated( QVector2D const &bottomLeft);
    void case8NonInterpolated( QVector2D const &bottomLeft);
    void case9NonInterpolated( QVector2D const &bottomLeft);
    void case10NonInterpolated(QVector2D const &bottomLeft);
    void case11NonInterpolated(QVector2D const &bottomLeft);
    void case12NonInterpolated(QVector2D const &bottomLeft);
    void case13NonInterpolated(QVector2D const &bottomLeft);
    void case14NonInterpolated(QVector2D const &bottomLeft);
    void case15NonInterpolated(QVector2D const &bottomLeft);

    std::array<std::function<void(QVector2D const &bottomLeft,
                                  size_t const,
                                  size_t const,
                                  size_t const,
                                  size_t const)> const, 16> const jumpTable
    {
        std::bind(&Isoline::case0,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case1,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case2,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case3,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case4,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case5,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case6,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case7,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case8,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case9,  this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case10, this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case11, this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case12, this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case13, this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case14, this, _1, _2, _3, _4, _5),
        std::bind(&Isoline::case15, this, _1, _2, _3, _4, _5)
    };

    std::array<std::function<void(QVector2D const &bottomLeft)> const, 16> const jumpTableNonInterpolated
    {
        std::bind(&Isoline::case0NonInterpolated,  this, _1),
        std::bind(&Isoline::case1NonInterpolated,  this, _1),
        std::bind(&Isoline::case2NonInterpolated,  this, _1),
        std::bind(&Isoline::case3NonInterpolated,  this, _1),
        std::bind(&Isoline::case4NonInterpolated,  this, _1),
        std::bind(&Isoline::case5NonInterpolated,  this, _1),
        std::bind(&Isoline::case6NonInterpolated,  this, _1),
        std::bind(&Isoline::case7NonInterpolated,  this, _1),
        std::bind(&Isoline::case8NonInterpolated,  this, _1),
        std::bind(&Isoline::case9NonInterpolated,  this, _1),
        std::bind(&Isoline::case10NonInterpolated, this, _1),
        std::bind(&Isoline::case11NonInterpolated, this, _1),
        std::bind(&Isoline::case12NonInterpolated, this, _1),
        std::bind(&Isoline::case13NonInterpolated, this, _1),
        std::bind(&Isoline::case14NonInterpolated, this, _1),
        std::bind(&Isoline::case15NonInterpolated, this, _1)
    };

public:
    enum class InterpolationMethod
    {
        Linear,
        None
    };

    Isoline(std::vector<float> const &values,
            size_t const valuesSideSize,
            float const isolineRho,
            float const cellSideLength,
            InterpolationMethod const interpolationMethod);

    std::vector<QVector2D> vertices() const;
};

#endif // ISOLINE_H
