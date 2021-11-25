#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColorDialog>

// Isolines, draw on/off.
void MainWindow::on_isolinesDrawIsolinesCheckBox_toggled(bool checked)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    visualizationPtr->m_drawIsolines = checked;
}

// Isolines, color picker.
void MainWindow::on_isolinesColorPickerButton_clicked()
{
    QColor const colorFromUI = QColorDialog::getColor(Qt::yellow, this);
    if (colorFromUI.isValid())
    {
        // double r, g, b;
        // colorFromUI.getRgbF(&r, &g, &b);
        // QVector3D isolineColor{static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};

        /*
        Color const isolineColor{static_cast<float>(colorFromUI.redF()),
                     static_cast<float>(colorFromUI.greenF()),
                     static_cast<float>(colorFromUI.blueF())};
         */

        auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
        visualizationPtr->m_isolineColor = {static_cast<float>(colorFromUI.redF()), static_cast<float>(colorFromUI.greenF()), static_cast<float>(colorFromUI.blueF())};
    }
    else
        qDebug() << "Color dialog did not return a valid color.";
}

// Isolines, pick data type manually or not.
void MainWindow::on_isolinesUseCurrentScalarDataCheckBox_toggled(bool checked)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    visualizationPtr->m_manuallyChooseIsolineDataType = !checked;
}

// Isolines, data type.
void MainWindow::on_isolinesDataComboBox_currentIndexChanged(int index)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");

    switch (index)
    {
        case 0:
            visualizationPtr->m_currentIsolineDataType = ScalarDataType::Density;
        break;

        case 1:
            visualizationPtr->m_currentIsolineDataType = ScalarDataType::VelocityMagnitude;
        break;

        case 2:
            visualizationPtr->m_currentIsolineDataType = ScalarDataType::ForceFieldMagnitude;
        break;

        case 3:
            visualizationPtr->m_currentIsolineDataType = ScalarDataType::VelocityDivergence;
        break;

        case 4:
            visualizationPtr->m_currentIsolineDataType = ScalarDataType::ForceFieldDivergence;
        break;
    }
}

// Isolines, range.
void MainWindow::on_isolinesNumberOfIsolinesSpinBox_valueChanged(int value)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    visualizationPtr->m_numberOfIsolines = static_cast<size_t>(value);
}

void MainWindow::on_isolinesRangeRhoMinSpinBox_valueChanged(double value)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    visualizationPtr->m_isolineMinValue = static_cast<float>(value);
}

void MainWindow::on_isolinesRangeRhoMaxSpinBox_valueChanged(double value)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    visualizationPtr->m_isolineMaxValue = static_cast<float>(value);
}

void MainWindow::on_isolinesRangeRhoMinSlider_valueChanged(int value)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    ui->isolinesRangeRhoMinSpinBox->setValue(static_cast<double>(value));
}

void MainWindow::on_isolinesRangeRhoMaxSlider_valueChanged(int value)
{
    auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
    ui->isolinesRangeRhoMaxSpinBox->setValue(static_cast<double>(value));
}

void MainWindow::on_isolinesInterpolationMethodLinearRadioButton_toggled(bool checked)
{
    if (checked)
    {
        auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
        visualizationPtr->m_isolineInterpolationMethod = Isoline::InterpolationMethod::Linear;
    }
}

void MainWindow::on_isolinesInterpolationMethodNoneRadioButton_toggled(bool checked)
{
    if (checked)
    {
        auto const visualizationPtr = findChildSafe<Visualization*>("visualizationOpenGLWidget");
        visualizationPtr->m_isolineInterpolationMethod = Isoline::InterpolationMethod::None;
    }
}
