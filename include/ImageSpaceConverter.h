//
// Created by wookie on 12/8/24.
//

#ifndef IMAGEPROFILECONVERTER_IMAGESPACECONVERTER_H
#define IMAGEPROFILECONVERTER_IMAGESPACECONVERTER_H

#include "functional"
#include "unordered_map"
#include <optional>
#include <QVector3D>
#include <QImage>

class ColorProfileSettings;
class QImage;
class QColor;
class double2;

enum class ConversionType
{
    AbsoluteColorimetric,
    RelativeColorimetric,
    Perceptual,
    Saturation
};

class ConversionOutput
{
    public:
    QImage convertedImage;
    QImage outOfGamutMask;
};

class ImageSpaceConverter
{
    public:
    static ConversionOutput convert(
        const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile,
        ConversionType conversionType
    );
    static ConversionOutput convertAbsoluteColorimetric(
        const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
    );

    static ConversionOutput convertRelativeColorimetric(
        const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
    );

    static ConversionOutput convertPerceptual(
        const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
    );

    static ConversionOutput convertPreserveSaturation(
        const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
    );

    // Helper functions
    static QMatrix4x4
    computeRGBtoXYZMatrix(const double2 &white, const double2 &red, const double2 &green, const double2 &blue);
    static QVector3D applyGammaCorrection(const QColor &color, double gamma);
    static QColor applyInverseGammaCorrection(const QVector3D &color, double gamma);
    static QVector3D
    transformColor(const QVector3D &color, const QMatrix4x4 &sourceRGBtoXYZ, const QMatrix4x4 &targetXYZtoRGB);
    static QVector3D adjustWhitePoint(const QVector3D &color, const double2 &sourceWhite, const double2 &targetWhite);

    static void maskImage(QImage &image, QImage &mask);

    private:
    static std::unordered_map<
        ConversionType,
        std::function<ConversionOutput(const QImage &, const ColorProfileSettings &, const ColorProfileSettings &)>>
        conversionMethods;

    static void rgbToHsl(const QVector3D &rgb, float &h, float &s, float &l);

    static QVector3D hslToRgb(float h, float s, float l);

    static QVector3D computeXYZGamutBounds(const ColorProfileSettings &profile);

    static QVector3D scaleToTargetGamut(
        const QVector3D &xyz, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
    );

    static bool outOfGamut(const QVector3D &xyz);
};

#endif // IMAGEPROFILECONVERTER_IMAGESPACECONVERTER_H
