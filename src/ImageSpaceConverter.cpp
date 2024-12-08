//
// Created by wookie on 12/8/24.
//

#include <QImage>
#include <QColor>
#include <QVector4D>
#include <QMatrix4x4>
#include <functional>
#include <unordered_map>
#include <qvector3d.h>
#include <complex>
#include "ImageSpaceConverter.h"
#include "ColorProfileSettings.h"

std::unordered_map<ConversionType, std::function<QImage(const QImage &, const ColorProfileSettings &, const ColorProfileSettings &)> >
        ImageSpaceConverter::conversionMethods = {
        {ConversionType::AbsoluteColorimetric, &ImageSpaceConverter::convertAbsoluteColorimetric},
        {ConversionType::RelativeColorimetric, &ImageSpaceConverter::convertRelativeColorimetric},
        {ConversionType::Perceptual, &ImageSpaceConverter::convertPerceptual},
        {ConversionType::Saturation, &ImageSpaceConverter::convertSaturation}
};

QImage ImageSpaceConverter::convert(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                    const ColorProfileSettings &targetProfile, ConversionType conversionType) {
    return ImageSpaceConverter::conversionMethods[conversionType](sourceImage, sourceProfile, targetProfile);
}

// Helper: Compute RGB to XYZ transformation matrix
QMatrix4x4 ImageSpaceConverter::computeRGBtoXYZMatrix(const double2 &white, const double2 &red, const double2 &green, const double2 &blue) {
    QMatrix4x4 chromaticity;
    chromaticity.setRow(0, QVector4D(red.x, green.x, blue.x, 0.0));
    chromaticity.setRow(1, QVector4D(red.y, green.y, blue.y, 0.0));
    chromaticity.setRow(2, QVector4D(1.0 - red.x - red.y, 1.0 - green.x - green.y, 1.0 - blue.x - blue.y, 0.0));
    chromaticity.setRow(3, QVector4D(0.0, 0.0, 0.0, 1.0));

    // Calculate scaling factors to match the reference white point
    QVector3D whitePoint(white.x / white.y, 1.0, (1.0 - white.x - white.y) / white.y);
    QVector3D scale = chromaticity.inverted().mapVector(whitePoint);

    QMatrix4x4 scaleMatrix;
    scaleMatrix.scale(scale.x(), scale.y(), scale.z());

    return chromaticity * scaleMatrix;
}

// Helper: Apply gamma correction
QVector3D ImageSpaceConverter::applyGammaCorrection(const QColor &color, double gamma) {
    auto applyGamma = [gamma](double value) -> double {
        return (value <= 0.04045) ? value / 12.92 : std::pow((value + 0.055) / 1.055, gamma);
    };
    return QVector3D(
            applyGamma(color.redF()),
            applyGamma(color.greenF()),
            applyGamma(color.blueF())
    );
}

// Helper: Apply inverse gamma correction
QColor ImageSpaceConverter::applyInverseGammaCorrection(const QVector3D &color, double gamma) {
    auto inverseGamma = [gamma](double value) -> double {
        return (value <= 0.0031308) ? value * 12.92 : 1.055 * std::pow(value, 1.0 / gamma) - 0.055;
    };
    return QColor::fromRgbF(
            inverseGamma(color.x()),
            inverseGamma(color.y()),
            inverseGamma(color.z())
    );
}

// Helper: Transform color between RGB spaces
QVector3D ImageSpaceConverter::transformColor(const QVector3D &color, const QMatrix4x4 &sourceRGBtoXYZ, const QMatrix4x4 &targetXYZtoRGB) {
    QVector3D xyz = sourceRGBtoXYZ.mapVector(color);
    QVector3D targetRGB = targetXYZtoRGB.mapVector(xyz);
    return targetRGB;
}

QVector3D ImageSpaceConverter::adjustWhitePoint(const QVector3D &color, const double2 &sourceWhite, const double2 &targetWhite) {
    // Bradford transformation matrix
    QMatrix4x4 bradford;
    bradford.setRow(0, QVector4D(0.8951,  0.2664, -0.1614, 0.0));
    bradford.setRow(1, QVector4D(-0.7502,  1.7135,  0.0367, 0.0));
    bradford.setRow(2, QVector4D(0.0389, -0.0685,  1.0296, 0.0));
    bradford.setRow(3, QVector4D(0.0,     0.0,     0.0,    1.0));

    QMatrix4x4 inverseBradford = bradford.inverted();

    QVector3D sourceWhiteXYZ(sourceWhite.x / sourceWhite.y, 1.0, (1.0 - sourceWhite.x - sourceWhite.y) / sourceWhite.y);
    QVector3D targetWhiteXYZ(targetWhite.x / targetWhite.y, 1.0, (1.0 - targetWhite.x - targetWhite.y) / targetWhite.y);

    QVector3D sourceCone = bradford.mapVector(sourceWhiteXYZ);
    QVector3D targetCone = bradford.mapVector(targetWhiteXYZ);

    QVector3D adaptationScale(targetCone.x() / sourceCone.x(), targetCone.y() / sourceCone.y(), targetCone.z() / sourceCone.z());

    QMatrix4x4 adaptationMatrix;
    adaptationMatrix.scale(adaptationScale);

    QVector3D adaptedColor = inverseBradford.mapVector(adaptationMatrix.mapVector(bradford.mapVector(color)));

    return adaptedColor;
}

