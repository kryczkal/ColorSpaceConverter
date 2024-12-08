#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include "ColorProfileSettings.h"
#include "ImageSpaceConverter.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    // App state
    ColorProfileSettings sourceProfile;
    ColorProfileSettings targetProfile;
    ConversionType currentConversionType = ConversionType::Perceptual;

    // UI Elements
    QPixmap sourceImage;
    QPixmap targetImage;
    QLabel *sourceImageLabel;
    QLabel *targetImageLabel;

    struct ColorControls {
        QLineEdit *xValue;
        QLineEdit *yValue;
    };

    struct ColorProfileControls {
        QLineEdit *gamma;
        ColorControls white;
        ColorControls red;
        ColorControls green;
        ColorControls blue;
    };

    ColorProfileControls sourceSettings;
    ColorProfileControls targetSettings;

    QPushButton *loadButton;
    QPushButton *saveButton;
    QPushButton *convertButton;

    QGroupBox* createSettingsGroup(const QString &title, ColorProfileControls &settings, ColorProfileSettings &profile);
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onLoadClicked();
    void onSaveClicked();
    void onConvertClicked();

    QHBoxLayout *createToolbarLayout();

    ConversionType selectConversionType();
};

#endif // MAINWINDOW_H
