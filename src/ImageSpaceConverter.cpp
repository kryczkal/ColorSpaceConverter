//
// Created by wookie on 12/8/24.
//

#include "ImageSpaceConverter.h"
#include "ColorProfileSettings.h"
#include <QColor>
#include <QImage>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector4D>
#include <complex>
#include <functional>
#include <qvector3d.h>
#include <unordered_map>

std::unordered_map<
    ConversionType,
    std::function<ConversionOutput(const QImage &, const ColorProfileSettings &, const ColorProfileSettings &)>>
    ImageSpaceConverter::conversionMethods = {
        {ConversionType::AbsoluteColorimetric, &ImageSpaceConverter::convertAbsoluteColorimetric},
        {ConversionType::RelativeColorimetric, &ImageSpaceConverter::convertRelativeColorimetric},
        {          ConversionType::Perceptual,           &ImageSpaceConverter::convertPerceptual},
        {          ConversionType::Saturation,   &ImageSpaceConverter::convertPreserveSaturation}
};

ConversionOutput ImageSpaceConverter::convert(
    const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile,
    ConversionType conversionType
)
{
    return ImageSpaceConverter::conversionMethods[conversionType](sourceImage, sourceProfile, targetProfile);
}

// Helper: Compute RGB to XYZ transformation matrix
QMatrix4x4 ImageSpaceConverter::computeRGBtoXYZMatrix(
    const double2 &white, const double2 &red, const double2 &green, const double2 &blue
)
{
    // Calc Sr, Sg, Sb
    // from source white point
    static constexpr double Yw = 1.0;
    QVector4D whitePointXYZ    = QVector4D(white.x * Yw / white.y, Yw, Yw * (1.0 - white.x - white.y) / white.y, 1.0);
    QMatrix4x4 whitePointMatrix;
    whitePointMatrix.setRow(0, QVector4D(red.x, green.x, blue.x, 0.0));
    whitePointMatrix.setRow(1, QVector4D(red.y, green.y, blue.y, 0.0));
    whitePointMatrix.setRow(2, QVector4D(1.0 - red.x - red.y, 1.0 - green.x - green.y, 1.0 - blue.x - blue.y, 0.0));
    whitePointMatrix.setRow(3, QVector4D(0.0, 0.0, 0.0, 1.0));
    QVector4D coeffs = whitePointMatrix.inverted().map(whitePointXYZ);

    // Calc RGB to XYZ matrix
    QMatrix4x4 rgbToXYZ;
    rgbToXYZ.setRow(0, QVector4D(red.x * coeffs.x(), green.x * coeffs.y(), blue.x * coeffs.z(), 0.0));
    rgbToXYZ.setRow(1, QVector4D(red.y * coeffs.x(), green.y * coeffs.y(), blue.y * coeffs.z(), 0.0));
    rgbToXYZ.setRow(
        2, QVector4D(
               (1.0 - red.x - red.y) * coeffs.x(), (1.0 - green.x - green.y) * coeffs.y(),
               (1.0 - blue.x - blue.y) * coeffs.z(), 0.0
           )
    );
    rgbToXYZ.setRow(3, QVector4D(0.0, 0.0, 0.0, 1.0));
    return rgbToXYZ;
}

QVector3D ImageSpaceConverter::applyGammaCorrection(const QColor &color, double gamma)
{
    return QVector3D(std::pow(color.redF(), gamma), std::pow(color.greenF(), gamma), std::pow(color.blueF(), gamma));
}

QColor ImageSpaceConverter::applyInverseGammaCorrection(const QVector3D &color, double gamma)
{
    return QColor::fromRgbF(
        std::pow(color.x(), 1.0 / gamma), std::pow(color.y(), 1.0 / gamma), std::pow(color.z(), 1.0 / gamma)
    );
}

