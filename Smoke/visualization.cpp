#include "visualization.h"

#include "constants.h"
#include "isoline.h"
#include "mainwindow.h"
#include "texture.h"

#include <fftw3.h>

#include <QDebug>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

Visualization::Visualization(QWidget *parent) : QOpenGLWidget(parent)
{
    qDebug() << "Visualization constructor";

    // Start the simulation loop.
    m_timer.start(17); // Each frame takes 17ms, making the simulation run at approximately 60 FPS
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(do_one_simulation_step()));
}

Visualization::~Visualization()
{
    makeCurrent();

    qDebug() << "In Visualization destructor";

    glDeleteVertexArrays(1, &m_vaoScalarData);
    glDeleteBuffers(1, &m_vboScalarPoints);
    glDeleteBuffers(1, &m_vboScalarData);
    glDeleteBuffers(1, &m_eboScalarData);

    glDeleteVertexArrays(1, &m_vaoIsolines);
    glDeleteBuffers(1, &m_vboIsolines);

    glDeleteVertexArrays(1, &m_vaoHeightplot);
    glDeleteBuffers(1, &m_vboHeightplotPoints);
    glDeleteBuffers(1, &m_vboHeightplotHeight);
    glDeleteBuffers(1, &m_vboHeightplotScalarValues);
    glDeleteBuffers(1, &m_vboHeightplotNormals);
    glDeleteBuffers(1, &m_eboHeightplot);

    glDeleteTextures(1, &m_scalarDataTextureLocation);
    glDeleteTextures(1, &m_vectorDataTextureLocation);
}

void Visualization::do_one_simulation_step()
{
    if (m_isRunning)
        m_simulation.do_one_simulation_step();

    update();
}

void Visualization::initializeGL() {
    qDebug() << ":: Initializing OpenGL";
    initializeOpenGLFunctions();

    connect(&m_debugLogger, SIGNAL(messageLogged(QOpenGLDebugMessage)),
            this, SLOT(onMessageLogged(QOpenGLDebugMessage)), Qt::DirectConnection);

    if (m_debugLogger.initialize())
    {
        qDebug() << ":: Logging initialized";
        m_debugLogger.startLogging( QOpenGLDebugLogger::SynchronousLogging );
        m_debugLogger.enableMessages();
    }

    {
        QString const glVersion{reinterpret_cast<const char*>(glGetString(GL_VERSION))};
        qDebug() << ":: Using OpenGL" << qPrintable(glVersion);
    }

    glClearColor(0.2F, 0.1F, 0.2F, 1.0F);

    createShaderProgramScalarDataScaleTexture();
    createShaderProgramScalarDataScaleCustomColorMap();
    createShaderProgramScalarDataClampTexture();
    createShaderProgramScalarDataClampCustomColorMap();
    createShaderProgramIsolines();
    createShaderProgramHeightplotScale();
    createShaderProgramHeightplotClamp();

    // Retrieve default textures.
    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
    std::vector<Color> const defaultScalarDataColorMap = mainWindowPtr->m_defaultScalarDataColorMap;
    std::vector<Color> const defaultVectorDataColorMap = mainWindowPtr->m_defaultVectorDataColorMap;

    // Generate buffers.
    glGenVertexArrays(1, &m_vaoScalarData);
    glGenBuffers(1, &m_vboScalarPoints);
    glGenBuffers(1, &m_vboScalarData);
    glGenBuffers(1, &m_eboScalarData);
    glGenTextures(1, &m_scalarDataTextureLocation);

    glGenVertexArrays(1, &m_vaoIsolines);
    glGenBuffers(1, &m_vboIsolines);
    glGenTextures(1, &m_isolinesTextureLocation);

    glGenVertexArrays(1, &m_vaoHeightplot);
    glGenBuffers(1, &m_vboHeightplotPoints);
    glGenBuffers(1, &m_vboHeightplotScalarValues);
    glGenBuffers(1, &m_vboHeightplotHeight);
    glGenBuffers(1, &m_vboHeightplotNormals);
    glGenBuffers(1, &m_eboHeightplot);

    setupAllBuffers();

    loadScalarDataTexture(defaultScalarDataColorMap);

    rotateView();
    m_normalTransformationMatrix.setToIdentity();
}

void Visualization::setupAllBuffers()
{
    setupScalarData();
    setupIsolines();
    setupHeightplot();
}

