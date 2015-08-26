#pragma once


#ifdef _WIN32
#include <io.h>
#include <cstring>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif NOMINMAX
#include <windows.h>
#define fileops_open _open
#define fileops_close(fd) _close(fd)
#define fileops_lseek(fd, offset, origin) _lseek(fd, offset, origin)
#else
#include <sys/mman.h>
#define fileops_open open
#define fileops_close(fd) close(fd)
#define fileops_lseek(fd, offset, origin) lseek(fd, offset, origin)
#endif

#include <fstream>
#include <fcntl.h>
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <memory>
#include <exception>
#include <vector>
#include <cstdio>
#include <assert.h>
#include <vector>
#include <unistd.h>

#include "shmemIO.h"

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


using namespace ShMem;
using namespace std;


// This flag can be used for unit tests to keep track of calling shmemio or using native methods
static bool unittest_disable_shmemio = false;

void ShMem::test_disable_shmemio()
{
    unittest_disable_shmemio = true;
}
void ShMem::test_enable_shmemio()
{
    unittest_disable_shmemio = false;
}

bool ShMem::test_is_shmemio_enabled() { return !unittest_disable_shmemio; }

class shMemRawWriter
{
public:
    shMemRawWriter() {}
    ~shMemRawWriter() {}

    /** \brief Generate the header of file format

                  */
    template <typename PF, typename P = typename PF::pixel_type_t>
    int writeBinary(const char * file_name, const P * pixel_data, const PF & ifo);

    template <typename PF, typename P = typename PF::pixel_type_t>
    int writePNGBinary(const char * file_name, const P * pixel_data, PF & ifo);
};

// Check ppm and pgm version byte
static bool valid_verion_byte(char vb)
{
    return vb != '5' && vb != '6' && vb != '9';
}

/*
         * structure to store encoded PNG image bytes
         */
template <typename P, int C = 1>
struct mem_io_buffer
{
    typedef P pixel_type_t;
    char * buffer;
    size_t size;
};


// A Simple Pixel Traits + Image Information for ppt, pgm, psm and pgn
template <typename P, int C = 1>
class p_image_info
{
public:
    typedef P pixel_type_t;
    typedef mem_io_buffer<P, C> mem_io_buffer_t;

    p_image_info(int w, int h, int mv, int compress, int c_type, int8_t v_byte = 0, bool isbgr = false, bool hasalpha = false)
        : m_width(w), m_height(h), m_max_val(mv), m_compression(compress), m_color_type(c_type), m_bytes_per_pixel(sizeof(P)), m_pixels_per_channel(C), m_is_bgr(isbgr), m_has_alpha(hasalpha)
    {
        m_bytes_per_channel = m_bytes_per_pixel * m_pixels_per_channel;
        m_total_byte_size = width() * height() * m_bytes_per_channel;
        m_version_byte = v_byte;
    }

    // Image size in pixels from Pixel Traits
    unsigned long image_pixel_size() const
    {
        return width() * height();
    }
    // Image size in bytes from Pixel traits
    unsigned long image_byte_size() const
    {
        return static_cast<long>(m_total_byte_size);
    }

    void set_encoded_byte_size(int size)
    {
        m_encoded_byte_size = size;
    }

    int encoded_byte_size() const { return m_encoded_byte_size; }


