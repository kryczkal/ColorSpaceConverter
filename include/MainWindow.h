#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define MAINWINDOW_H

#include "ColorProfileSettings.h"
#include "ImageSpaceConverter.h"
#include <QBitmap>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>

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
    bool showOutOfGamut                  = false;
    QImage gamutMask;

    // UI Elements
    QPixmap sourceImage;
    QPixmap targetImage;
    QLabel *sourceImageLabel;
    QLabel *targetImageLabel;

    struct ColorControls
    {
        QLineEdit *xValue;
        QLineEdit *yValue;
    };

    struct ColorProfileControls
    {
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

    QGroupBox *createSettingsGroup(const QString &title, ColorProfileControls &settings, ColorProfileSettings &profile);
    void resizeEvent(QResizeEvent *event) override;

    private slots:
    void onLoadClicked();
    void onSaveClicked();
    void onConvertClicked();
    void onShowOutOfGamutClicked();

    QHBoxLayout *createToolbarLayout();

    ConversionType selectConversionType();
};

#endif // MAINWINDOW_H