void Visualization::setupScalarData()
{
    glBindVertexArray(m_vaoScalarData);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboScalarPoints);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * 2 * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_vboScalarData);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    size_t const numberOfTriangleStripIndices = (m_DIM - 1) * (2 * m_DIM + 2) - 2;
    m_indices.reserve(numberOfTriangleStripIndices);

    for (unsigned short stripIdx = 0; stripIdx < (m_DIM * (m_DIM - 1)); stripIdx += m_DIM)
    {
        unsigned short lastUsedIdx;
        for (unsigned short idx = stripIdx; idx < (stripIdx + m_DIM); ++idx)
        {
            m_indices.push_back(idx);

            lastUsedIdx = static_cast<unsigned short>(idx + m_DIM);
            m_indices.push_back(lastUsedIdx);
        }

        // Add degenerate vertices to start rendering the next strip without requiring a new (expensive) draw call.
        // Note: there's no special case for the last triangle, so a couple of redundant indices are added.
        m_indices.push_back(lastUsedIdx); // Repeat last added vertex.
        m_indices.push_back(static_cast<unsigned short>(stripIdx + m_DIM)); // Add first vertex of next strip, so that it will appear twice.
    }

    // No primitive restart, so the last (degenerated) triangles can be removed.
    m_indices.erase(m_indices.end() - 2, m_indices.end());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboScalarData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned short)),
                 m_indices.data(),
                 GL_STATIC_DRAW);
}

void Visualization::setupIsolines()
{
    glBindVertexArray(m_vaoIsolines);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboIsolines);

    // Set vertex coordinates to location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), reinterpret_cast<GLvoid*>(0));

    // Set height to location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(QVector3D), reinterpret_cast<GLvoid*>(2 * sizeof(float)));
}

void Visualization::setupHeightplot()
{
    glBindVertexArray(m_vaoHeightplot);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotPoints);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * 2 * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotHeight);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotScalarValues);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotNormals);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * 3 * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboHeightplot);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned short)),
                 m_indices.data(),
                 GL_STATIC_DRAW);
}

void Visualization::createShaderProgramScalarDataScaleTexture()
{
    m_shaderProgramScalarDataScaleTexture.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/scalarData_scale.vert");
    m_shaderProgramScalarDataScaleTexture.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/scalarData_texture.frag");
    m_shaderProgramScalarDataScaleTexture.link();

    m_uniformLocationScalarDataScaleTexture_projection = m_shaderProgramScalarDataScaleTexture.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationScalarDataScaleTexture_projection != -1);
    m_uniformLocationScalarDataScaleTexture_texture = m_shaderProgramScalarDataScaleTexture.uniformLocation("textureSampler");
    Q_ASSERT(m_uniformLocationScalarDataScaleTexture_texture != -1);

    m_uniformLocationScalarDataScaleTexture_rangeMin = m_shaderProgramScalarDataScaleTexture.uniformLocation("rangeMin");
    Q_ASSERT(m_uniformLocationScalarDataScaleTexture_rangeMin != -1);
    m_uniformLocationScalarDataScaleTexture_rangeMax = m_shaderProgramScalarDataScaleTexture.uniformLocation("rangeMax");
    Q_ASSERT(m_uniformLocationScalarDataScaleTexture_rangeMax != -1);
    m_uniformLocationScalarDataScaleTexture_transferK = m_shaderProgramScalarDataScaleTexture.uniformLocation("transferK");
    Q_ASSERT(m_uniformLocationScalarDataScaleTexture_transferK != -1);

    qDebug() << "m_shaderProgramScalarDataScaleTexture initialized.";
}

void Visualization::createShaderProgramScalarDataScaleCustomColorMap()
{
    m_shaderProgramScalarDataScaleCustomColorMap.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/scalarData_scale.vert");
    m_shaderProgramScalarDataScaleCustomColorMap.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/scalarData_customcolormap.frag");
    m_shaderProgramScalarDataScaleCustomColorMap.link();

    m_uniformLocationScalarDataScaleCustomColorMap_projection = m_shaderProgramScalarDataScaleCustomColorMap.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationScalarDataScaleCustomColorMap_projection != -1);

    m_uniformLocationScalarDataScaleCustomColorMap_colorMapColors = m_shaderProgramScalarDataScaleCustomColorMap.uniformLocation("colorMapColors");
    Q_ASSERT(m_uniformLocationScalarDataScaleCustomColorMap_colorMapColors != -1);

    m_uniformLocationScalarDataScaleCustomColorMap_rangeMin = m_shaderProgramScalarDataScaleCustomColorMap.uniformLocation("rangeMin");
    Q_ASSERT(m_uniformLocationScalarDataScaleCustomColorMap_rangeMin != -1);
    m_uniformLocationScalarDataScaleCustomColorMap_rangeMax = m_shaderProgramScalarDataScaleCustomColorMap.uniformLocation("rangeMax");
    Q_ASSERT(m_uniformLocationScalarDataScaleCustomColorMap_rangeMax != -1);
    m_uniformLocationScalarDataScaleCustomColorMap_transferK = m_shaderProgramScalarDataScaleCustomColorMap.uniformLocation("transferK");
    Q_ASSERT(m_uniformLocationScalarDataScaleCustomColorMap_transferK != -1);

    qDebug() << "m_shaderProgramScalarDataScaleCustomColorMap initialized.";
}