    // Create header infor for ppm and ppt image formats
    std::string create_p_header() const
    {
        ostringstream header;
        header << "P" << m_version_byte << std::endl;
        header << width();
        header << " " << height() << std::endl << max_val() << std::endl;
        return header.str();
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int compression() const { return m_compression; }
    int color_type() const { return m_color_type; }
    int max_val() const { return m_max_val; }
    std::vector<__uint8_t*> & row_pointers() { return m_rowPtrs; }
    int bytes_per_pixel() const { return m_bytes_per_pixel; }
    int pixels_per_channel() const { return m_pixels_per_channel; }
    int bytes_per_channel() const { return m_bytes_per_channel; }
    bool has_alpha() const { return m_has_alpha; }
    bool is_bgr() const { return m_is_bgr; }

    char version_byte() const { return m_version_byte; }

private:
    // Format specific definitions
    char m_version_byte;
    int m_width;
    int m_height;
    int m_compression;
    int m_color_type;
    unsigned long m_max_val;
    size_t m_total_byte_size;
    int m_bytes_per_pixel;
    int m_pixels_per_channel;
    int m_bytes_per_channel;
    int m_encoded_byte_size;
    std::vector<__uint8_t*> m_rowPtrs;
    bool m_is_bgr;
    bool m_has_alpha;
};


template p_image_info<unsigned char>;
template p_image_info<unsigned char, 3>;
template p_image_info<unsigned short>;
template p_image_info<unsigned char, 4>;

typedef p_image_info<unsigned char> p_gray_char_info_t;
typedef p_image_info<unsigned char, 3> p_rgb_char_info_t;
typedef p_image_info<unsigned short> p_gray_short_info_t;
typedef p_image_info<unsigned char, 4> p_bgra_char_info_t;


int ShMem::save_image_to_pgm_file(const char * file_name, const unsigned char * image, int width, int height)
{
    p_image_info<unsigned char> pif(width, height, 255, 0, '5');

    shMemRawWriter writer;
    if (writer.writeBinary(file_name, image, pif) != 0)
    {
        return (2);
    }
    return (0);
}


int ShMem::save_image_to_ppm_file(const char * file_name, const unsigned char * image, int width, int height)
{
    p_image_info<unsigned char, 3> pif(width, height, 255, 0, PNG_COLOR_TYPE_RGB, '6');

    shMemRawWriter writer;
    if (writer.writeBinary(file_name, image, pif) != 0)
    {
        return (2);
    }
    return (0);
}


int ShMem::save_image_to_psm_file(const char * file_name, const unsigned short * image, int width, int height, unsigned long maxVal)
{
    p_image_info<unsigned short, 1> pif(width, height, maxVal, 0, PNG_COLOR_TYPE_GRAY, '9');
    shMemRawWriter writer;
    if (writer.writeBinary(file_name, image, pif) != 0)
    {
        return (2);
    }
    return (0);
}


///////////////////////////////////          PNG -> Memory ////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PNG_IMPL

bool ShMem::writeLum8(const char * fileName, int width, int height, const unsigned char * lum, int compressionLevel)
{
    p_gray_char_info_t pif(width, height, 255, compressionLevel, PNG_COLOR_TYPE_GRAY);
    
    shMemRawWriter writer;
    if (writer.writePNGBinary(fileName, lum, pif) != 0)
    {
        return false;
    }
    return true;
}

bool ShMem::writeRgb(const char * fileName, int width, int height, const unsigned char * rgb, int compressionLevel)
{
    p_rgb_char_info_t pif(width, height, 255, compressionLevel, PNG_COLOR_TYPE_RGB);
    
    shMemRawWriter writer;
    if (writer.writePNGBinary(fileName, rgb, pif) != 0)
    {
        return false;
    }
    return true;
}

bool ShMem::writeBgra(const char * fileName, int width, int height, const unsigned char * bgra, int compressionLevel)
{
    // Note for BGRA, we use RGBA and then use png_set_bge to switch
    p_bgra_char_info_t pif(width, height, 255, compressionLevel, PNG_COLOR_TYPE_RGB_ALPHA, 0, true, true);
    
    shMemRawWriter writer;
    if (writer.writePNGBinary(fileName, bgra, pif) != 0)
    {
        return false;
    }
    return true;
}

bool ShMem::writeLum16(const char * fileName, int width, int height, const unsigned short * lum, int compressionLevel)
{
    p_gray_short_info_t pif(width, height, 65535, compressionLevel, PNG_COLOR_TYPE_GRAY);
    
    shMemRawWriter writer;
    if (writer.writePNGBinary(fileName, lum, pif) != 0)
    {
        return false;
    }
    return true;
}



template <typename M>
void custom_writer(png_structp png_ptr, png_bytep data, png_size_t length)
{
    typedef typename M::mem_io_buffer_t mem_io_buffer_t;
    typedef typename mem_io_buffer_t::pixel_type_t pixel_type_t;

    mem_io_buffer_t * p = (mem_io_buffer_t *)png_get_io_ptr(png_ptr);
    assert(p != 0);
    assert(p->buffer != 0);

    /* copy new bytes to end of buffer */
    DS_MEMCPY(p->buffer + p->size, data, length);
    p->size += length;
}


template void custom_writer<p_gray_char_info_t>(png_structp png_ptr, png_bytep data, png_size_t length);
template void custom_writer<p_gray_short_info_t>(png_structp png_ptr, png_bytep data, png_size_t length);
template void custom_writer<p_rgb_char_info_t>(png_structp png_ptr, png_bytep data, png_size_t length);
template void custom_writer<p_bgra_char_info_t>(png_structp png_ptr, png_bytep data, png_size_t length);




template <typename PF, typename P>
static typename PF::mem_io_buffer_t save_png_to_mem(const P * data, PF & ifo)
{

    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    PF::mem_io_buffer_t state;


    // Allocate twice ( could be reduced ) as much and let png compressor use it up
    // This memory is eventually handled to a shared_ptr for auto-deletion
    state.buffer = new char[ifo.image_byte_size() * 2];
    state.size = 0;

    // set up png_ptr
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) return state;

