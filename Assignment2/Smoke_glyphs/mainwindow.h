#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "color.h"
#include "colormap.h"
#include "texture.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class Legend;
    friend class Visualization;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Simulation, show minimum/maximum values of scalar and vector data.
    void on_showMinMaxDataCheckBox_toggled(bool checked);

    // Simulation, density injected fluid.
    void on_densitySlider_valueChanged(int value);
    void on_densitySpinBox_valueChanged(double value);

    // Simulation, viscosity injected fluid.
    void on_viscositySlider_valueChanged(int value);
    void on_viscositySpinBox_valueChanged(double value);

    // Simulation, timestep.
    void on_timestepSlider_valueChanged(int value);
    void on_timestepSpinBox_valueChanged(double value);

    // Simulation, number of gridpoints.
    void on_gridpointsSpinBox_valueChanged(int value);

    // Simulation, run simulation.
    void on_pausePlayButton_clicked();

    // Scalar data, draw true/false.
    void on_scalarDataDrawScalarDataCheckBox_toggled(bool checked);

    // Scalar data, data type.
    void on_scalarDataComboBox_currentIndexChanged(int index);

    // Scalar data, color map.
    void on_scalarDataColorMapComboBox_currentIndexChanged(int index);
    void on_scalarDataColorMapNumberOfColorsSpinBox_valueChanged(int value);

    // Scalar data, push button for the color picker
    void on_scalarDataCustomColorPickerPushButton_clicked();

    // Scalar data, mapping type.
    void on_scalarDataMappingMappingTypeScalingRadioButton_toggled(bool checked);
    void on_scalarDataMappingMappingTypeClampingRadioButton_toggled(bool checked);

    // Scalar data, mapping, scaling.
    void on_scalarDataMappingScalingMovingAverageWindowSpinBox_valueChanged(int arg1);

    // Scalar data, mapping, clamping.
    void on_scalarDataMappingClampingMinSlider_valueChanged(int value);
    void on_scalarDataMappingClampingMinDoubleSpinBox_valueChanged(double arg1);
    void on_scalarDataMappingClampingMaxSlider_valueChanged(int value);
    void on_scalarDataMappingClampingMaxDoubleSpinBox_valueChanged(double arg1);

    // Scalar data, mapping, transfer function.
    void on_scalarDataMappingTransferKSlider_valueChanged(int value);
    void on_scalarDataMappingTransferKSpinBox_valueChanged(double value);


    // Vector data, draw on/off.
    void on_vectorDataDrawGlyphsCheckBox_toggled(bool checked);

    // Vector data, number of glyphs.
    void on_vectorDataNumberOfGlyphsHorizontalSpinBox_valueChanged(int arg1);
    void on_vectorDataNumberOfGlyphsVerticalSpinBox_valueChanged(int arg1);

    // Vector data, data type.
    void on_vectorDataComboBox_currentIndexChanged(int index);

    // Vector data, glyph type.
    void on_vectorDataGlyphComboBox_currentIndexChanged(int index);

    // Vector data, glyph magnifier.
    void on_vectorDataGlyphMagnifierDoubleSpinBox_valueChanged(double arg1);
    void on_vectorDataGlyphMagnifierSlider_valueChanged(int value);

    // Vector data, color map.
    void on_vectorDataColorMapComboBox_currentIndexChanged(int index);
    void on_vectorDataNumberOfColorsSpinBox_valueChanged(int value);

    // Setters
    void setScalarDataMin(float const min);
    void setScalarDataMax(float const max);
    void setVectorDataMin(float const min);
    void setVectorDataMax(float const max);

private:
    Ui::MainWindow *ui;

    ColorMap m_scalarDataColorMap = ColorMap::Grayscale;
    size_t m_numberOfColorsScalarData = 256U;
    std::vector<Color> m_defaultScalarDataColorMap = Texture::createGrayscaleTexture(m_numberOfColorsScalarData);

    ColorMap m_vectorDataColorMap = ColorMap::Grayscale;
    size_t m_numberOfColorsVectorData = 256U;
    std::vector<Color> m_defaultVectorDataColorMap = Texture::createGrayscaleTexture(m_numberOfColorsScalarData);

    std::vector<Color> enumToColorMap(ColorMap const colorMap, size_t numberOfColors) const;
    void updateScalarDataColorMapGlobally() const;
    void updateVectorDataColorMapGlobally() const;

    template <class T> T findChildSafe(QString const &widgetName) const;
};

template <class T>
T MainWindow::findChildSafe(QString const &widgetName) const
{
    T widgetPtr = findChild<T>(widgetName);
    Q_ASSERT(widgetPtr != nullptr);

    return widgetPtr;
}

#endif // MAINWINDOW_H