QVector3D ImageSpaceConverter::transformColor(
    const QVector3D &color, const QMatrix4x4 &sourceRGBtoXYZ, const QMatrix4x4 &targetXYZtoRGB
)
{
    QVector3D xyz       = sourceRGBtoXYZ.mapVector(color);
    QVector3D targetRGB = targetXYZtoRGB.mapVector(xyz);
    return targetRGB;
}

QVector3D
ImageSpaceConverter::adjustWhitePoint(const QVector3D &color, const double2 &sourceWhite, const double2 &targetWhite)
{
    // Bradford transformation matrix
    QMatrix4x4 bradford;
    bradford.setRow(0, QVector4D(0.8951, 0.2664, -0.1614, 0.0));
    bradford.setRow(1, QVector4D(-0.7502, 1.7135, 0.0367, 0.0));
    bradford.setRow(2, QVector4D(0.0389, -0.0685, 1.0296, 0.0));
    bradford.setRow(3, QVector4D(0.0, 0.0, 0.0, 1.0));

    QMatrix4x4 inverseBradford = bradford.inverted();

    QVector3D sourceWhiteXYZ(sourceWhite.x / sourceWhite.y, 1.0, (1.0 - sourceWhite.x - sourceWhite.y) / sourceWhite.y);
    QVector3D targetWhiteXYZ(targetWhite.x / targetWhite.y, 1.0, (1.0 - targetWhite.x - targetWhite.y) / targetWhite.y);

    QVector3D sourceCone = bradford.mapVector(sourceWhiteXYZ);
    QVector3D targetCone = bradford.mapVector(targetWhiteXYZ);

    QVector3D adaptationScale(
        targetCone.x() / sourceCone.x(), targetCone.y() / sourceCone.y(), targetCone.z() / sourceCone.z()
    );

    QMatrix4x4 adaptationMatrix;
    adaptationMatrix.scale(adaptationScale);

    QVector3D adaptedColor = inverseBradford.mapVector(adaptationMatrix.mapVector(bradford.mapVector(color)));

    return adaptedColor;
}

ConversionOutput ImageSpaceConverter::convertAbsoluteColorimetric(
    const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
)
{
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);
    QImage outOfGamutMask(resultImage.size(), QImage::Format_RGB32);

    QMatrix4x4 sourceRGBtoXYZ =
        computeRGBtoXYZMatrix(sourceProfile.white, sourceProfile.red, sourceProfile.green, sourceProfile.blue);
    QMatrix4x4 targetRGBtoXYZ =
        computeRGBtoXYZMatrix(targetProfile.white, targetProfile.red, targetProfile.green, targetProfile.blue);
    QMatrix4x4 targetXYZtoRGB = targetRGBtoXYZ.inverted();

    for (int y = 0; y < resultImage.height(); ++y)
    {
        for (int x = 0; x < resultImage.width(); ++x)
        {
            QColor color        = resultImage.pixelColor(x, y);
            QVector3D linearRGB = applyGammaCorrection(color, sourceProfile.gamma);
            QVector3D targetRGB = transformColor(linearRGB, sourceRGBtoXYZ, targetXYZtoRGB);
            if (outOfGamut(targetRGB))
            {
                outOfGamutMask.setPixelColor(x, y, Qt::white);
            }
            else
            {
                outOfGamutMask.setPixelColor(x, y, Qt::black);
            }
            resultImage.setPixelColor(x, y, applyInverseGammaCorrection(targetRGB, targetProfile.gamma));
        }
    }

    return {resultImage, outOfGamutMask};
}

