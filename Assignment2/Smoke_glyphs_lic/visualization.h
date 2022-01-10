#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "color.h"
#include "datatype.h"
#include "glyph.h"
#include "movingaverage.h"
#include "simulation.h"
#include "texture.h"
#include "lic.h"

#include <QOpenGLWidget>
#include <QTimer>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

#include <array>
#include <deque>
#include <utility>

class Visualization : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

    friend class MainWindow;

    enum class ProjectionType
    {
        Orthographic,
        Perspective
    };

    enum class MappingType
    {
        Scaling,
        Clamping
    };

    QTimer m_timer;
    QOpenGLDebugLogger m_debugLogger;

    //--- VISUALIZATION PARAMETERS ---------------------------------------------------------------------
    bool m_isRunning = true;        // Toggles on/off the animation.
    bool m_sendMinMaxToUI = true;   // Show the min/max values of the scalar data and vector data.
    bool m_drawScalarData = true;   // Draw the smoke or not.
    bool m_drawVectorData = false;  // Draw the vector field or not.
    bool m_drawLIC = false;         // Draw LIC or not.
    size_t m_DIM = 64U;             // Size of simulation grid. Must be even.

    float m_cellWidth;		        // Grid cell width
    float m_cellHeight;      		// Grid cell height

    Simulation m_simulation{m_DIM};

    // Scalar info
    ScalarDataType m_currentScalarDataType = ScalarDataType::Density;
    MappingType m_currentMappingType = MappingType::Clamping;
    float m_scalarDataScale = 0.1F;
    float m_clampMin = 0.0F;                                              // Minimum density we want to visualize.
    float m_clampMax = 1.0F;                                              // Maximum density we want to visualize.
    float m_transferK = 1.0F;                                             // The K in the transfer function: t(f) = f^K.

    // Custom color map. Only used for scalar data
    bool m_useCustomColorMap = false;
    std::array<Color, 3> m_customColors{Color{1.0F, 0.0F, 0.0F}, Color{0.0F, 1.0F, 0.0F}, Color{0.0F, 0.0F, 1.0F}};

    // Vector info
    VectorDataType m_currentVectorDataType = VectorDataType::Velocity;
    float m_vectorDataMagnifier = 50.0F;

    // Glyph info
    Glyph::GlyphType m_currentGlyphType = Glyph::GlyphType::Arrow2D;    // Current glyph type in use.
    size_t m_glyphIndicesSize;											// Number of indices of current glyph type.
    size_t m_numberOfGlyphsX = 50U;                                     // Number of glyphs in x-direction.
    size_t m_numberOfGlyphsY = 50U;                                     // Number of glyphs in y-direction.
    float m_glyphCellWidth;
    float m_glyphCellHeight;
    float m_vec_scale = 1000.0F;    									// Glyph scaling factor.

    // LIC info
    Lic m_licObject = Lic(256, 256);

    void drag(int const mx, int my);

    // OpenGL related members
    GLuint m_vaoScalarData;
    GLuint m_vboScalarPoints;
    GLuint m_vboScalarData;
    GLuint m_eboScalarData;

    GLuint m_vaoGlyphs;
    GLuint m_vboGlyphs;
    GLuint m_eboGlyphs;
    GLuint m_vboModelTransformationMatricesGlyphs;
    GLuint m_vboValuesGlyphs;

    GLuint m_vaoLic;
    GLuint m_vboLic;
    GLuint m_licTextureLocation;

    QOpenGLShaderProgram m_shaderProgramScalarDataScaleTexture;
    QOpenGLShaderProgram m_shaderProgramScalarDataScaleCustomColorMap;
    QOpenGLShaderProgram m_shaderProgramScalarDataClampTexture;
    QOpenGLShaderProgram m_shaderProgramScalarDataClampCustomColorMap;
    QOpenGLShaderProgram m_shaderProgramVectorData;
    QOpenGLShaderProgram m_shaderProgramLic;

    GLint m_uniformLocationScalarDataScaleTexture_rangeMin;
    GLint m_uniformLocationScalarDataScaleTexture_rangeMax;
    GLint m_uniformLocationScalarDataScaleTexture_transferK;
    GLint m_uniformLocationScalarDataScaleTexture_projection;
    GLint m_uniformLocationScalarDataScaleTexture_texture;

    GLint m_uniformLocationScalarDataScaleCustomColorMap_rangeMin;
    GLint m_uniformLocationScalarDataScaleCustomColorMap_rangeMax;
    GLint m_uniformLocationScalarDataScaleCustomColorMap_transferK;
    GLint m_uniformLocationScalarDataScaleCustomColorMap_projection;
    GLint m_uniformLocationScalarDataScaleCustomColorMap_colorMapColors;

    GLint m_uniformLocationScalarDataClampTexture_clampMin;
    GLint m_uniformLocationScalarDataClampTexture_clampMax;
    GLint m_uniformLocationScalarDataClampTexture_transferK;
    GLint m_uniformLocationScalarDataClampTexture_projection;
    GLint m_uniformLocationScalarDataClampTexture_texture;

    GLint m_uniformLocationScalarDataClampCustomColorMap_clampMin;
    GLint m_uniformLocationScalarDataClampCustomColorMap_clampMax;
    GLint m_uniformLocationScalarDataClampCustomColorMap_transferK;
    GLint m_uniformLocationScalarDataClampCustomColorMap_projection;
    GLint m_uniformLocationScalarDataClampCustomColorMap_colorMapColors;

    GLint m_uniformLocationProjectionColorMapInstanced;
    GLint m_uniformLocationTextureColorMapInstanced;

    GLuint m_scalarDataTextureLocation;
    GLuint m_vectorDataTextureLocation;

    GLint m_uniformLocationLic_projection;

    GLint m_uniformLocationLicTexture;

    QMatrix4x4 m_projectionTransformationMatrix;
    QMatrix4x4 m_viewTransformationMatrix;

    QMatrix3x3 m_normalTransformationMatrix;

    MovingAverage<QVector2D> m_minMaxDensity{60, {0.0F, 0.0F}};

    // Indices used in OpenGL indexed rendering
    std::vector<unsigned short> m_indices;

    // OpenGL related functions
    void createShaderProgramScalarDataScaleTexture();
    void createShaderProgramScalarDataScaleCustomColorMap();
    void createShaderProgramScalarDataClampTexture();
    void createShaderProgramScalarDataClampCustomColorMap();
    void createShaderProgramColorMapInstanced();
    void createShaderProgramLic();

    void loadScalarDataTexture(std::vector<Color> const &colorMap);
    void loadVectorDataTexture(std::vector<Color> const &colorMap);
    void loadLicTexture(std::vector<uint8_t> const &licTexture);

    void setupAllBuffers();
    void setupScalarData();
    void updateScalarPoints();
    void drawScalarData();

    void setupGlyphs();
    void bufferSingleGlyph();
    void setupGlyphsPerInstanceData();
    void drawGlyphs();

    void setupLic();
    void updateLicPoints();
    void drawLic();

protected:
    void initializeGL();
    void resizeGL(int const newWidth, int const newHeight);
    void paintGL();

    void mouseMoveEvent(QMouseEvent *ev);

private slots:
    void onMessageLogged(QOpenGLDebugMessage const &Message) const;

public slots:
    void do_one_simulation_step();

public:
    Visualization(QWidget *parent = nullptr);
    ~Visualization();

    // Setters
    void setDIM(size_t const DIM);

    void setNumberOfGlyphsX(size_t const numberOfGlyphsX);
    void setNumberOfGlyphsY(size_t const numberOfGlyphsY);
};

#endif // VISUALIZATION_H
