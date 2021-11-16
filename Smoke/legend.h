#ifndef LEGEND_H
#define LEGEND_H

#include "color.h"

#include <QOpenGLWidget>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

class Legend : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

    struct Vertex
    {
        float x, y;
        float value;
    };

    QOpenGLDebugLogger m_debugLogger;

    GLuint m_vaoLegend;
    GLuint m_vboLegend;
    GLuint m_eboLegend;
    GLuint m_textureLocation;
    GLint m_uniformLocationTexture;

    QOpenGLShaderProgram m_shaderProgramColorMap;

    // The legend is a rectangle, drawn as two triangles.
    static std::array<Vertex, 4> constexpr m_vertices{Vertex{-1.0f,  1.0f, 0.0f},  // Top left.
                                                      Vertex{-1.0f, -1.0f, 0.0f},  // Bottom left.
                                                      Vertex{ 1.0f,  1.0f, 1.0f},  // Top right.
                                                      Vertex{ 1.0f, -1.0f, 1.0f}}; // Bottom right.
    static std::array<unsigned short, 6> constexpr m_indices{0, 1, 2,
                                                             1, 3, 2};

    // OpenGL related functions
    void createShaderProgram();
    void loadInitialColorMap();

protected:
    void initializeGL();
    void resizeGL(int const newWidth, int const newHeight);
    void paintGL();

private slots:
    void onMessageLogged(QOpenGLDebugMessage const &Message) const;

public:
    Legend(QWidget *parent = nullptr);
    ~Legend();

    void updateColorMap(std::vector<Color> const &colorMap);
};

#endif // LEGEND_H