void Visualization::createShaderProgramScalarDataClampTexture()
{
    m_shaderProgramScalarDataClampTexture.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/scalarData_clamp.vert");
    m_shaderProgramScalarDataClampTexture.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/scalarData_texture.frag");
    m_shaderProgramScalarDataClampTexture.link();

    m_uniformLocationScalarDataClampTexture_projection = m_shaderProgramScalarDataClampTexture.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationScalarDataClampTexture_projection != -1);
    m_uniformLocationScalarDataClampTexture_texture = m_shaderProgramScalarDataClampTexture.uniformLocation("textureSampler");
    Q_ASSERT(m_uniformLocationScalarDataClampTexture_texture != -1);

    m_uniformLocationScalarDataClampTexture_clampMin = m_shaderProgramScalarDataClampTexture.uniformLocation("clampMin");
    Q_ASSERT(m_uniformLocationScalarDataClampTexture_clampMin != -1);
    m_uniformLocationScalarDataClampTexture_clampMax = m_shaderProgramScalarDataClampTexture.uniformLocation("clampMax");
    Q_ASSERT(m_uniformLocationScalarDataClampTexture_clampMax != -1);
    m_uniformLocationScalarDataClampTexture_transferK = m_shaderProgramScalarDataClampTexture.uniformLocation("transferK");
    Q_ASSERT(m_uniformLocationScalarDataClampTexture_transferK != -1);

    qDebug() << "m_shaderProgramScalarDataClampTexture initialized.";
}

void Visualization::createShaderProgramScalarDataClampCustomColorMap()
{
    m_shaderProgramScalarDataClampCustomColorMap.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/scalarData_clamp.vert");
    m_shaderProgramScalarDataClampCustomColorMap.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/scalarData_customcolormap.frag");
    m_shaderProgramScalarDataClampCustomColorMap.link();

    m_uniformLocationScalarDataClampCustomColorMap_projection = m_shaderProgramScalarDataClampCustomColorMap.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationScalarDataClampCustomColorMap_projection != -1);

    m_uniformLocationScalarDataClampCustomColorMap_colorMapColors = m_shaderProgramScalarDataClampCustomColorMap.uniformLocation("colorMapColors");
    Q_ASSERT(m_uniformLocationScalarDataClampCustomColorMap_colorMapColors != -1);

    m_uniformLocationScalarDataClampCustomColorMap_clampMin = m_shaderProgramScalarDataClampCustomColorMap.uniformLocation("clampMin");
    Q_ASSERT(m_uniformLocationScalarDataClampCustomColorMap_clampMin != -1);
    m_uniformLocationScalarDataClampCustomColorMap_clampMax = m_shaderProgramScalarDataClampCustomColorMap.uniformLocation("clampMax");
    Q_ASSERT(m_uniformLocationScalarDataClampCustomColorMap_clampMax != -1);
    m_uniformLocationScalarDataClampCustomColorMap_transferK = m_shaderProgramScalarDataClampCustomColorMap.uniformLocation("transferK");
    Q_ASSERT(m_uniformLocationScalarDataClampCustomColorMap_transferK != -1);

    qDebug() << "m_shaderProgramScalarDataClampCustomColorMap initialized.";
}

void Visualization::createShaderProgramIsolines()
{
    m_shaderProgramIsolines.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/isolines.vert");
    m_shaderProgramIsolines.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/isolines.frag");
    m_shaderProgramIsolines.link();

    m_uniformLocationIsolines_projection = m_shaderProgramIsolines.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationIsolines_projection != -1);
    m_uniformLocationIsolines_color = m_shaderProgramIsolines.uniformLocation("isolineColor");
    Q_ASSERT(m_uniformLocationIsolines_color != -1);

    qDebug() << "m_shaderProgramIsolines initialized.";
}

void Visualization::createShaderProgramHeightplotScale()
{
    m_shaderProgramHeightplotScale.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/heightplot_scale.vert");
    m_shaderProgramHeightplotScale.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/heightplot.frag");
    m_shaderProgramHeightplotScale.link();

    m_uniformLocationHeightplotScale_rangeMin = m_shaderProgramHeightplotScale.uniformLocation("rangeMin");
    Q_ASSERT(m_uniformLocationHeightplotScale_rangeMin != -1);
    m_uniformLocationHeightplotScale_rangeMax = m_shaderProgramHeightplotScale.uniformLocation("rangeMax");
    Q_ASSERT(m_uniformLocationHeightplotScale_rangeMax != -1);
    m_uniformLocationHeightplotScale_transferK = m_shaderProgramHeightplotScale.uniformLocation("transferK");
    Q_ASSERT(m_uniformLocationHeightplotScale_transferK != -1);

    m_uniformLocationHeightplotScale_projection = m_shaderProgramHeightplotScale.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationHeightplotScale_projection != -1);
    m_uniformLocationHeightplotScale_view = m_shaderProgramHeightplotScale.uniformLocation("viewTransform");
    Q_ASSERT(m_uniformLocationHeightplotScale_view != -1);
    m_uniformLocationHeightplotScale_normal = m_shaderProgramHeightplotScale.uniformLocation("normalTransform");
    Q_ASSERT(m_uniformLocationHeightplotScale_normal != -1);

    m_uniformLocationHeightplotScale_material = m_shaderProgramHeightplotScale.uniformLocation("material");
    Q_ASSERT(m_uniformLocationHeightplotScale_material != -1);
    m_uniformLocationHeightplotScale_light = m_shaderProgramHeightplotScale.uniformLocation("lightPosition");
    Q_ASSERT(m_uniformLocationHeightplotScale_light != -1);

    m_uniformLocationHeightplotScale_texture = m_shaderProgramHeightplotScale.uniformLocation("textureSampler");
    Q_ASSERT(m_uniformLocationHeightplotScale_texture != -1);

    qDebug() << "m_shaderProgramHeightplotScale initialized.";
}

