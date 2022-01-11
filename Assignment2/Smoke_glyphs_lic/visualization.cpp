#include "visualization.h"

#include "constants.h"
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

    glDeleteVertexArrays(1, &m_vaoGlyphs);
    glDeleteBuffers(1, &m_vboGlyphs);
    glDeleteBuffers(1, &m_eboGlyphs);
    glDeleteBuffers(1, &m_vboModelTransformationMatricesGlyphs);
    glDeleteBuffers(1, &m_vboValuesGlyphs);

    glDeleteBuffers(1, &m_vaoLic);
    glDeleteBuffers(1, &m_vboLic);
    glDeleteTextures(1, &m_licTextureLocation);

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
    createShaderProgramColorMapInstanced();
    createShaderProgramLic();

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

    glGenVertexArrays(1, &m_vaoGlyphs);
    glGenBuffers(1, &m_vboGlyphs);
    glGenBuffers(1, &m_eboGlyphs);
    glGenBuffers(1, &m_vboModelTransformationMatricesGlyphs);
    glGenBuffers(1, &m_vboValuesGlyphs);
    glGenTextures(1, &m_vectorDataTextureLocation);

    glGenVertexArrays(1, &m_vaoLic);
    glGenBuffers(1, &m_vboLic);
    glGenTextures(1, &m_licTextureLocation);

    setupAllBuffers();

    loadScalarDataTexture(defaultScalarDataColorMap);
    loadVectorDataTexture(defaultVectorDataColorMap);

    loadLicTexture(std::vector<uint8_t>()); // Initially provide an empty texture

    m_normalTransformationMatrix.setToIdentity();
}

void Visualization::setupAllBuffers()
{
    setupScalarData();
    setupGlyphs();
    setupLic();
}

void Visualization::setupScalarData()
{
    glBindVertexArray(m_vaoScalarData);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboScalarPoints);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_DIM * m_DIM * 2U * sizeof(float)),
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

    // No primitive restart, so the last (degenerate) triangles can be removed.
    m_indices.erase(m_indices.end() - 2, m_indices.end());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboScalarData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned short)),
                 m_indices.data(),
                 GL_STATIC_DRAW);
}

void Visualization::setupGlyphs()
{
    bufferSingleGlyph();
    setupGlyphsPerInstanceData();
}

void Visualization::setupLic()
{
    m_licObject.resetTexture();

    glBindVertexArray(m_vaoLic);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboLic);

    glEnableVertexAttribArray(0U);
    glVertexAttribPointer(0U, 2, GL_FLOAT, GL_FALSE, 2U * sizeof(QVector2D), reinterpret_cast<GLvoid*>(0));

    glEnableVertexAttribArray(1U);
    glVertexAttribPointer(1U, 2, GL_FLOAT, GL_FALSE, 2U * sizeof(QVector2D), reinterpret_cast<GLvoid*>(2U * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, m_vboLic);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(8U * sizeof(QVector2D)),
                 nullptr,
                 GL_STATIC_DRAW);
}

void Visualization::bufferSingleGlyph()
{
    std::pair<std::vector<QVector3D>, std::vector<unsigned short>> singleGlyph;

    switch (m_currentGlyphType)
    {
        case Glyph::GlyphType::Hedgehog:
            singleGlyph = Glyph::hedgehog();
        break;

        case Glyph::GlyphType::Triangle:
            singleGlyph = Glyph::triangle(0.3F);
        break;

        case Glyph::GlyphType::Arrow2D:
            singleGlyph = Glyph::arrow2D(0.1F, 0.5F, 0.3F);
        break;

        case Glyph::GlyphType::Cone:
            singleGlyph = Glyph::cone(0.3F, 10U);
        break;
    }

    auto const glyphVertices = singleGlyph.first;
    auto const glyphIndices = singleGlyph.second;
    m_glyphIndicesSize = glyphIndices.size();

    glBindVertexArray(m_vaoGlyphs);

    // Buffer a single glyph.
    glBindBuffer(GL_ARRAY_BUFFER, m_vboGlyphs);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(glyphVertices.size() * 3 * sizeof(float)),
                 glyphVertices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));

    // Buffer indices for the glyph.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_eboGlyphs);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_glyphIndicesSize * sizeof(unsigned short)),
                 glyphIndices.data(),
                 GL_STATIC_DRAW);
}

