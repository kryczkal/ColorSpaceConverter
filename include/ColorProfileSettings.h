//
// Created by wookie on 12/8/24.
//

#ifndef IMAGEPROFILECONVERTER_COLORPROFILESETTINGS_H
#define IMAGEPROFILECONVERTER_COLORPROFILESETTINGS_H

struct double2 {
    double x;
    double y;
};

struct ColorProfileSettings {
    double gamma = 1.0;
    double2 white = {0.0, 0.0};
    double2 red = {0.0, 0.0};
    double2 green = {0.0, 0.0};
    double2 blue = {0.0, 0.0};
};

#endif //IMAGEPROFILECONVERTER_COLORPROFILESETTINGS_H
