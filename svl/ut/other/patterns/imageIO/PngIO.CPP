#include "PngIO.h"
#ifdef _WIN32
#include "shmemIO.h"
#endif
#include <cstdlib>
#include <cassert>
#include <vector>

#include <zlib.h>
#include <png.h>
#include <pngconf.h>

#include "boost/detail/endian.hpp"

/**
 * Defines the different types of endian ordering.
 */
enum endian_ordering
{
    little_endian_ordering,
    big_endian_ordering,
#ifdef BOOST_LITTLE_ENDIAN
    system_endian_ordering = little_endian_ordering,
#endif
#ifdef BOOST_BIG_ENDIAN
    system_endian_ordering = big_endian_ordering,
#endif
};


#define PNG_BYTES_TO_CHECK 4

struct PngWriter::Impl
{
    int m_width;
    int m_height;
    std::vector<png_byte *> m_rowPtrs;

    void resize(int width, int height)
    {
        m_width = width;
        m_height = height;
        m_rowPtrs.resize(height);
    }

    bool write(const char * fileName, const void * image, int bitDepth, int colorType, int compressionLevel);
};


bool PngWriter::writeBgra(const char * fileName, int width, int height, const uint8_t * bgra, int compressionLevel)
{
#ifdef WIN32
    if (ShMem::test_is_shmemio_enabled())
        return ShMem::writeBgra(fileName, width, height, bgra, compressionLevel);
    else
    {
        // No non-shared_memory implementation
        //impl->resize(width, height);
        //return impl->write(fileName, bgra, 8, PNG_COLOR_TYPE_RGB, compressionLevel);
        assert(0);
        return false; // make compiler happy
    }
#else
    // No Linux implementation
    assert(0);
    return false; // make compiler happy
#endif
}

PngWriter::PngWriter()
    : impl(new Impl) {}
PngWriter::~PngWriter() {}

bool PngWriter::writeRgb(const char * fileName, int width, int height, const uint8_t * rgb, int compressionLevel)
{
#ifdef WIN32
    if (ShMem::test_is_shmemio_enabled())
        return ShMem::writeRgb(fileName, width, height, rgb, compressionLevel);
    else
    {
        impl->resize(width, height);
        return impl->write(fileName, rgb, 8, PNG_COLOR_TYPE_RGB, compressionLevel);
    }
#else
    impl->resize(width, height);
    return impl->write(fileName, rgb, 8, PNG_COLOR_TYPE_RGB, compressionLevel);
#endif
}


bool PngWriter::writeLum8(const char * fileName, int width, int height, const uint8_t * lum, int compressionLevel)
{
#ifdef WIN32
    if (ShMem::test_is_shmemio_enabled())
        return ShMem::writeLum8(fileName, width, height, lum, compressionLevel);
    else
    {
        impl->resize(width, height);
        return impl->write(fileName, lum, 8, PNG_COLOR_TYPE_GRAY, compressionLevel);
    }
#else
    impl->resize(width, height);
    return impl->write(fileName, lum, 8, PNG_COLOR_TYPE_GRAY, compressionLevel);
#endif
}

bool PngWriter::writeLum16(const char * fileName, int width, int height, const uint16_t * lum, int compressionLevel)
{
#ifdef WIN32
    if (ShMem::test_is_shmemio_enabled())
        return ShMem::writeLum16(fileName, width, height, lum, compressionLevel);
    impl->resize(width, height);
    return impl->write(fileName, lum, 16, PNG_COLOR_TYPE_GRAY, compressionLevel);
#else
    impl->resize(width, height);
    return impl->write(fileName, lum, 16, PNG_COLOR_TYPE_GRAY, compressionLevel);
#endif
}