void Visualization::setupGlyphsPerInstanceData()
{
    // Buffering section starts here.
    glBindVertexArray(m_vaoGlyphs);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboValuesGlyphs);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_numberOfGlyphsX * m_numberOfGlyphsY * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);

    // Buffer values.
    static bool firstRun = true;
    if (firstRun)
    {
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribDivisor(1, 1);
    }

    // Buffer model transformation matrices.
    glBindBuffer(GL_ARRAY_BUFFER, m_vboModelTransformationMatricesGlyphs);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_numberOfGlyphsX * m_numberOfGlyphsY * 16 * sizeof(float)),
                 static_cast<GLvoid*>(nullptr),
                 GL_DYNAMIC_DRAW);

    // A location can maximally hold 4 values, so for a 4x4 matrix,
    // 4 attribute pointers need to be defined.
    if (firstRun)
    {
        firstRun = false;
        for (unsigned int columnIdx = 0; columnIdx < 4; ++columnIdx)
        {
            glVertexAttribPointer(2 + columnIdx,
                                  4,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  16 * sizeof(float),
                                  reinterpret_cast<GLvoid*>(4 * sizeof(float) * columnIdx));
            glEnableVertexAttribArray(2 + columnIdx);
            glVertexAttribDivisor(2 + columnIdx, 1);
        }
    }
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

void Visualization::createShaderProgramColorMapInstanced()
{
    m_shaderProgramVectorData.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/glyphsshading.vert");
    m_shaderProgramVectorData.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/glyphsshading.frag");
    m_shaderProgramVectorData.link();

    m_uniformLocationProjectionColorMapInstanced = m_shaderProgramVectorData.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationProjectionColorMapInstanced != -1);
    m_uniformLocationTextureColorMapInstanced = m_shaderProgramVectorData.uniformLocation("textureSampler");
    Q_ASSERT(m_uniformLocationTextureColorMapInstanced != -1);

    qDebug() << "m_shaderProgramVectorData initialized.";
}