void Visualization::createShaderProgramHeightplotClamp()
{
    m_shaderProgramHeightplotClamp.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/heightplot_clamp.vert");
    m_shaderProgramHeightplotClamp.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/heightplot.frag");
    m_shaderProgramHeightplotClamp.link();

    m_uniformLocationHeightplotClamp_clampMin = m_shaderProgramHeightplotClamp.uniformLocation("clampMin");
    Q_ASSERT(m_uniformLocationHeightplotClamp_clampMin != -1);
    m_uniformLocationHeightplotClamp_clampMax = m_shaderProgramHeightplotClamp.uniformLocation("clampMax");
    Q_ASSERT(m_uniformLocationHeightplotClamp_clampMax != -1);
    m_uniformLocationHeightplotClamp_transferK = m_shaderProgramHeightplotClamp.uniformLocation("transferK");
    Q_ASSERT(m_uniformLocationHeightplotScale_transferK != -1);

    m_uniformLocationHeightplotClamp_projection = m_shaderProgramHeightplotClamp.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationHeightplotClamp_projection != -1);
    m_uniformLocationHeightplotClamp_view = m_shaderProgramHeightplotClamp.uniformLocation("viewTransform");
    Q_ASSERT(m_uniformLocationHeightplotClamp_view != -1);
    m_uniformLocationHeightplotClamp_normal = m_shaderProgramHeightplotClamp.uniformLocation("normalTransform");
    Q_ASSERT(m_uniformLocationHeightplotClamp_normal != -1);

    m_uniformLocationHeightplotClamp_material = m_shaderProgramHeightplotClamp.uniformLocation("material");
    Q_ASSERT(m_uniformLocationHeightplotClamp_material != -1);
    m_uniformLocationHeightplotClamp_light = m_shaderProgramHeightplotClamp.uniformLocation("lightPosition");
    Q_ASSERT(m_uniformLocationHeightplotClamp_light != -1);

    m_uniformLocationHeightplotClamp_texture = m_shaderProgramHeightplotClamp.uniformLocation("textureSampler");
    Q_ASSERT(m_uniformLocationHeightplotClamp_texture != -1);

    qDebug() << "m_shaderProgramHeightplotClamp initialized.";
}

void Visualization::loadScalarDataTexture(std::vector<Color> const &colorMap)
{
    // Set texture parameters.
    glBindTexture(GL_TEXTURE_1D, m_scalarDataTextureLocation);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage1D(GL_TEXTURE_1D,
                 0,
                 GL_RGB32F,
                 static_cast<GLint>(colorMap.size()),
                 0,
                 GL_RGB,
                 GL_FLOAT,
                 colorMap.data());
}

void Visualization::paintGL()
{
    glBindVertexArray(0);

    // Clear the screen before rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_drawHeightplot)
    {
        drawHeightplot();
        return;
    }

    if (m_drawScalarData)
        drawScalarData();

    if (m_drawIsolines)
    {
        m_shaderProgramIsolines.bind();
        glUniformMatrix4fv(m_uniformLocationIsolines_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());
        glUniform3fv(m_uniformLocationIsolines_color, 1, &m_isolineColor[0]);
        drawIsolines();
    }
}

void Visualization::resizeGL(int const width, int const height)
{
    m_cellWidth  = static_cast<float>(width) / static_cast<float>(m_DIM + 1);
    m_cellHeight = static_cast<float>(height) / static_cast<float>(m_DIM + 1);

    m_projectionTransformationMatrix.setToIdentity();

    m_projectionTransformationMatrix.ortho(0.0F, width,
                       0.0F, height,
                       -50.0F, 50.0F);

    updateScalarPoints();
}

void Visualization::rotateView()
{
    m_viewTransformationMatrix.setToIdentity();
    m_viewTransformationMatrix.rotate(m_rotation.x(), 1.0, 0.0, 0.0);
    m_viewTransformationMatrix.rotate(m_rotation.y(), 0.0, 1.0, 0.0);
    m_viewTransformationMatrix.rotate(m_rotation.z(), 0.0, 0.0, 1.0);
}

void Visualization::updateScalarPoints()
{
    // Recompute and upload grid coordinates.
    std::vector<QVector2D> scalarPoints;
    scalarPoints.reserve(m_DIM * m_DIM);

    for (size_t j = 0; j < m_DIM; ++j)
        for (size_t i = 0; i < m_DIM; ++i)
        {
            auto const iFloat = static_cast<float>(i);
            auto const jFloat = static_cast<float>(j);

            QVector2D const v0{m_cellWidth  + iFloat * m_cellWidth,
                               m_cellHeight + jFloat * m_cellHeight};

            scalarPoints.push_back(v0);
        }

    glBindBuffer(GL_ARRAY_BUFFER, m_vboScalarPoints);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(scalarPoints.size() * 2 * sizeof(float)),
                    scalarPoints.data());

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotPoints);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(scalarPoints.size() * 2 * sizeof(float)),
                    scalarPoints.data());
}

