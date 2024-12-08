//
// Created by wookie on 12/8/24.
//

#ifndef IMAGEPROFILECONVERTER_COMMONPROFILES_H
#define IMAGEPROFILECONVERTER_COMMONPROFILES_H

#include "ColorProfileSettings.h"
class CommonProfiles {
public:
    static constexpr ColorProfileSettings sRGB = {
            2.2,
            {0.31273, 0.329020},
            {0.64, 0.33},
            {0.30, 0.60},
            {0.15, 0.06}
    };
    static constexpr ColorProfileSettings AdobeRGB = {
            2.2,
            {0.31273, 0.329020},
            {0.64, 0.33},
            {0.21, 0.71},
            {0.15, 0.06}
    };
    static constexpr ColorProfileSettings AppleRGB = {
            1.8,
            {0.31273, 0.329020},
            {0.625, 0.34},
            {0.28, 0.595},
            {0.155, 0.07}
    };
    static constexpr ColorProfileSettings CIERGB = {
            2.2,
            {0.31273, 0.329020},
            {0.735, 0.265},
            {0.274, 0.717},
            {0.167, 0.009}
    };
    static constexpr ColorProfileSettings WideGamutRGB = {
            2.2,
            {0.31273, 0.329020},
            {0.735, 0.265},
            {0.115, 0.826},
            {0.157, 0.018}
    };

    struct ColorProfileInfo {
        const char *name;
        const ColorProfileSettings &profile;
    };

    constexpr static const ColorProfileInfo profiles[] = {
            {"sRGB", sRGB},
            {"Adobe RGB", AdobeRGB},
            {"Apple RGB", AppleRGB},
            {"CIERGB", CIERGB},
            {"Wide Gamut RGB", WideGamutRGB},
            {"Custom", {}}
    };
    static constexpr int profilesCount = sizeof(profiles) / sizeof(profiles[0]);
};


#endif //IMAGEPROFILECONVERTER_COMMONPROFILES_H