bool PngWriter::Impl::write(const char * fileName, const void * image, int bitDepth, int colorType, int compressionLevel)
{
    png_structp pngPtr;
    png_infop infoPtr;
    FILE * fp = nullptr;
    if ((pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr)
    {
        return false;
    }
    if ((infoPtr = png_create_info_struct(pngPtr)) == nullptr)
    {
        png_destroy_write_struct(&pngPtr, &infoPtr);
        return false;
    }
    if ((fp = fopen(fileName, "wb")) == nullptr)
    {
        png_destroy_write_struct(&pngPtr, &infoPtr);
        return false;
    }
    png_init_io(pngPtr, fp);
    png_set_filter(pngPtr, 0, PNG_FILTER_NONE);
    png_set_compression_level(pngPtr, compressionLevel);
    png_set_IHDR(pngPtr, infoPtr, m_width, m_height, bitDepth, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(pngPtr, infoPtr);

    /*
     * This line indicates that our date is little endian and therefore needs to be swapped in to 
     * libpng standard storage i.e. big endian ( Network Endian ) 
     * It is important when it gets called in this order of operations. ( after png_write_info )
     */
    if (system_endian_ordering == little_endian_ordering && bitDepth == 16) // swap for 16bit
        png_set_swap(pngPtr);

    if (colorType == PNG_COLOR_TYPE_RGB && bitDepth == 8)
    {
        const uint8_t * rgb = static_cast<const uint8_t *>(image);
        for (int y = 0; y < m_height; ++y)
            m_rowPtrs[y] = (const png_bytep)(rgb + y * m_width * 3);
    }
    else if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth == 8)
    {
        const uint8_t * lum = static_cast<const uint8_t *>(image);
        for (int y = 0; y < m_height; ++y)
            m_rowPtrs[y] = (const png_bytep)(lum + y * m_width);
    }
    else if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth == 16)
    {
        const uint16_t * lum = static_cast<const uint16_t *>(image);
        for (int y = 0; y < m_height; ++y)
            m_rowPtrs[y] = (const png_bytep)(lum + y * m_width);
    }
    else
    {
        assert(false);
    }
    png_write_image(pngPtr, m_rowPtrs.data());
    png_write_end(pngPtr, infoPtr);
    png_destroy_write_struct(&pngPtr, &infoPtr);
    fclose(fp);
    return true;
}

struct PngReader::Impl
{
    int m_expectedWidth;
    int m_expectedHeight;
    int m_expectedBitDepth;
    int m_expectedColorType;
    std::vector<png_byte *> m_rowPtrs;
    FILE * m_fp;

    ~Impl() { close(); }
    bool readHeader(const char * fileName, png_structp & pngPtr, png_infop & infoPtr, int * width, int * height, int * bitDepth, int * colorType);
    bool readRest(png_structp & pngPtr, png_infop & infoPtr, int bitDepth, int colorType, void * image);
    void close()
    {
        if (m_fp)
        {
            fclose(m_fp);
            m_fp = nullptr;
        }
    }
};

PngReader::PngReader()
    : impl(new Impl)
{
    impl->m_expectedWidth = impl->m_expectedHeight = impl->m_expectedBitDepth = impl->m_expectedColorType = 0;
    impl->m_fp = nullptr;
}

PngReader::~PngReader() {}

bool PngReader::readInfoAndPrep(const char * fileName, int * width, int * height, int * bitDepth, int * colorType)
{
    png_structp pngPtr;
    png_infop infoPtr;
    if (!impl->readHeader(fileName, pngPtr, infoPtr, width, height, bitDepth, colorType))
    {
        return false;
    }
    impl->m_expectedWidth = *width;
    impl->m_expectedHeight = *height;
    impl->m_expectedBitDepth = *bitDepth;
    impl->m_expectedColorType = *colorType;
    impl->m_rowPtrs.resize(*height);
    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
    impl->close();
    return true;
}

bool PngReader::readRgb(const char * fileName, uint8_t * rgb)
{
    png_structp pngPtr;
    png_infop infoPtr;
    int width, height, bitDepth, colorType;
    if (!impl->readHeader(fileName, pngPtr, infoPtr, &width, &height, &bitDepth, &colorType))
    {
        return false;
    }
    if (width != impl->m_expectedWidth || height != impl->m_expectedHeight ||
        bitDepth != impl->m_expectedBitDepth || bitDepth != 8 ||
        colorType != impl->m_expectedColorType || colorType != PNG_COLOR_TYPE_RGB)
    {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        impl->close();
        return false;
    }
    if (!impl->readRest(pngPtr, infoPtr, 8, PNG_COLOR_TYPE_RGB, rgb))
    {
        return false;
    }
    return true;
}