void Visualization::applyQuantization(std::vector<float> &scalarValues)
{
    // Convert the floating point values to (8 bit) unsigned integers,
    // so that the data can be treated as an image.
    // The image's pixel values are in the range [0, 255].
    float const maxValue = *std::max_element(scalarValues.cbegin(), scalarValues.cend());
    std::vector<unsigned int> image;
    image.reserve(scalarValues.size());
    for (auto const x : scalarValues)
        image.push_back(static_cast<unsigned int>(std::lroundf(x / maxValue * 255.0F)));


    // Apply quantization to std::vector<unsigned int> image here.
    // The variable m_quantizationBits ('n' in the lecture slides) is set in the GUI and can be used here.
    // L needs to be set to the appropriate value and will be used to set the clamping range in the GUI.
    // ..

    unsigned int const L = 1U; // placeholder value
    qDebug() << "Quantization not implemented";


    // Convert the image's data back to floating point values, so that it can be processed as usual.
    scalarValues = std::vector<float>{image.cbegin(), image.cend()};

    // Force the clamping range in the GUI to be [0, L].
    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
    Q_ASSERT(mainWindowPtr != nullptr);
    mainWindowPtr->on_scalarDataMappingClampingMaxSlider_valueChanged(0);
    mainWindowPtr->on_scalarDataMappingClampingMaxSlider_valueChanged(100 * static_cast<int>(L));
}

void Visualization::applyGaussianBlur(std::vector<float> &scalarValues)
{
    // Implement Gaussian blur here, applied on the values of the scalarValues container.
    // First, define a 3x3 matrix for the kernel.
    // (Use a C-style 2D array, a std::array of std::array's, or a std::vector of std::vectors)

    std::vector<std::vector<float>> kernel_gauss{{1.0/16,2.0/16,1.0/16}, {2.0/16,4.0/16,2.0/16}, {1.0/16,2.0/16,1.0/16}};
    Visualization::convolute(scalarValues, kernel_gauss);
}

static inline double computeSquare (float x) {
    return x*x;
}

void Visualization::applyGradients(std::vector<float> &scalarValues)
{
    // Implement Gradient extraction here, applied on the values of the scalarValues container.
    // First, define a 3x3 Sobel kernels (for x and y directions).
    // (Use a C-style 2D array, a std::array of std::array's, or a std::vector of std::vectors)
    std::vector<std::vector<float>> kernel_x{{1.0, 0.0, -1.0}, {2.0, 0.0, -2.0}, {1.0, 0.0, -1.0}}; // kernel to detect horizontal edges
    std::vector<std::vector<float>> kernel_y{{1.0, 2.0, 1.0}, {0.0, 0.0, 0.0}, {-1.0, -2.0, -1.0}}; // kernel to detect vertical edges

    // Convolve the values of the scalarValues container with the Sobel kernels
    std::vector<float> output_x(scalarValues); // copy the values of scalarValues into output_x
    std::vector<float> output_y(scalarValues);

    Visualization::convolute(output_x, kernel_x); // store the approximate partial derivatives in the x direction into output_x
    Visualization::convolute(output_y, kernel_y);

    // Calculate the Gradient magnitude
    // mag = sqrt((output_x)^2 + (output_y)^2)
    std::vector<float> mag;
    mag.reserve(output_x.size());
    for (size_t i = 0; i < output_x.size(); ++i)
    {
        float val = computeSquare(output_x[i]) + computeSquare(output_y[i]);
        mag.push_back(val);
    }

    // Calculate the Gradient direction
    // dir = tan-1(output_y/output_x);
    std::vector<float> dir;
    dir.reserve(output_x.size());
    for (size_t i = 0; i < output_x.size(); ++i)
    {
        float val = atan(output_y[i] / output_x[i]);
        dir.push_back(val);
    } // TODO dir currently not used

    // apply the Gradient magnitude to the scalarValues.
    std::copy(mag.begin(), mag.end(), scalarValues.begin());
}

void Visualization::applyPreprocessing(std::vector<float> &scalarValues)
{
    if (m_useQuantization)
        applyQuantization(scalarValues);

    if (m_useGaussianBlur)
        applyGaussianBlur(scalarValues);

    if (m_useGradients)
        applyGradients(scalarValues);

    if (m_useSlicing)
        applySlicing(scalarValues);
}