void Visualization::createShaderProgramLic()
{
    m_shaderProgramLic.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/lic.vert");
    m_shaderProgramLic.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/lic.frag");
    m_shaderProgramLic.link();

    m_uniformLocationLic_projection = m_shaderProgramLic.uniformLocation("projectionTransform");
    Q_ASSERT(m_uniformLocationLic_projection != -1);

    m_uniformLocationLicTexture = m_shaderProgramLic.uniformLocation("textureSampler");
    Q_ASSERT(m_uniformLocationLicTexture != -1);

    m_shaderProgramLic.bind();

    qDebug() << "m_shaderProgramLic initialized.";
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

void Visualization::loadVectorDataTexture(std::vector<Color> const &colorMap)
{
    // Set texture parameters.
    glBindTexture(GL_TEXTURE_1D, m_vectorDataTextureLocation);
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

void Visualization::loadLicTexture(std::vector<uint8_t> const &licTexture)
{
    glBindTexture(GL_TEXTURE_2D, m_licTextureLocation);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 static_cast<GLint>(m_licObject.getXDim()),
                 static_cast<GLint>(m_licObject.getYDim()),
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 licTexture.data());
}

void Visualization::paintGL()
{
    glBindVertexArray(0);

    // Clear the screen before rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_drawScalarData)
        drawScalarData();

    if (m_drawLIC)
        drawLic();

    if (m_drawVectorData)
    {
        m_shaderProgramVectorData.bind();
        glUniformMatrix4fv(m_uniformLocationProjectionColorMapInstanced, 1, GL_FALSE, m_projectionTransformationMatrix.data());
        glUniform1i(m_uniformLocationTextureColorMapInstanced, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, m_vectorDataTextureLocation);
        drawGlyphs();
    }
}

void Visualization::resizeGL(int const width, int const height)
{
    m_cellWidth  = static_cast<float>(width) / static_cast<float>(m_DIM + 1U);
    m_cellHeight = static_cast<float>(height) / static_cast<float>(m_DIM + 1U);

    m_projectionTransformationMatrix.setToIdentity();

    m_projectionTransformationMatrix.ortho(0.0F, width,
                                           0.0F, height,
                                           -50.0F, 50.0F);

    m_glyphCellWidth = static_cast<float>(width) / static_cast<float>(m_numberOfGlyphsX + 1U);
    m_glyphCellHeight = static_cast<float>(height) / static_cast<float>(m_numberOfGlyphsY + 1U);

    updateScalarPoints();
    updateLicPoints();
}
void Visualization::drawGlyphs()
{
    std::vector<float> vectorMagnitude;
    std::vector<float> vectorDirectionX;
    std::vector<float> vectorDirectionY;
    switch (m_currentVectorDataType)
    {
        case VectorDataType::Velocity:
            vectorMagnitude = m_simulation.velocityMagnitudeInterpolated(m_numberOfGlyphsX, m_numberOfGlyphsY);
            vectorDirectionX = m_simulation.velocityXInterpolated(m_numberOfGlyphsX, m_numberOfGlyphsY);
            vectorDirectionY = m_simulation.velocityYInterpolated(m_numberOfGlyphsX, m_numberOfGlyphsY);
        break;

        case VectorDataType::ForceField:
            vectorMagnitude = m_simulation.forceFieldMagnitudeInterpolated(m_numberOfGlyphsX, m_numberOfGlyphsY);
            vectorDirectionX = m_simulation.forceFieldXInterpolated(m_numberOfGlyphsX, m_numberOfGlyphsY);
            vectorDirectionY = m_simulation.forceFieldYInterpolated(m_numberOfGlyphsX, m_numberOfGlyphsY);
        break;
    }

    // Scale the magnitudes to where these become visible.
    std::transform(vectorMagnitude.begin(), vectorMagnitude.end(), vectorMagnitude.begin(),
                   std::bind(std::multiplies<>(), std::placeholders::_1, m_vectorDataMagnifier));

    if (m_sendMinMaxToUI)
    {
        if (!vectorMagnitude.empty())
        {
            auto const currentMinMaxIt = std::minmax_element(vectorMagnitude.cbegin(), vectorMagnitude.cend());
            QVector2D const currentMinMax{*currentMinMaxIt.first, *currentMinMaxIt.second};

            // Send values to GUI.
            auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
            Q_ASSERT(mainWindowPtr != nullptr);
            mainWindowPtr->setVectorDataMin(currentMinMax.x());
            mainWindowPtr->setVectorDataMax(currentMinMax.y());
        }
    }

    size_t const numberOfInstances = m_numberOfGlyphsX * m_numberOfGlyphsY;

//    std::vector<float> modelTransformationMatrices;
    std::vector<QMatrix4x4> modelTransformationMatrices;
    /* Fill the container modelTransformationMatrices here...
     * Use the following variables:
     * modelTransformationMatrix: This vector should contain the result.
     * m_DIM: The grid dimensions of the simulation.
     * m_cellWidth, m_cellHeight: A cell, made up of 4 simulation grid points, has the size m_cellWidth * m_cellHeight for the visualization.
     * m_numberOfGlyphsX (horizontal)
     * m_numberOfGlyphsY (vertical)
     * m_glyphCellWidth, m_glyphCellHeight: Use these as a small offset from the border of the OpenGL window.
     *                                      Having this little border around the visualization prevents a left-pointing arrow
     *                                      on the left side of the window to become invisible and thus convey no information.
     * vectorDirectionX, vectorDirectionY: To which direction the glyph should point. Row-major, size given by the m_numberOfGlyphs*.
     * vectorMagnitude: Use this value to scale the glyphs. I.e. higher values are visualized using larger glyphs. Row-major, size given by the m_numberOfGlyphs*.
     */

    // Insert code here...
//    modelTransformationMatrices = std::vector<float>(numberOfInstances * 16U, 0.0F); // Remove this placeholder initialization
    modelTransformationMatrices = std::vector<QMatrix4x4>(numberOfInstances, QMatrix4x4());

    for(size_t i = 0; i < numberOfInstances; ++i)
    {
//        float mag = vectorMagnitude[i];
//        float dirX = vectorDirectionX[i];
//        float dirY = vectorDirectionY[i];

        QMatrix4x4 matrix = QMatrix4x4(0.0F, 0.0F, 0.0F, 1.0F,
                                       0.0F, 0.0F, 1.0F, 0.0F,
                                       0.0F, 1.0F, 0.0F, 0.0F,
                                       1.0F, 0.0F, 0.0F, 0.0F);

        modelTransformationMatrices[i] = matrix; // Constructs an identity matrix: https://doc.qt.io/qt-5/qmatrix4x4.html
    }

    // TODO: This shouldn't be here, but otherwise re-binding an already bound Glyphs VAO may cause glitches.
    glBindVertexArray(0);

    // Buffering section starts here.
    glBindVertexArray(m_vaoGlyphs);

    glBindBuffer(GL_ARRAY_BUFFER, m_vboValuesGlyphs);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(vectorMagnitude.size() * sizeof(float)),
                    vectorMagnitude.data());

    // Buffer model transformation matrices.
    glBindBuffer(GL_ARRAY_BUFFER, m_vboModelTransformationMatricesGlyphs);
    void * const dataPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(dataPtr, modelTransformationMatrices.data(), modelTransformationMatrices.size() * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    if (m_currentGlyphType == Glyph::GlyphType::Hedgehog)
        glDrawElementsInstanced(GL_LINES,
                                static_cast<GLsizei>(m_glyphIndicesSize),
                                GL_UNSIGNED_SHORT,
                                reinterpret_cast<GLvoid*>(0),
                                static_cast<GLsizei>(numberOfInstances));
    else
        glDrawElementsInstanced(GL_TRIANGLE_STRIP,
                                static_cast<GLsizei>(m_glyphIndicesSize),
                                GL_UNSIGNED_SHORT,
                                reinterpret_cast<GLvoid*>(0),
                                static_cast<GLsizei>(numberOfInstances));
}

void Visualization::updateScalarPoints()
{
    // Recompute and upload grid coordinates.
    std::vector<QVector2D> scalarPoints;
    scalarPoints.reserve(m_DIM * m_DIM);

    for (size_t j = 0U; j < m_DIM; ++j)
        for (size_t i = 0U; i < m_DIM; ++i)
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
                    static_cast<GLsizeiptr>(scalarPoints.size() * 2U * sizeof(float)),
                    scalarPoints.data());
}