ConversionOutput ImageSpaceConverter::convertRelativeColorimetric(
    const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
)
{
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);
    QImage outOfGamutMask(resultImage.size(), QImage::Format_RGB32);

    QMatrix4x4 sourceRGBtoXYZ =
        computeRGBtoXYZMatrix(sourceProfile.white, sourceProfile.red, sourceProfile.green, sourceProfile.blue);
    QMatrix4x4 targetRGBtoXYZ =
        computeRGBtoXYZMatrix(targetProfile.white, targetProfile.red, targetProfile.green, targetProfile.blue);
    QMatrix4x4 targetXYZtoRGB = targetRGBtoXYZ.inverted();

    for (int y = 0; y < resultImage.height(); ++y)
    {
        for (int x = 0; x < resultImage.width(); ++x)
        {
            QColor color          = resultImage.pixelColor(x, y);
            QVector3D linearRGB   = applyGammaCorrection(color, sourceProfile.gamma);
            QVector3D adjustedRGB = adjustWhitePoint(linearRGB, sourceProfile.white, targetProfile.white);
            QVector3D targetRGB   = transformColor(adjustedRGB, sourceRGBtoXYZ, targetXYZtoRGB);
            if (outOfGamut(targetRGB))
            {
                outOfGamutMask.setPixelColor(x, y, Qt::white);
            }
            else
            {
                outOfGamutMask.setPixelColor(x, y, Qt::black);
            }
            resultImage.setPixelColor(x, y, applyInverseGammaCorrection(targetRGB, targetProfile.gamma));
        }
    }

    return {resultImage, outOfGamutMask};
}

ConversionOutput ImageSpaceConverter::convertPerceptual(
    const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
)
{
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);
    QImage outOfGamutMask(resultImage.size(), QImage::Format_RGB32);

    QMatrix4x4 sourceRGBtoXYZ =
        computeRGBtoXYZMatrix(sourceProfile.white, sourceProfile.red, sourceProfile.green, sourceProfile.blue);
    QMatrix4x4 targetRGBtoXYZ =
        computeRGBtoXYZMatrix(targetProfile.white, targetProfile.red, targetProfile.green, targetProfile.blue);
    QMatrix4x4 targetXYZtoRGB = targetRGBtoXYZ.inverted();

    for (int y = 0; y < resultImage.height(); ++y)
    {
        for (int x = 0; x < resultImage.width(); ++x)
        {
            QColor color        = resultImage.pixelColor(x, y);
            QVector3D linearRGB = applyGammaCorrection(color, sourceProfile.gamma);
            QVector3D xyz       = sourceRGBtoXYZ.mapVector(linearRGB);
            QVector3D scaledXYZ = scaleToTargetGamut(xyz, sourceProfile, targetProfile);
            QVector3D targetRGB = targetXYZtoRGB.mapVector(scaledXYZ);
            if (outOfGamut(targetRGB))
            {
                outOfGamutMask.setPixelColor(x, y, Qt::white);
            }
            else
            {
                outOfGamutMask.setPixelColor(x, y, Qt::black);
            }
            QColor finalColor = applyInverseGammaCorrection(targetRGB, targetProfile.gamma);
            resultImage.setPixelColor(x, y, finalColor);
        }
    }

    // No out of gamut mask for perceptual conversion
    return {resultImage, outOfGamutMask};
}

// Conversion: Saturation
ConversionOutput ImageSpaceConverter::convertPreserveSaturation(
    const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
)
{
    ConversionOutput output = convertAbsoluteColorimetric(sourceImage, sourceProfile, targetProfile);
    QImage &resultImage     = output.convertedImage;

    for (int y = 0; y < resultImage.height(); ++y)
    {
        for (int x = 0; x < resultImage.width(); ++x)
        {
            QColor targetColor = resultImage.pixelColor(x, y);
            QColor sourceColor = sourceImage.pixelColor(x, y);

            float sourceH, sourceS, sourceL;
            rgbToHsl(
                QVector3D(sourceColor.redF(), sourceColor.greenF(), sourceColor.blueF()), sourceH, sourceS, sourceL
            );
            float targetH, targetS, targetL;
            rgbToHsl(
                QVector3D(targetColor.redF(), targetColor.greenF(), targetColor.blueF()), targetH, targetS, targetL
            );

            // Keep source saturation
            QVector3D adjustedRgb = hslToRgb(targetH, sourceS, targetL);

            resultImage.setPixelColor(x, y, QColor::fromRgbF(adjustedRgb.x(), adjustedRgb.y(), adjustedRgb.z()));
        }
    }

    return output;
}