    // set up info_ptr
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) return state;

    /*
    Error handling in libpng is done through png_error() and png_warning(). 
    Errors handled through png_error() are fatal, meaning that png_error() should never return to its caller. 
    Currently, this is handled via setjmp() and longjmp()
    (unless you have compiled libpng with PNG_SETJMP_NOT_SUPPORTED, in which case it is handled via PNG_ABORT()),
    but you could change this to do things like exit() if you should wish.
    */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return state;
    }

    int width = ifo.width();
    int height = ifo.height();
    ifo.row_pointers().resize(height);

    // Png requires one of two ways to initialize IO
    // 1. png_init_io call is the easiest way
    // 2. use custom write functions ( our case since we want to use shared Memory
    // Actual writing:
    // 1. Simple way png_write_png
    // 2. set IHDR. Set significance bits (optional) and color pallete ( for index color images )

    png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
    png_set_compression_level(png_ptr, ifo.compression());
    png_set_IHDR(png_ptr,
                 info_ptr,
                 (png_uint_32)width,
                 (png_uint_32)height,
                 (ifo.bytes_per_pixel() == 2) ? 16 : 8,
                 ifo.color_type(),
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    if (ifo.is_bgr())
        png_set_bgr(png_ptr);

    // Set the write function before we write anything out
    png_set_write_fn(png_ptr, &state, custom_writer<PF>, NULL);

    // Write the header
    png_write_info(png_ptr, info_ptr);


    /*
     * This line indicates that our date is little endian and therefore needs to be swapped in to 
     * libpng standard storage i.e. big endian ( Network Endian ) 
     * It is important when it gets called in this order of operations. ( after png_write_info )
     */
    if (system_endian_ordering == little_endian_ordering && ifo.bytes_per_pixel() == 2) // swap for 16bit
        png_set_swap(png_ptr);


    const char * pixels = (const char *)(data);
    for (int y = 0; y < height; ++y)
        ifo.row_pointers()[y] = (const png_bytep)(pixels + y * width * ifo.bytes_per_channel());

    /* the actual write */
    png_set_rows(png_ptr, info_ptr, ifo.row_pointers().data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* Finish writing. */
    png_destroy_write_struct(&png_ptr, &info_ptr);

    /* now state.buffer contains the PNG image of size s.size bytes */

    return state;
}

template <typename PF, typename P>
std::shared_ptr<char> create_png(const P * data, PF & info)
{
    PF::mem_io_buffer_t state = save_png_to_mem(data, info);
    info.set_encoded_byte_size((int)state.size);
    std::shared_ptr<char> png(state.buffer);
    return png;
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PF, typename P>
int shMemRawWriter::writeBinary(const char * file_name, const P * pixel_data, const PF & ifo)
{

    if (pixel_data == 0 || ifo.width() == 0 || ifo.height() == 0 || ifo.bytes_per_pixel() == 0 ||
        valid_verion_byte(ifo.version_byte()))
    {
        return (-1); // throw exception ?
    }
    std::string header = ifo.create_p_header();
    int data_idx = (int)header.length();

#if _WIN32
    HANDLE h_native_file = CreateFileA(file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h_native_file == INVALID_HANDLE_VALUE)
    {
        int er = GetLastError();
        //    TODO: Throw exception
        return (-1);
    }
#else
    int fd = fileops_open(file_name, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd < 0)
    {
        //    TODO: Throw exception
        return (-1);
    }
#endif


    auto data_size = ifo.image_byte_size();


// Prepare the map
#if _WIN32
    HANDLE fm = CreateFileMapping(h_native_file, NULL, PAGE_READWRITE, 0, data_idx + data_size, NULL);
    char * map = static_cast<char *>(MapViewOfFile(fm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, data_idx + data_size));
    CloseHandle(fm);

#else
    // Stretch the file size to the size of the data
    int result = fileops_lseek(fd, getpagesize() + data_size - 1, SEEK_SET);
    if (result < 0)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }
    // Write a bogus entry so that the new file size comes in effect
    result = ::write(fd, "", 1);
    if (result != 1)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }

    char * map = (char *)mmap(0, data_idx + data_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }
#endif

    // Copy the header
    std::memcpy(&map[0], header.c_str(), data_idx);

    char * out = &map[0] + data_idx;
    // Copy the data
    std::memcpy (out, pixel_data, data_size);


// Unmap the pages of memory
#if _WIN32
    UnmapViewOfFile(map);
#else
    if (munmap(map, (data_idx + data_size)) == -1)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }
#endif
// Close file
#if _WIN32
    CloseHandle(h_native_file);
#else
    fileops_close(fd);
#endif
    return (0);
}


template <typename PF, typename P>
int shMemRawWriter::writePNGBinary(const char * file_name, const P * pixel_data, PF & ifo)
{

    if (pixel_data == 0 || ifo.width() == 0 || ifo.height() == 0) return (-1); // throw exception ?

    std::shared_ptr<char> encoded = create_png(pixel_data, ifo);
    char * cargo = encoded.get();
    if (cargo == 0) return (-1); // throw exception ?
    int data_size = ifo.encoded_byte_size();

#if _WIN32
    HANDLE h_native_file = CreateFileA(file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h_native_file == INVALID_HANDLE_VALUE)
    {
        int er = GetLastError();
        //    TODO: Throw exception
        return (-1);
    }
    HANDLE fm = CreateFileMapping(h_native_file, NULL, PAGE_READWRITE | SEC_RESERVE /* | SEC_COMMIT | SEC_LARGE_PAGES */, 0, ifo.encoded_byte_size(), NULL);
    char * map = static_cast<char *>(MapViewOfFile(fm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, ifo.encoded_byte_size()));
    CloseHandle(fm);
#else
    int fd = fileops_open(file_name, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd < 0)
    {
        //    TODO: Throw exception
        return (-1);
    }
    // Stretch the file size to the size of the data
    int result = fileops_lseek(fd, getpagesize() + data_size - 1, SEEK_SET);
    if (result < 0)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }
    // Write a bogus entry so that the new file size comes in effect
    result = ::write(fd, "", 1);
    if (result != 1)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }

    char * map = (char *)mmap(0, data_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }
#endif

    char * out = &map[0];
    // Copy the data
    std::memcpy(out, cargo, ifo.encoded_byte_size());
// Unmap the pages of memory
#if _WIN32
    UnmapViewOfFile(map);
#else
    if (munmap(map, (data_size)) == -1)
    {
        fileops_close(fd);
        //    TODO: Throw exception
        return (-1);
    }
#endif
// Close file
#if _WIN32
    CloseHandle(h_native_file);
#else
    fileops_close(fd);
#endif
    return (0);
}


#if 0
void png_write_to_file(boost::shared_array<char> png, size_t png_size, const char* filename)
{

    // Open file
    std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    
    // Ensure that file is open
    if(!file.is_open())
    {
        die("png_write_to_file()", "Can't open file!");
    }

    // Write to file
    file.write(png.get(), png_size);
    
    // Close file
    file.close();
}
#endif
