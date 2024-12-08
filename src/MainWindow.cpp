#include "MainWindow.h"
#include "ImageSpaceConverter.h"
#include "CommonProfiles.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QFrame>
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>


MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
{
    // Create central widget and main layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Create button toolbar
    QHBoxLayout *toolbarLayout = createToolbarLayout();
    mainLayout->addLayout(toolbarLayout);

    // Create main content layout that will take all available space
    QVBoxLayout *contentLayout = new QVBoxLayout();

    // Create settings layout that will only take needed space
    QHBoxLayout *settingsLayout = new QHBoxLayout();

    // Create source and target settings groups
    QGroupBox *sourceGroup = createSettingsGroup("Source Settings", sourceSettings, sourceProfile);
    QGroupBox *targetGroup = createSettingsGroup("Target Settings", targetSettings, targetProfile);

    sourceGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    targetGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    settingsLayout->addWidget(sourceGroup);
    settingsLayout->addWidget(targetGroup);

    contentLayout->addLayout(settingsLayout);

    // Create image areas layout
    QHBoxLayout *imageLayout = new QHBoxLayout();

    // Create source image area
    sourceImageLabel = new QLabel(this);
    sourceImageLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    sourceImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sourceImageLabel->setAlignment(Qt::AlignCenter);
    sourceImageLabel->setMinimumHeight(200);

    // Create target image area
    targetImageLabel = new QLabel(this);
    targetImageLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    targetImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    targetImageLabel->setAlignment(Qt::AlignCenter);
    targetImageLabel->setMinimumHeight(200);

    imageLayout->addWidget(sourceImageLabel);
    imageLayout->addWidget(targetImageLabel);

    contentLayout->addLayout(imageLayout, 1);
    mainLayout->addLayout(contentLayout);

    // Set window properties
    setWindowTitle("Color Profile Converter");
    resize(800, 600);

}

QHBoxLayout *MainWindow::createToolbarLayout() {
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    loadButton = new QPushButton("Load", this);
    saveButton = new QPushButton("Save", this);
    convertButton = new QPushButton("Convert", this);

    toolbarLayout->addWidget(loadButton);
    toolbarLayout->addWidget(saveButton);
    toolbarLayout->addWidget(convertButton);
    toolbarLayout->addStretch();

    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoadClicked);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(convertButton, &QPushButton::clicked, this, &MainWindow::onConvertClicked);
    return toolbarLayout;
}

