#pragma once

#include <cstdint>
#include <memory>

class PngWriter
{
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    PngWriter();
    ~PngWriter();

    bool writeRgb(const char * fileName, int width, int height, const uint8_t * rgb, int compressionLevel);
    bool writeBgra(const char * fileName, int width, int height, const uint8_t * bgra, int compressionLevel);
    bool writeLum8(const char * fileName, int width, int height, const uint8_t * lum, int compressionLevel);
    bool writeLum16(const char * fileName, int width, int height, const uint16_t * lum, int compressionLevel);
};

class PngReader
{
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    PngReader();
    ~PngReader();

    bool readInfoAndPrep(const char * fileName, int * width, int * height, int * bitDepth, int * colorType);

    bool readRgb(const char * fileName, uint8_t * rgb);
    bool readLum8(const char * fileName, uint8_t * lum);
    bool readLum16(const char * fileName, uint16_t * lum);
};
