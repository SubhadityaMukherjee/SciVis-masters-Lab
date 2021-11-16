#include "visualization.h"

#include "constants.h"
#include "mainwindow.h"
#include "texture.h"

#include <fftw3.h>

#include <QDebug>

#include <algorithm>
#include <cmath>

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

    glDeleteTextures(1, &m_scalarDataTextureLocation);
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

    // Retrieve default textures.
    auto const mainWindowPtr = qobject_cast<MainWindow*>(parent()->parent());
    std::vector<Color> const defaultScalarDataColorMap = mainWindowPtr->m_defaultScalarDataColorMap;

    // Generate buffers.
    glGenVertexArrays(1, &m_vaoScalarData);
    glGenBuffers(1, &m_vboScalarPoints);
    glGenBuffers(1, &m_vboScalarData);
    glGenBuffers(1, &m_eboScalarData);
    glGenTextures(1, &m_scalarDataTextureLocation);

    setupAllBuffers();

    loadScalarDataTexture(defaultScalarDataColorMap);

    m_normalTransformationMatrix.setToIdentity();
}

void Visualization::setupAllBuffers()
{
    setupScalarData();
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

    size_t const numberOfTriangleStripIndices = (m_DIM - 1U) * (2U * m_DIM + 2U) - 2U;
    m_indices.reserve(numberOfTriangleStripIndices);

    for (unsigned short stripIdx = 0U; stripIdx < (m_DIM * (m_DIM - 1U)); stripIdx += m_DIM)
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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_drawScalarData)
        drawScalarData();
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
}

/* This function receives a *reference* to a std::vector<float>,
 * which acts as a pointer. Modifying scalarValues here will result
 * in the scalarValues passed to this function to be modified.
 * You may also define a new "std::vector<float> v" here, fill it with
 * new values and assign it to scalarValues to replace it,
 * e.g. "scalarValues = v;".
 *
 * m_sliceIdx contains the value set in the GUI.
 * m_DIM contains the current dimensions of the square (m_DIM * m_DIM).
 * m_slicingWindowSize contains the size of the window (here, also m_DIM).
 * m_slicingDirection contains the slicing direction set in the GUI and
 *    is already handled in a switch statement.
 */
void Visualization::applySlicing(std::vector<float> &scalarValues)
{
    qDebug() << "Slicing not implemented";
    // Add code here and below to complete the implementation

    switch (m_slicingDirection)
    {
    case SlicingDirection::x:
        // xIdx is constant
        qDebug() << "Slicing in x not implemented";
        break;

    case SlicingDirection::y:
        // yIdx is constant
        qDebug() << "Slicing in y not implemented";
        break;

    case SlicingDirection::t:
        // t is constant
        qDebug() << "Slicing in t not implemented";
        break;
    }
}

void Visualization::applyPreprocessing(std::vector<float> &scalarValues)
{
    // Other preprocessing steps can be insered here

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

        case ScalarDataType::VelocityDivergence:
            qDebug() << "Velocity divergence not implemented";
        break;

        case ScalarDataType::ForceFieldDivergence:
            qDebug() << "Force field divergence not implemented";
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
    resizeGL(width(), height()); // TODO: Does this do too much or exactly enough?
    m_simulation.setDIM(m_DIM);
    m_timer.start();
}