bool PngReader::readLum8(const char * fileName, uint8_t * lum)
{
    png_structp pngPtr;
    png_infop infoPtr;
    int width, height, bitDepth, colorType;
    if (!impl->readHeader(fileName, pngPtr, infoPtr, &width, &height, &bitDepth, &colorType))
    {
        return false;
    }
    if (width != impl->m_expectedWidth || height != impl->m_expectedHeight ||
        bitDepth != impl->m_expectedBitDepth || bitDepth != 8 ||
        colorType != impl->m_expectedColorType || colorType != PNG_COLOR_TYPE_GRAY)
    {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        impl->close();
        return false;
    }
    if (!impl->readRest(pngPtr, infoPtr, 8, PNG_COLOR_TYPE_GRAY, lum))
    {
        return false;
    }
    return true;
}

bool PngReader::readLum16(const char * fileName, uint16_t * lum)
{
    png_structp pngPtr;
    png_infop infoPtr;
    int width, height, bitDepth, colorType;
    if (!impl->readHeader(fileName, pngPtr, infoPtr, &width, &height, &bitDepth, &colorType))
    {
        return false;
    }
    if (width != impl->m_expectedWidth || height != impl->m_expectedHeight ||
        bitDepth != impl->m_expectedBitDepth || bitDepth != 16 ||
        colorType != impl->m_expectedColorType || colorType != PNG_COLOR_TYPE_GRAY)
    {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        impl->close();
        return false;
    }
    if (!impl->readRest(pngPtr, infoPtr, 16, PNG_COLOR_TYPE_GRAY, lum))
    {
        return false;
    }
    return true;
}

bool PngReader::Impl::readHeader(const char * fileName, png_structp & pngPtr, png_infop & infoPtr, int * width, int * height, int * bitDepth, int * colorType)
{
    char buf[PNG_BYTES_TO_CHECK];
    int interlaceType;
    unsigned int sigRead = 0;
    if ((m_fp = fopen(fileName, "rb")) == nullptr)
    {
        return false;
    }
    if (fread((void *)buf, 1, PNG_BYTES_TO_CHECK, m_fp) != PNG_BYTES_TO_CHECK)
    {
        close();
        return false;
    }
    if (png_sig_cmp((png_const_bytep)buf, (png_size_t)0, PNG_BYTES_TO_CHECK))
    {
        close();
        return false;
    }
    sigRead = PNG_BYTES_TO_CHECK;
    if ((pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr)
    {
        close();
        return false;
    }
    if ((infoPtr = png_create_info_struct(pngPtr)) == nullptr)
    {
        close();
        png_destroy_read_struct(&pngPtr, nullptr, nullptr);
        return false;
    }
    if (setjmp(png_jmpbuf(pngPtr)))
    {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        close();
        return false;
    }
    png_init_io(pngPtr, m_fp);
    png_set_sig_bytes(pngPtr, sigRead);
    png_read_info(pngPtr, infoPtr);
    png_get_IHDR(pngPtr, infoPtr, (png_uint_32 *)width, (png_uint_32 *)height, bitDepth, colorType, &interlaceType, nullptr, nullptr);

    /*
     * This line indicates that our date is little endian and therefore needs to be swapped in to 
     * libpng standard storage i.e. big endian ( Network Endian ) 
     * It is important when it gets called in this order of operations. ( after get_IHDR)
     */
    if (system_endian_ordering == little_endian_ordering && *bitDepth == 16) // swap for 16bit
        png_set_swap(pngPtr);

    return true;
}

bool PngReader::Impl::readRest(png_structp & pngPtr, png_infop & infoPtr, int bitDepth, int colorType, void * image)
{
    if (colorType == PNG_COLOR_TYPE_RGB && bitDepth == 8)
    {
        const uint8_t * rgb = static_cast<const uint8_t *>(image);
        for (int y = 0; y < m_expectedHeight; ++y)
            m_rowPtrs[y] = (const png_bytep)(rgb + y * m_expectedWidth * 3);
    }
    else if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth == 8)
    {
        const uint8_t * lum = static_cast<const uint8_t *>(image);
        for (int y = 0; y < m_expectedHeight; ++y)
            m_rowPtrs[y] = (const png_bytep)(lum + y * m_expectedWidth);
    }
    else if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth == 16)
    {
        const uint16_t * lum = static_cast<const uint16_t *>(image);
        for (int y = 0; y < m_expectedHeight; ++y)
            m_rowPtrs[y] = (const png_bytep)(lum + y * m_expectedWidth);
    }
    else
    {
        return false;
    }
    png_read_image(pngPtr, m_rowPtrs.data());
    png_read_end(pngPtr, infoPtr);
    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
    close();
    return true;
}