void ImageSpaceConverter::rgbToHsl(const QVector3D &rgb, float &h, float &s, float &l)
{
    float r = rgb.x();
    float g = rgb.y();
    float b = rgb.z();

    float maxVal = std::max({r, g, b});
    float minVal = std::min({r, g, b});
    l            = (maxVal + minVal) / 2.0f;

    if (maxVal == minVal)
    {
        h = s = 0.0f;
    }
    else
    {
        float delta = maxVal - minVal;
        s           = (l > 0.5f) ? delta / (2.0f - maxVal - minVal) : delta / (maxVal + minVal);

        if (maxVal == r)
        {
            h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
        }
        else if (maxVal == g)
        {
            h = (b - r) / delta + 2.0f;
        }
        else
        {
            h = (r - g) / delta + 4.0f;
        }
        h /= 6.0f;
    }
}

QVector3D ImageSpaceConverter::hslToRgb(float h, float s, float l)
{
    auto hueToRgb = [](float p, float q, float t) -> float
    {
        if (t < 0.0f)
            t += 1.0f;
        if (t > 1.0f)
            t -= 1.0f;
        if (t < 1.0f / 6.0f)
            return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f)
            return q;
        if (t < 2.0f / 3.0f)
            return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
        return p;
    };

    float r, g, b;
    if (s == 0.0f)
    {
        r = g = b = l; // Achromatic
    }
    else
    {
        float q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r       = hueToRgb(p, q, h + 1.0f / 3.0f);
        g       = hueToRgb(p, q, h);
        b       = hueToRgb(p, q, h - 1.0f / 3.0f);
    }

    return QVector3D(r, g, b);
}

QVector3D ImageSpaceConverter::scaleToTargetGamut(
    const QVector3D &xyz, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile
)
{
    QVector3D sourceMax = computeXYZGamutBounds(sourceProfile);
    QVector3D targetMax = computeXYZGamutBounds(targetProfile);

    QVector3D scaleFactors =
        QVector3D(targetMax.x() / sourceMax.x(), targetMax.y() / sourceMax.y(), targetMax.z() / sourceMax.z());

    return QVector3D(xyz.x() * scaleFactors.x(), xyz.y() * scaleFactors.y(), xyz.z() * scaleFactors.z());
}

QVector3D ImageSpaceConverter::computeXYZGamutBounds(const ColorProfileSettings &profile)
{
    QMatrix4x4 rgbToXYZ = computeRGBtoXYZMatrix(profile.white, profile.red, profile.green, profile.blue);
    QVector3D redXYZ    = rgbToXYZ.mapVector(QVector3D(1.0, 0.0, 0.0));
    QVector3D greenXYZ  = rgbToXYZ.mapVector(QVector3D(0.0, 1.0, 0.0));
    QVector3D blueXYZ   = rgbToXYZ.mapVector(QVector3D(0.0, 0.0, 1.0));

    return QVector3D(
        std::max({redXYZ.x(), greenXYZ.x(), blueXYZ.x()}), std::max({redXYZ.y(), greenXYZ.y(), blueXYZ.y()}),
        std::max({redXYZ.z(), greenXYZ.z(), blueXYZ.z()})
    );
}

bool ImageSpaceConverter::outOfGamut(const QVector3D &rgb)
{
    static constexpr double epsilon = 1e-3;
    return rgb.x() < -epsilon || rgb.y() < -epsilon || rgb.z() < -epsilon || rgb.x() > 1.0 + epsilon ||
           rgb.y() > 1.0 + epsilon || rgb.z() > 1.0 + epsilon;
}

void ImageSpaceConverter::maskImage(QImage &image, QImage &mask)
{
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (mask.pixelColor(x, y) == QColor(Qt::white))
            {
                image.setPixelColor(x, y, QColor(Qt::magenta));
            }
        }
    }
}