void Visualization::drawScalarData()
{
    std::vector<float> scalarValues;

    switch (m_currentScalarDataType)
    {
        case ScalarDataType::Density:
            scalarValues = m_simulation.density();
        break;

        case ScalarDataType::ForceFieldMagnitude:
            scalarValues = m_simulation.forceFieldMagnitude();
        break;

        case ScalarDataType::VelocityMagnitude:
            scalarValues = m_simulation.velocityMagnitude();
        break;

        case ScalarDataType::ForceFieldDivergence:
            qDebug() << "Scalar data type ForceFieldDivergence not implemented";
        break;

        case ScalarDataType::VelocityDivergence:
            qDebug() << "Scalar data type VelocityDivergence not implemented";
        break;
    }

    applyPreprocessing(scalarValues);

    switch (m_currentMappingType)
    {
        case MappingType::Scaling:
        {
            if (m_useCustomColorMap)
            {
                m_shaderProgramScalarDataScaleCustomColorMap.bind();
                glUniformMatrix4fv(m_uniformLocationScalarDataScaleCustomColorMap_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());

                auto const currentMinMaxIt = std::minmax_element(scalarValues.cbegin(), scalarValues.cend());
                QVector2D currentMinMax{*currentMinMaxIt.first, *currentMinMaxIt.second};

                m_minMaxDensity.update(currentMinMax);
                QVector2D const minMaxAverage{m_minMaxDensity.average()};

                // Send values to GUI.
                if (m_sendMinMaxToUI)
                {
                    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
                    Q_ASSERT(mainWindowPtr != nullptr);
                    mainWindowPtr->setScalarDataMin(minMaxAverage.x());
                    mainWindowPtr->setScalarDataMax(minMaxAverage.y());
                }

                glUniform1f(m_uniformLocationScalarDataScaleCustomColorMap_rangeMin, minMaxAverage.x());
                glUniform1f(m_uniformLocationScalarDataScaleCustomColorMap_rangeMax, minMaxAverage.y());
                glUniform1f(m_uniformLocationScalarDataScaleCustomColorMap_transferK, m_transferK);

                GLfloat const *ptrToFirstElement = &m_customColors[0].r;
                glUniform3fv(m_uniformLocationScalarDataScaleCustomColorMap_colorMapColors, 3, ptrToFirstElement);
            }
            else
            {
                m_shaderProgramScalarDataScaleTexture.bind();
                glUniformMatrix4fv(m_uniformLocationScalarDataScaleTexture_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());

                auto const currentMinMaxIt = std::minmax_element(scalarValues.cbegin(), scalarValues.cend());
                QVector2D currentMinMax{*currentMinMaxIt.first, *currentMinMaxIt.second};

                m_minMaxDensity.update(currentMinMax);
                QVector2D const minMaxAverage{m_minMaxDensity.average()};

                // Send values to GUI.
                if (m_sendMinMaxToUI)
                {
                    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
                    Q_ASSERT(mainWindowPtr != nullptr);
                    mainWindowPtr->setScalarDataMin(minMaxAverage.x());
                    mainWindowPtr->setScalarDataMax(minMaxAverage.y());
                }

                glUniform1f(m_uniformLocationScalarDataScaleTexture_rangeMin, minMaxAverage.x());
                glUniform1f(m_uniformLocationScalarDataScaleTexture_rangeMax, minMaxAverage.y());
                glUniform1f(m_uniformLocationScalarDataScaleTexture_transferK, m_transferK);

                glUniform1i(m_uniformLocationScalarDataScaleTexture_texture, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_1D, m_scalarDataTextureLocation);
            }
        }
        break;

        case MappingType::Clamping:
        {
            if (m_useCustomColorMap)
            {
                m_shaderProgramScalarDataClampCustomColorMap.bind();
                glUniformMatrix4fv(m_uniformLocationScalarDataClampCustomColorMap_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());

                // Send values to GUI.
                if (m_sendMinMaxToUI)
                {
                    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
                    Q_ASSERT(mainWindowPtr != nullptr);
                    mainWindowPtr->setScalarDataMin(m_clampMin);
                    mainWindowPtr->setScalarDataMax(m_clampMax);
                }

                glUniform1f(m_uniformLocationScalarDataClampCustomColorMap_clampMin, m_clampMin);
                glUniform1f(m_uniformLocationScalarDataClampCustomColorMap_clampMax, m_clampMax);
                glUniform1f(m_uniformLocationScalarDataClampCustomColorMap_transferK, m_transferK);

                GLfloat const *ptrToFirstElement = &m_customColors[0].r;
                glUniform3fv(m_uniformLocationScalarDataClampCustomColorMap_colorMapColors, 3, ptrToFirstElement);
            }
            else
            {
                m_shaderProgramScalarDataClampTexture.bind();
                glUniformMatrix4fv(m_uniformLocationScalarDataClampTexture_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());

                // Send values to GUI.
                if (m_sendMinMaxToUI)
                {
                    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
                    Q_ASSERT(mainWindowPtr != nullptr);
                    mainWindowPtr->setScalarDataMin(m_clampMin);
                    mainWindowPtr->setScalarDataMax(m_clampMax);
                }

                glUniform1f(m_uniformLocationScalarDataClampTexture_clampMin, m_clampMin);
                glUniform1f(m_uniformLocationScalarDataClampTexture_clampMax, m_clampMax);
                glUniform1f(m_uniformLocationScalarDataClampTexture_transferK, m_transferK);

                glUniform1i(m_uniformLocationScalarDataClampTexture_texture, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_1D, m_scalarDataTextureLocation);
            }
        }
        break;
    }

    glBindVertexArray(m_vaoScalarData);

    // Copy scalars to GPU buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vboScalarData);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(scalarValues.size() * sizeof(float)),
                    scalarValues.data());

    glDrawElements(GL_TRIANGLE_STRIP,
                   static_cast<GLsizei>(m_indices.size()),
                   GL_UNSIGNED_SHORT,
                   static_cast<GLvoid*>(nullptr));
}