// Conversion: Absolute Colorimetric
QImage ImageSpaceConverter::convertAbsoluteColorimetric(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                                        const ColorProfileSettings &targetProfile) {
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);

    QMatrix4x4 sourceRGBtoXYZ = computeRGBtoXYZMatrix(sourceProfile.white, sourceProfile.red, sourceProfile.green, sourceProfile.blue);
    QMatrix4x4 targetRGBtoXYZ = computeRGBtoXYZMatrix(targetProfile.white, targetProfile.red, targetProfile.green, targetProfile.blue);
    QMatrix4x4 targetXYZtoRGB = targetRGBtoXYZ.inverted();

    for (int y = 0; y < resultImage.height(); ++y) {
        for (int x = 0; x < resultImage.width(); ++x) {
            QColor color = resultImage.pixelColor(x, y);
            QVector3D linearRGB = applyGammaCorrection(color, sourceProfile.gamma);
            QVector3D targetRGB = transformColor(linearRGB, sourceRGBtoXYZ, targetXYZtoRGB);
            resultImage.setPixelColor(x, y, applyInverseGammaCorrection(targetRGB, targetProfile.gamma));
        }
    }

    return resultImage;
}

// Conversion: Relative Colorimetric
QImage ImageSpaceConverter::convertRelativeColorimetric(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                                        const ColorProfileSettings &targetProfile) {
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);

    QMatrix4x4 sourceRGBtoXYZ = computeRGBtoXYZMatrix(sourceProfile.white, sourceProfile.red, sourceProfile.green, sourceProfile.blue);
    QMatrix4x4 targetRGBtoXYZ = computeRGBtoXYZMatrix(targetProfile.white, targetProfile.red, targetProfile.green, targetProfile.blue);
    QMatrix4x4 targetXYZtoRGB = targetRGBtoXYZ.inverted();

    for (int y = 0; y < resultImage.height(); ++y) {
        for (int x = 0; x < resultImage.width(); ++x) {
            QColor color = resultImage.pixelColor(x, y);
            QVector3D linearRGB = applyGammaCorrection(color, sourceProfile.gamma);
            QVector3D adjustedRGB = adjustWhitePoint(linearRGB, sourceProfile.white, targetProfile.white);
            QVector3D targetRGB = transformColor(adjustedRGB, sourceRGBtoXYZ, targetXYZtoRGB);
            resultImage.setPixelColor(x, y, applyInverseGammaCorrection(targetRGB, targetProfile.gamma));
        }
    }

    return resultImage;
}

// Conversion: Perceptual
QImage ImageSpaceConverter::convertPerceptual(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                              const ColorProfileSettings &targetProfile) {
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);

    // Use Absolute Colorimetric as a base and then perform gamut compression
    resultImage = convertAbsoluteColorimetric(sourceImage, sourceProfile, targetProfile);

    // Apply perceptual adjustments (e.g., clipping or scaling)
    for (int y = 0; y < resultImage.height(); ++y) {
        for (int x = 0; x < resultImage.width(); ++x) {
            QColor color = resultImage.pixelColor(x, y);
            QVector3D rgb(color.redF(), color.greenF(), color.blueF());

            // Compress to fit gamut
            rgb = rgb.normalized();  // Example logic (replace with a proper gamut mapping algorithm)
            resultImage.setPixelColor(x, y, QColor::fromRgbF(rgb.x(), rgb.y(), rgb.z()));
        }
    }

    return resultImage;
}

// Conversion: Saturation
QImage ImageSpaceConverter::convertSaturation(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                              const ColorProfileSettings &targetProfile) {
    QImage resultImage = sourceImage.convertToFormat(QImage::Format_RGB32);

    // Perform base conversion using Absolute Colorimetric
    resultImage = convertAbsoluteColorimetric(sourceImage, sourceProfile, targetProfile);

    for (int y = 0; y < resultImage.height(); ++y) {
        for (int x = 0; x < resultImage.width(); ++x) {
            QColor color = resultImage.pixelColor(x, y);

            QVector3D rgb(color.redF(), color.greenF(), color.blueF());
            float h, s, l;
            rgbToHsl(rgb, h, s, l);

            // Modify saturation (example: scale by 1.2, clamp to [0, 1])
            s = std::min(1.0f, s * 1.2f);

            QVector3D adjustedRgb = hslToRgb(h, s, l);
            resultImage.setPixelColor(x, y, QColor::fromRgbF(adjustedRgb.x(), adjustedRgb.y(), adjustedRgb.z()));
        }
    }

    return resultImage;
}

// Helper function: Convert RGB to HSL
void ImageSpaceConverter::rgbToHsl(const QVector3D &rgb, float &h, float &s, float &l) {
    float r = rgb.x();
    float g = rgb.y();
    float b = rgb.z();

    float maxVal = std::max({r, g, b});
    float minVal = std::min({r, g, b});
    l = (maxVal + minVal) / 2.0f;

    if (maxVal == minVal) {
        h = s = 0.0f; // Achromatic
    } else {
        float delta = maxVal - minVal;
        s = (l > 0.5f) ? delta / (2.0f - maxVal - minVal) : delta / (maxVal + minVal);

        if (maxVal == r) {
            h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
        } else if (maxVal == g) {
            h = (b - r) / delta + 2.0f;
        } else {
            h = (r - g) / delta + 4.0f;
        }
        h /= 6.0f;
    }
}

// Helper function: Convert HSL to RGB
QVector3D ImageSpaceConverter::hslToRgb(float h, float s, float l) {
    auto hueToRgb = [](float p, float q, float t) -> float {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f) return q;
        if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
        return p;
    };

    float r, g, b;
    if (s == 0.0f) {
        r = g = b = l; // Achromatic
    } else {
        float q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hueToRgb(p, q, h + 1.0f / 3.0f);
        g = hueToRgb(p, q, h);
        b = hueToRgb(p, q, h - 1.0f / 3.0f);
    }

    return QVector3D(r, g, b);
}
