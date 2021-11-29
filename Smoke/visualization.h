#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "color.h"
#include "datatype.h"
#include "isoline.h"
#include "movingaverage.h"
#include "simulation.h"
#include "texture.h"

#include <QOpenGLWidget>
#include <QTimer>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

#include <array>
#include <deque>
#include <utility>
#include <vector>

class Visualization : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

    friend class MainWindow;

    enum class MappingType
    {
        Scaling,
        Clamping
    };

    enum class SlicingDirection
    {
        x,
        y,
        t
    };

    QTimer m_timer;
    QOpenGLDebugLogger m_debugLogger;

    //--- VISUALIZATION PARAMETERS ---------------------------------------------------------------------
    bool m_isRunning = true;        // Toggles on/off the animation.
    bool m_sendMinMaxToUI = true;   // Show the min/max values of the scalar data.
    bool m_drawScalarData = true;   // Draw the smoke or not.
    bool m_drawIsolines = false;    // Draw isolines or not.
    bool m_drawHeightplot = false;  // Draw height plot or not.
    size_t m_DIM = 64U;             // Size of simulation grid. Must be even.

    float m_cellWidth;		        // Grid cell width
    float m_cellHeight;      		// Grid cell height

    Simulation m_simulation{m_DIM};

    // Scalar info
    ScalarDataType m_currentScalarDataType = ScalarDataType::Density;
    MappingType m_currentMappingType = MappingType::Clamping;
    float m_scalarDataScale = 0.1F;
    float m_clampMin = 0.0F;                                              // Minimum density we want to visualize.
    float m_clampMax = static_cast<float>(m_simulation.rhoInjected());    // Maximum density we want to visualize.
    float m_transferK = 1.0F;                                             // The K in the transfer function: t(f) = f^K.

    // Custom color map. Only used for scalar data
    bool m_useCustomColorMap = false;
    std::array<Color, 3> m_customColors{Color{1.0F, 0.0F, 0.0F}, Color{0.0F, 1.0F, 0.0F}, Color{0.0F, 0.0F, 1.0F}};

    // Isolines info
    ScalarDataType m_currentIsolineDataType = ScalarDataType::Density;
    bool m_manuallyChooseIsolineDataType = false;
    size_t m_numberOfIsolines = 1U;
    Isoline::InterpolationMethod m_isolineInterpolationMethod = Isoline::InterpolationMethod::Linear;
    float m_isolineMinValue = 0.0F;
    float m_isolineMaxValue = 10.0F;
    QVector3D m_isolineColor{1.0F, 1.0F, 1.0F};

    // Height plot info
    ScalarDataType m_currentHeightplotDataType = ScalarDataType::Density;
    QVector3D m_rotation{45.0F, 0.0F, 0.0F};

    // Functions
    std::vector<QVector3D> computeNormals(std::vector<float> height);

    void drag(int const mx, int my);

    // OpenGL related members
    GLuint m_vaoScalarData;
    GLuint m_vboScalarPoints;
    GLuint m_vboScalarData;
    GLuint m_eboScalarData;

    GLuint m_vaoIsolines;
    GLuint m_vboIsolines;
    GLuint m_isolinesTextureLocation;

    GLuint m_vaoHeightplot;
    GLuint m_vboHeightplotPoints;
    GLuint m_vboHeightplotScalarValues;
    GLuint m_vboHeightplotHeight;
    GLuint m_vboHeightplotNormals;
    GLuint m_eboHeightplot;

    QOpenGLShaderProgram m_shaderProgramScalarDataScaleTexture;
    QOpenGLShaderProgram m_shaderProgramScalarDataScaleCustomColorMap;
    QOpenGLShaderProgram m_shaderProgramScalarDataClampTexture;
    QOpenGLShaderProgram m_shaderProgramScalarDataClampCustomColorMap;
    QOpenGLShaderProgram m_shaderProgramIsolines;
    QOpenGLShaderProgram m_shaderProgramHeightplotScale;
    QOpenGLShaderProgram m_shaderProgramHeightplotClamp;

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

    GLint m_uniformLocationIsolines_projection;
    GLint m_uniformLocationIsolines_color;

    GLint m_uniformLocationHeightplotScale_rangeMin;
    GLint m_uniformLocationHeightplotScale_rangeMax;
    GLint m_uniformLocationHeightplotScale_transferK;
    GLint m_uniformLocationHeightplotScale_projection;
    GLint m_uniformLocationHeightplotScale_view;
    GLint m_uniformLocationHeightplotScale_normal;
    GLint m_uniformLocationHeightplotScale_material;
    GLint m_uniformLocationHeightplotScale_light;
    GLint m_uniformLocationHeightplotScale_texture;

    GLint m_uniformLocationHeightplotClamp_clampMin;
    GLint m_uniformLocationHeightplotClamp_clampMax;
    GLint m_uniformLocationHeightplotClamp_transferK;
    GLint m_uniformLocationHeightplotClamp_projection;
    GLint m_uniformLocationHeightplotClamp_view;
    GLint m_uniformLocationHeightplotClamp_normal;
    GLint m_uniformLocationHeightplotClamp_material;
    GLint m_uniformLocationHeightplotClamp_light;
    GLint m_uniformLocationHeightplotClamp_texture;

    GLuint m_scalarDataTextureLocation;
    GLuint m_vectorDataTextureLocation;

    QMatrix4x4 m_projectionTransformationMatrix;
    QMatrix4x4 m_viewTransformationMatrix;

    QMatrix3x3 m_normalTransformationMatrix;

    QVector4D m_materialConstants{0.5f, 0.5f, 1.0f, 5.0f};
    QVector3D m_lightPosition{300.0F, 300.0F, 200.0F};

    MovingAverage<QVector2D> m_minMaxDensity{60, {0.0f, 0.0f}};

    void applyPreprocessing(std::vector<float> &scalarValues);

    // Quantization
    bool m_useQuantization = false;
    unsigned int m_quantizationBits = 2U;
    void applyQuantization(std::vector<float> &scalarValues);

    // Gaussian blur
    bool m_useGaussianBlur = false;
    void convolute(std::vector<float> &input, std::vector<std::vector<float>> &kernel);
    void applyGaussianBlur(std::vector<float> &scalarValues);

    // Gradients
    bool m_useGradients = false;
    void applyGradients(std::vector<float> &scalarValues);
    std::vector<QVector3D> hsv2rgb(std::vector<QVector3D> c);

    // Slicing
    bool m_useSlicing = false;
    size_t const m_slicingWindowSize = m_DIM;
    SlicingDirection m_slicingDirection = SlicingDirection::x;
    size_t m_sliceIdx = 0U;
    std::deque<std::vector<float>> m_scalarValuesWindow{m_slicingWindowSize, std::vector<float>(m_DIM * m_DIM, 0.0F)};

    void applySlicing(std::vector<float> &scalarValues);


    // Indices used in OpenGL indexed rendering
    std::vector<unsigned short> m_indices;


    // OpenGL related functions
    void createShaderProgramScalarDataScaleTexture();
    void createShaderProgramScalarDataScaleCustomColorMap();
    void createShaderProgramScalarDataClampTexture();
    void createShaderProgramScalarDataClampCustomColorMap();
    void createShaderProgramColorMapInstanced();
    void createShaderProgramIsolines();
    void createShaderProgramHeightplotScale();
    void createShaderProgramHeightplotClamp();

    void loadScalarDataTexture(std::vector<Color> const &colorMap);

    void setupAllBuffers();
    void setupScalarData();
    void updateScalarPoints();
    void drawScalarData();

    void setupIsolines();
    void drawIsolines();

    void setupHeightplot();
    void drawHeightplot();
    void rotateView();

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
};

#endif // VISUALIZATION_H