void Visualization::drawIsolines()
{
       
    float const stepsize = [&]()
    {
        if (m_numberOfIsolines > 1)
            return (m_isolineMaxValue - m_isolineMinValue) / (m_numberOfIsolines - 1);

        return 1.0F;
    }();

    std::vector<float> scalarValues;

    switch (m_manuallyChooseIsolineDataType ? m_currentIsolineDataType : m_currentScalarDataType)
    {
        case ScalarDataType::Density:
            scalarValues = m_simulation.density();
        break;

        case ScalarDataType::ForceFieldMagnitude:
            scalarValues = m_simulation.forceFieldMagnitude();
        break;

        case ScalarDataType::VelocityMagnitude:
            scalarValues = m_simulation.velocityMagnitude();
        break;

        case ScalarDataType::VelocityDivergence:
            qDebug() << "Not implemented";
        break;

        case ScalarDataType::ForceFieldDivergence:
            qDebug() << "Not implemented";
        break;
    }


    for (size_t n = 0U; n < m_numberOfIsolines; ++n)
    {
        float const currentIsolineValue = m_isolineMinValue + (n * stepsize);
        std::vector<QVector2D> const vertices{Isoline(scalarValues,
                                                      m_DIM,
                                                      currentIsolineValue,
                                                      m_cellWidth,
                                                      m_isolineInterpolationMethod).vertices()};

        std::vector<QVector3D> isolineVertices;
        isolineVertices.reserve(vertices.size());
        for (auto const &v : vertices)
            isolineVertices.emplace_back(v, 0.0F);

        glBindVertexArray(m_vaoIsolines);


        // buffer data and draw lines
        glBindBuffer(GL_ARRAY_BUFFER, m_vboIsolines);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(isolineVertices.size() * sizeof(QVector3D)),
                     isolineVertices.data(),
                     GL_DYNAMIC_DRAW);

	
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(isolineVertices.size()));

    }
}