void Visualization::updateLicPoints()
{
    // Recompute and upload grid coordinates for the quad that the LIC texture is rendered to.
    std::vector<QVector2D> licCoordsAndTexCoords;
    licCoordsAndTexCoords.reserve(8U);

    float min = m_cellWidth;
    float max = static_cast<float>(m_DIM) * m_cellWidth;

    // Top left OpenGL coordinate.
    licCoordsAndTexCoords.emplace_back(QVector2D{min, max});
    licCoordsAndTexCoords.emplace_back(QVector2D{0.0F, 1.0F});

    // Bottom left
    licCoordsAndTexCoords.emplace_back(QVector2D{min, min});
    licCoordsAndTexCoords.emplace_back(QVector2D{0.0F, 0.0F});

    // Top right
    licCoordsAndTexCoords.emplace_back(QVector2D{max, max});
    licCoordsAndTexCoords.emplace_back(QVector2D{1.0F, 1.0F});

    // Bottom right
    licCoordsAndTexCoords.emplace_back(QVector2D{max, min});
    licCoordsAndTexCoords.emplace_back(QVector2D{1.0F, 0.0F});

    glBindBuffer(GL_ARRAY_BUFFER, m_vboLic);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(licCoordsAndTexCoords.size() *sizeof(QVector2D)),
                    licCoordsAndTexCoords.data());
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
    }

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
    m_numberOfGlyphsX = m_DIM;
    m_numberOfGlyphsY = m_DIM;
    setupAllBuffers();
    resizeGL(width(), height());
    m_simulation.setDIM(m_DIM);
    m_timer.start();
}

void Visualization::setNumberOfGlyphsX(size_t const numberOfGlyphsX)
{
    m_numberOfGlyphsX = numberOfGlyphsX;
    setupGlyphsPerInstanceData();
}

void Visualization::setNumberOfGlyphsY(size_t const numberOfGlyphsY)
{
    m_numberOfGlyphsY = numberOfGlyphsY;
    setupGlyphsPerInstanceData();
}

void Visualization::drawLic()
{
    std::vector<float> vectorField_in_x;
    std::vector<float> vectorField_in_y;
    vectorField_in_x = m_simulation.velocityXInterpolated(m_licObject.getXDim(), m_licObject.getYDim()); // These should get the force field vectors of size equal to the simulation area, if I understand the function correctly?
    vectorField_in_y = m_simulation.velocityYInterpolated(m_licObject.getXDim(), m_licObject.getYDim());

    //m_licObject.resetTexture(); // Uncomment this line if you want the noise texture to look like its "Flowing".

    std::vector<float> texture_in = m_licObject.getTexture();

    std::vector<uint8_t> texture_out = m_licObject.updateTexture(vectorField_in_x, vectorField_in_y, texture_in); //Generate the texture to be sent to openGL

    m_shaderProgramLic.bind();
    glUniformMatrix4fv(m_uniformLocationLic_projection, 1, GL_FALSE, m_projectionTransformationMatrix.data());
    glUniform1i(m_uniformLocationLicTexture, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_licTextureLocation);

    loadLicTexture(texture_out);

    glBindVertexArray(m_vaoLic);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
