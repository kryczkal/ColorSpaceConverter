//
// Created by wookie on 12/8/24.
//

#ifndef IMAGEPROFILECONVERTER_IMAGESPACECONVERTER_H
#define IMAGEPROFILECONVERTER_IMAGESPACECONVERTER_H

#include "functional"
#include "unordered_map"

class ColorProfileSettings;
class QImage;
class QColor;
class double2;

enum class ConversionType{
    AbsoluteColorimetric,
    RelativeColorimetric,
    Perceptual,
    Saturation
};

class ImageSpaceConverter {
public:
    static QImage convert(const QImage &sourceImage, const ColorProfileSettings &sourceProfile, const ColorProfileSettings &targetProfile, ConversionType conversionType);
    static QImage convertAbsoluteColorimetric(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                              const ColorProfileSettings &targetProfile);

    static QImage convertRelativeColorimetric(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                              const ColorProfileSettings &targetProfile);

    static QImage convertPerceptual(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                    const ColorProfileSettings &targetProfile);

    static QImage convertSaturation(const QImage &sourceImage, const ColorProfileSettings &sourceProfile,
                                    const ColorProfileSettings &targetProfile);

    // Helper functions
    static QMatrix4x4 computeRGBtoXYZMatrix(const double2 &white, const double2 &red, const double2 &green, const double2 &blue);
    static QVector3D applyGammaCorrection(const QColor &color, double gamma);
    static QColor applyInverseGammaCorrection(const QVector3D &color, double gamma);
    static QVector3D transformColor(const QVector3D &color, const QMatrix4x4 &sourceRGBtoXYZ, const QMatrix4x4 &targetXYZtoRGB);
    static QVector3D adjustWhitePoint(const QVector3D &color, const double2 &sourceWhite, const double2 &targetWhite);
private:
    static std::unordered_map<ConversionType, std::function<QImage(const QImage &, const ColorProfileSettings &, const ColorProfileSettings &)> > conversionMethods;

    static void rgbToHsl(const QVector3D &rgb, float &h, float &s, float &l);

    static QVector3D hslToRgb(float h, float s, float l);
};


#endif //IMAGEPROFILECONVERTER_IMAGESPACECONVERTER_H