void Visualization::drawHeightplot()
{
    std::vector<float> scalarValues;
    std::vector<float> heightValues;

    switch (m_currentScalarDataType)
    {
        case ScalarDataType::Density:
            scalarValues = m_simulation.density();
        break;

        case ScalarDataType::ForceFieldMagnitude:
            scalarValues = m_simulation.forceFieldMagnitude();
        break;

        case ScalarDataType::VelocityMagnitude:
            scalarValues = m_simulation.velocityMagnitude();
        break;

        case ScalarDataType::VelocityDivergence:
            qDebug() << "Not implemented";
        break;

        case ScalarDataType::ForceFieldDivergence:
            qDebug() << "Not implemented";
        break;
    }

    switch (m_currentHeightplotDataType)
    {
        case ScalarDataType::Density:
            heightValues = m_simulation.density();
        break;

        case ScalarDataType::ForceFieldMagnitude:
            heightValues = m_simulation.forceFieldMagnitude();
        break;

        case ScalarDataType::VelocityMagnitude:
            heightValues = m_simulation.velocityMagnitude();
        break;

        case ScalarDataType::VelocityDivergence:
            qDebug() << "Not implemented";
        break;

        case ScalarDataType::ForceFieldDivergence:
            qDebug() << "Not implemented";
        break;
    }

    switch (m_currentMappingType)
    {
        case MappingType::Scaling:
        {
            m_shaderProgramHeightplotScale.bind();
            glUniformMatrix4fv(m_uniformLocationHeightplotScale_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());
            glUniformMatrix4fv(m_uniformLocationHeightplotScale_view, 1, GL_FALSE, m_viewTransformationMatrix.data());
            glUniformMatrix3fv(m_uniformLocationHeightplotScale_normal, 1, GL_FALSE, m_normalTransformationMatrix.data());

            glUniform4fv(m_uniformLocationHeightplotScale_material, 1, &m_materialConstants[0]);
            glUniform3fv(m_uniformLocationHeightplotScale_light, 1, &m_lightPosition[0]);

            auto const currentMinMaxIt = std::minmax_element(scalarValues.cbegin(), scalarValues.cend());
            QVector2D currentMinMax{*currentMinMaxIt.first, *currentMinMaxIt.second};

            m_minMaxDensity.update(currentMinMax);
            QVector2D const minMaxAverage{m_minMaxDensity.average()};

            // Send values to GUI.
            if (m_sendMinMaxToUI)
            {
                auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
                Q_ASSERT(mainWindowPtr != nullptr);
                mainWindowPtr->setScalarDataMin(minMaxAverage.x());
                mainWindowPtr->setScalarDataMax(minMaxAverage.y());
            }

            glUniform1f(m_uniformLocationHeightplotScale_rangeMin, minMaxAverage.x());
            glUniform1f(m_uniformLocationHeightplotScale_rangeMax, minMaxAverage.y());
            glUniform1f(m_uniformLocationHeightplotScale_transferK, m_transferK);

            glUniform1i(m_uniformLocationHeightplotScale_texture, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_1D, m_scalarDataTextureLocation);
        }
        break;

        case MappingType::Clamping:
        {
            m_shaderProgramHeightplotClamp.bind();
            glUniformMatrix4fv(m_uniformLocationHeightplotClamp_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());
            glUniformMatrix4fv(m_uniformLocationHeightplotClamp_view, 1, GL_FALSE, m_viewTransformationMatrix.data());
            glUniformMatrix3fv(m_uniformLocationHeightplotClamp_normal, 1, GL_FALSE, m_normalTransformationMatrix.data());

            glUniform4fv(m_uniformLocationHeightplotClamp_material, 1, &m_materialConstants[0]);
            glUniform3fv(m_uniformLocationHeightplotClamp_light, 1, &m_lightPosition[0]);

            // Send values to GUI.
            auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
            Q_ASSERT(mainWindowPtr != nullptr);
            mainWindowPtr->setScalarDataMin(m_clampMin);
            mainWindowPtr->setScalarDataMax(m_clampMax);

            glUniform1f(m_uniformLocationHeightplotClamp_clampMin, m_clampMin);
            glUniform1f(m_uniformLocationHeightplotClamp_clampMax, m_clampMax);
            glUniform1f(m_uniformLocationHeightplotClamp_transferK, m_transferK);

            glUniform1i(m_uniformLocationHeightplotClamp_texture, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_1D, m_scalarDataTextureLocation);
        }
        break;
    }

    // added some scaling for nicer results
    for(auto & e : heightValues)
      e *= 3.;

    std::vector<QVector3D> normals = computeNormals(heightValues);

    glBindVertexArray(m_vaoHeightplot);

    // Copy scalars to GPU buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotScalarValues);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(scalarValues.size() * sizeof(float)),
                    scalarValues.data());

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotHeight);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(heightValues.size() * sizeof(float)),
                    heightValues.data());

    glBindBuffer(GL_ARRAY_BUFFER, m_vboHeightplotNormals);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(normals.size() * 3 * sizeof(float)),
                    normals.data());

    glDrawElements(GL_TRIANGLE_STRIP,
                   static_cast<GLsizei>(m_indices.size()),
                   GL_UNSIGNED_SHORT,
                   static_cast<GLvoid*>(nullptr));
}

std::vector<QVector3D> Visualization::computeNormals(std::vector<float> heights)
{
    return std::vector<QVector3D>(heights.size(), QVector3D(0,0,1));
}

// drag: When the user drags with the mouse, add a force that corresponds to the direction of the mouse
//       cursor movement. Also inject some new matter into the field at the mouse location.
void Visualization::drag(int const mx, int my)
{
    // Remembers last mouse location.
    static int lmx = 0;
    static int lmy = 0;

    // Compute the array index that corresponds to the cursor location.
    // X ranges from 0 (left) to m_DIM (right)
    // Y ranges from 0 (bottom) to m_DIM (top)
    auto X = static_cast<size_t>(std::floor(static_cast<float>(m_DIM + 1) * (static_cast<float>(mx) / static_cast<float>(width()))));
    auto Y = static_cast<size_t>(std::floor(static_cast<float>(m_DIM + 1) * (static_cast<float>(my) / static_cast<float>(height()))));
    X = std::clamp(X, static_cast<size_t>(0), m_DIM - 1);
    Y = std::clamp(Y, static_cast<size_t>(0), m_DIM - 1);

    // Add force at the cursor location.
    float dx = mx - lmx;
    float dy = my - lmy;
    float const length = std::sqrt(std::pow(dx, 2.0F) + std::pow(dy, 2.0F));

    if (length != 0.0F)
    {
        dx *= 0.1F / length;
        dy *= 0.1F / length;
    }

    size_t const idx = X + Y * m_DIM;

    m_simulation.setFx(idx, m_simulation.fx(idx) + dx);
    m_simulation.setFy(idx, m_simulation.fy(idx) + dy);
    m_simulation.setRho(idx, m_simulation.rhoInjected());

    // Store the current mouse position as the previous mouse position.
    lmx = mx;
    lmy = my;
}

void Visualization::onMessageLogged(QOpenGLDebugMessage const &Message) const
{
    qDebug() << "Log from Visualization:" << Message;
}

// Setters
void Visualization::setDIM(size_t const DIM)
{
    // Stop the simulation, do all resizing, then continue.
    m_timer.stop();

    m_DIM = DIM;
    setupAllBuffers();
    resizeGL(width(), height());
    m_simulation.setDIM(m_DIM);
    m_timer.start();
}
