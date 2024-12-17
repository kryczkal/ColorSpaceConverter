# Color Profile Converter
![image](https://github.com/user-attachments/assets/b9d425f9-2e7d-4659-9988-dbb1a3d3d02e)

This project is a Qt-based application for converting images between different color spaces using various rendering intents (Absolute Colorimetric, Relative Colorimetric, Perceptual, and Saturation). It allows you to load a source image, select source and target color profiles (including well-known standards like sRGB and Adobe RGB), and then apply a chosen color space conversion. The application provides an out-of-gamut visualization option to highlight image regions that cannot be accurately represented in the target color space.

## Key Features

- Load images from common formats (PNG, JPG, BMP).
- Choose from built-in color profiles (sRGB, Adobe RGB, Apple RGB, CIERGB, Wide Gamut RGB) or specify custom ones.
- Adjust white points, gamma values, and the chromaticity coordinates of red, green, and blue primaries.
- Perform conversions using different intents:
  - Absolute Colorimetric
  - Relative Colorimetric
  - Perceptual
  - Saturation
- Display an out-of-gamut mask to identify colors that cannot be reproduced accurately in the target space.

## Project Structure

- **main.cpp**: Application entry point.
- **MainWindow.cpp/h**: The main UI class handling user interactions, loading images, saving output, and invoking conversions.
- **ImageSpaceConverter.cpp/h**: Core conversion logic and methods to compute transformations between color profiles.
- **CommonProfiles.h**: A set of common reference color profiles defined as static data.
- **ColorProfileSettings.h**: Defines structures for color profile parameters, including gamma and chromaticities.
- **CMakeLists.txt (if present)**: Build configuration for this project (if using CMake).

## Dependencies

- **Qt5 or Qt6**: For GUI and image processing support.
- **C++14 or newer**: Required language standard for modern C++ features.
- **A C++ Compiler and Build System**: e.g., GCC/Clang/MSVC and CMake/qmake.

## Building & Running

1. Install Qt and ensure `qmake` or `cmake` is available.
2. Clone the repository into a local directory:
   ```bash
   git clone https://github.com/kryczkal/ColorSpaceConverter
   cd ColorSpaceConverter
   ```

3. If using CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

   If using qmake:
   ```bash
   qmake .
   make
   ```

4. Run the generated executable:
   ```bash
   ./ColorProfileConverter
   ```

## Usage

- After launching the app:
  - Click **Load** to open an image.
  - Adjust source and target profile parameters using the UI controls.
  - Click **Convert** and select a conversion type from the dialog.
  - Toggle **Show Out of Gamut** to visualize unreproducible colors.
  - Click **Save** to export the converted image.

## Customization

You can modify or add new color profiles to `CommonProfiles.h`. Just define new sets of gamma and xy chromaticities, and add them to the profiles array. The UI will automatically list them.

## Known Limitations

- The perceptual intent is approximated by scaling XYZ values. For more accurate results, a dedicated perceptual mapping algorithm could be integrated.
- Color transformations assume no alpha channel adjustments; transparency is not fully addressed.

This tool demonstrates how to implement basic color conversions and is a starting point for more complex color management workflows.