QGroupBox* MainWindow::createSettingsGroup(const QString &title, ColorProfileControls &settings, ColorProfileSettings &profile)
{
    QGroupBox *group = new QGroupBox(title);
    QGridLayout *layout = new QGridLayout(group);

    QComboBox *profileCombo = new QComboBox(this);
    for (int i = 0; i < CommonProfiles::profilesCount; ++i) {
        profileCombo->addItem(CommonProfiles::profiles[i].name);
    }
    profileCombo->setCurrentIndex(CommonProfiles::profilesCount - 1);
    layout->addWidget(profileCombo, 0, 0, 1, 3);  // span 3 columns

    layout->addWidget(new QLabel(""), 1, 0);
    layout->addWidget(new QLabel("X", this), 1, 1, Qt::AlignCenter);
    layout->addWidget(new QLabel("Y", this), 1, 2, Qt::AlignCenter);

    layout->addWidget(new QLabel("Gamma", this), 2, 0, Qt::AlignRight);
    settings.gamma = new QLineEdit(this);
    settings.gamma->setFixedWidth(60);
    settings.gamma->setValidator(new QDoubleValidator(0.0, 10.0, 6, this));
    settings.gamma->setText(QString::number(profile.gamma));
    layout->addWidget(settings.gamma, 2, 1);
    connect(settings.gamma, &QLineEdit::textChanged, [this, &profile](const QString &text) {
        profile.gamma = text.toDouble();
    });


    auto addColorControl = [this](const QString &name, ColorControls &controls, double2 &value, QGridLayout *layout, int row) {
        layout->addWidget(new QLabel(name, this), row, 0, Qt::AlignRight);

        controls.xValue = new QLineEdit(this);
        controls.yValue = new QLineEdit(this);

        controls.xValue->setFixedWidth(60);
        controls.yValue->setFixedWidth(60);

        controls.xValue->setText(QString::number(value.x));
        controls.yValue->setText(QString::number(value.y));

        QDoubleValidator *validator = new QDoubleValidator(0.0, 1.0, 6, this);
        validator->setNotation(QDoubleValidator::StandardNotation);
        controls.xValue->setValidator(validator);
        controls.yValue->setValidator(new QDoubleValidator(0.0, 1.0, 6, this));

        layout->addWidget(controls.xValue, row, 1);
        layout->addWidget(controls.yValue, row, 2);
        connect(controls.xValue, &QLineEdit::textChanged, [this, &value](const QString &text) {
            value.x = text.toDouble();
        });
        connect(controls.yValue, &QLineEdit::textChanged, [this, &value](const QString &text) {
            value.y = text.toDouble();
        });
    };

    addColorControl("White", settings.white, profile.white, layout, 3);
    addColorControl("Red", settings.red, profile.red, layout, 4);
    addColorControl("Green", settings.green, profile.green, layout, 5);
    addColorControl("Blue", settings.blue, profile.blue, layout, 6);

    connect(profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, &profile, &settings](int index) {
        if (index >= 0 && index < CommonProfiles::profilesCount) {
            const auto &selectedProfile = CommonProfiles::profiles[index].profile;
            profile = selectedProfile;

            settings.gamma->setText(QString::number(profile.gamma));
            settings.white.xValue->setText(QString::number(profile.white.x));
            settings.white.yValue->setText(QString::number(profile.white.y));
            settings.red.xValue->setText(QString::number(profile.red.x));
            settings.red.yValue->setText(QString::number(profile.red.y));
            settings.green.xValue->setText(QString::number(profile.green.x));
            settings.green.yValue->setText(QString::number(profile.green.y));
            settings.blue.xValue->setText(QString::number(profile.blue.x));
            settings.blue.yValue->setText(QString::number(profile.blue.y));
        }
    });

    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignTop);

    layout->setRowStretch(7, 1);

    return group;
}

void MainWindow::onLoadClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.xpm *.jpg *.bmp)");
    if (!filePath.isEmpty()) {
        QPixmap pixmap(filePath);
        if (!pixmap.isNull()) {
            sourceImage = pixmap;
            sourceImageLabel->setPixmap(sourceImage.scaled(sourceImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            QMessageBox::warning(this, "Error", "Failed to load image.");
        }
    }
}

void MainWindow::onSaveClicked()
{
    if (targetImage.isNull()) {
        QMessageBox::warning(this, "Error", "No converted image to save.");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "Save Image", "", "Images (*.png *.jpg *.bmp)");
    if (!filePath.isEmpty()) {
        if (!targetImage.save(filePath)) {
            QMessageBox::warning(this, "Error", "Failed to save image.");
        }
    }
}

void MainWindow::onConvertClicked()
{
    if (sourceImage.isNull()) {
        QMessageBox::warning(this, "Error", "No source image loaded.");
        return;
    }
    currentConversionType = selectConversionType();
    QImage convertedImage;
    convertedImage = ImageSpaceConverter::convert(sourceImage.toImage(), sourceProfile, targetProfile, currentConversionType);
    convertedImage = convertedImage.scaled(sourceImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    targetImage = QPixmap::fromImage(convertedImage);
    targetImageLabel->setPixmap(targetImage);
}

MainWindow::~MainWindow()
{
}

ConversionType MainWindow::selectConversionType() {
    QStringList items = {
            "Absolute Colorimetric",
            "Relative Colorimetric",
            "Perceptual",
            "Saturation"
    };

    bool ok;
    QString selected = QInputDialog::getItem(this, "Select Conversion Type",
                                             "Conversion Type:", items, 0, false, &ok);

    if (!ok) {
        return currentConversionType;
    }

    if (selected == "Absolute Colorimetric") {
        return ConversionType::AbsoluteColorimetric;
    } else if (selected == "Relative Colorimetric") {
        return ConversionType::RelativeColorimetric;
    } else if (selected == "Perceptual") {
        return ConversionType::Perceptual;
    } else if (selected == "Saturation") {
        return ConversionType::Saturation;
    }

    return currentConversionType;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}
