#pragma once

#include <memory>


namespace ShMem
{
    
    #define PNG_COLOR_TYPE_GRAY 0
    #define PNG_COLOR_TYPE_RGB  2


    
int save_image_to_pgm_file(const char * file_name, const unsigned char * image, int width, int height);
int save_image_to_ppm_file(const char * file_name, const unsigned char * rgb, int width, int height);
int save_image_to_psm_file(const char * file_name, const unsigned short * image, int width, int height, unsigned long maxVal = 0xffff);

bool writeRgb(const char * fileName, int width, int height, const unsigned char * rgb, int compressionLevel);
bool writeBgra(const char * fileName, int width, int height, const unsigned char * bgra, int compressionLevel);
bool writeLum8(const char * fileName, int width, int height, const unsigned char * lum, int compressionLevel);
bool writeLum16(const char * fileName, int width, int height, const unsigned short * lum, int compressionLevel);

// Test support
void test_disable_shmemio();
void test_enable_shmemio();
bool test_is_shmemio_enabled();
}
