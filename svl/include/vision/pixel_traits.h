#ifndef __Pixel__
#define __Pixel__


#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>
#include "core/core.hpp"
//#include <glut/glut.h>
#include "cinder/ImageIo.h"


using namespace std;
using namespace ci;


#ifdef min
#undef min
#endif
#pragma once
#ifdef max
#undef max
#endif

/*
 * The following definitions (until #endif)
 * is an extract from IPL headers.
 * Copyright (c) 1995 Intel Corporation.
 */
#define IPL_DEPTH_SIGN 0x80000000

#define IPL_DEPTH_1U 1
#define IPL_DEPTH_8U 8
#define IPL_DEPTH_16U 16
#define IPL_DEPTH_32F 32

#define IPL_DEPTH_8S (IPL_DEPTH_SIGN | 8)
#define IPL_DEPTH_16S (IPL_DEPTH_SIGN | 16)
#define IPL_DEPTH_32S (IPL_DEPTH_SIGN | 32)


#define DS_CN_SHIFT 3
#define DS_DEPTH_MAX (1 << DS_CN_SHIFT)

#define DS_8U 0
#define DS_8S 1
#define DS_16U 2
#define DS_16S 3
#define DS_32S 4
#define DS_32F 5
#define DS_64F 6
#define DS_USRTYPE1 7

#define DS_MAT_DEPTH_MASK (DS_DEPTH_MAX - 1)
#define DS_MAT_DEPTH(flags) ((flags)&DS_MAT_DEPTH_MASK)

#define DS_MAKETYPE(depth, cn) (DS_MAT_DEPTH(depth) + (((cn)-1) << DS_CN_SHIFT))
#define DS_MAKE_TYPE DS_MAKETYPE

#define DS_8UC1 DS_MAKETYPE(DS_8U, 1)
#define DS_8UC2 DS_MAKETYPE(DS_8U, 2)
#define DS_8UC3 DS_MAKETYPE(DS_8U, 3)
#define DS_8UC4 DS_MAKETYPE(DS_8U, 4)
#define DS_8UC(n) DS_MAKETYPE(DS_8U, (n))

#define DS_8SC1 DS_MAKETYPE(DS_8S, 1)
#define DS_8SC2 DS_MAKETYPE(DS_8S, 2)
#define DS_8SC3 DS_MAKETYPE(DS_8S, 3)
#define DS_8SC4 DS_MAKETYPE(DS_8S, 4)
#define DS_8SC(n) DS_MAKETYPE(DS_8S, (n))

#define DS_16UC1 DS_MAKETYPE(DS_16U, 1)
#define DS_16UC2 DS_MAKETYPE(DS_16U, 2)
#define DS_16UC3 DS_MAKETYPE(DS_16U, 3)
#define DS_16UC4 DS_MAKETYPE(DS_16U, 4)
#define DS_16UC(n) DS_MAKETYPE(DS_16U, (n))

#define DS_16SC1 DS_MAKETYPE(DS_16S, 1)
#define DS_16SC2 DS_MAKETYPE(DS_16S, 2)
#define DS_16SC3 DS_MAKETYPE(DS_16S, 3)
#define DS_16SC4 DS_MAKETYPE(DS_16S, 4)
#define DS_16SC(n) DS_MAKETYPE(DS_16S, (n))

#define DS_32SC1 DS_MAKETYPE(DS_32S, 1)
#define DS_32SC2 DS_MAKETYPE(DS_32S, 2)
#define DS_32SC3 DS_MAKETYPE(DS_32S, 3)
#define DS_32SC4 DS_MAKETYPE(DS_32S, 4)
#define DS_32SC(n) DS_MAKETYPE(DS_32S, (n))

#define DS_32FC1 DS_MAKETYPE(DS_32F, 1)
#define DS_32FC2 DS_MAKETYPE(DS_32F, 2)
#define DS_32FC3 DS_MAKETYPE(DS_32F, 3)
#define DS_32FC4 DS_MAKETYPE(DS_32F, 4)
#define DS_32FC(n) DS_MAKETYPE(DS_32F, (n))

#define DS_64FC1 DS_MAKETYPE(DS_64F, 1)
#define DS_64FC2 DS_MAKETYPE(DS_64F, 2)
#define DS_64FC3 DS_MAKETYPE(DS_64F, 3)
#define DS_64FC4 DS_MAKETYPE(DS_64F, 4)
#define DS_64FC(n) DS_MAKETYPE(DS_64F, (n))


typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

/*************************************************************/

/* Version */
// #define GL_VERSION_1_1                    1

/* DataType */
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_2_BYTES 0x1407
#define GL_3_BYTES 0x1408
#define GL_4_BYTES 0x1409
#define GL_DOUBLE 0x140A

/* PixelCopyType */
#define GL_COLOR 0x1800
#define GL_DEPTH 0x1801
#define GL_STENCIL 0x1802

/* PixelFormat */
#define GL_COLOR_INDEX 0x1900
#define GL_STENCIL_INDEX 0x1901
#define GL_DEPTH_COMPONENT 0x1902
#define GL_RED 0x1903
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A

namespace svl
{
    
    
    enum bayer_type
    {
        NoneBayer = -1,
        BG2 = 0,
        GB2 = 1,
        RG2 = 2,
        GR2 = 3
    };
    
    
    template <typename T, int Planes = 1, int Components = 1>
    struct dsPixelTraits
    {
        static bool is_floating_point() = 0;
        static bool is_integral() = 0;
        static bool is_signed() = 0;
        static int planes() = 0;
        static int components() = 0;
        static int cv() = 0;
        static int depth() = 0;
        static int bytes() = 0;
        static int bits() = 0;
        static int origin_style() = 0;
        static int data_order() = 0;
        static GLenum gl_type() = 0;
    };
    
    
    template <>
    struct dsPixelTraits<uint8_t, 1, 1>
    {
        typedef uint8_t value_type;
        
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<uint8_t>::value; }
        static bool is_signed() { return std::is_signed<uint8_t>::value; }
        static int planes() { return 1; }
        static int components() { return 1; }
        static int cv() { return IPL_DEPTH_8U; }
        static int depth() { return DS_8U; }
        static int bytes() { return 1; }
        static int bits() { return 8; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_UNSIGNED_BYTE; }
        static uint8_t minimum() { return numeric_limits<uint8_t>::min(); }
        static uint8_t maximum() { return numeric_limits<uint8_t>::max(); }
    };
    
    template <>
    struct dsPixelTraits<uint16_t, 1, 1>
    {
        typedef uint16_t value_type;
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<uint16_t>::value; }
        static bool is_signed() { return std::is_signed<uint16_t>::value; }
        static int planes() { return 1; }
        static int components() { return 1; }
        static int cv() { return IPL_DEPTH_16U; }
        static int depth() { return DS_16U; }
        static int bytes() { return 2; }
        static int bits() { return 16; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_UNSIGNED_SHORT; }
        static uint16_t minimum() { return numeric_limits<uint16_t>::min(); }
        static uint16_t maximum() { return numeric_limits<uint16_t>::max(); }
    };
    
    template <>
    struct dsPixelTraits<int8_t, 1, 1>
    {
        
        typedef int8_t value_type;
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<int8_t>::value; }
        static bool is_signed() { return std::is_signed<int8_t>::value; }
        static int planes() { return 1; }
        static int components() { return 1; }
        static int cv() { return IPL_DEPTH_8S; }
        static int depth() { return DS_8S; }
        static int bytes() { return 1; }
        static int bits() { return 8; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_BYTE; }
        static int8_t minimum() { return numeric_limits<int8_t>::min(); }
        static int8_t maximum() { return numeric_limits<int8_t>::max(); }
    };
    
    template <>
    struct dsPixelTraits<int16_t, 1, 1>
    {
        
        typedef int16_t value_type;
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<int16_t>::value; }
        static bool is_signed() { return std::is_signed<int16_t>::value; }
        static int planes() { return 1; }
        static int components() { return 1; }
        static int cv() { return IPL_DEPTH_16S; }
        static int depth() { return DS_16S; }
        static int bytes() { return 2; }
        static int bits() { return 16; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_SHORT; }
        static int16_t minimum() { return numeric_limits<int16_t>::min(); }
        static int16_t maximum() { return numeric_limits<int16_t>::max(); }
    };
    
    template <>
    struct dsPixelTraits<int32_t, 1, 1>
    {
        
        typedef int32_t value_type;
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<int32_t>::value; }
        static bool is_signed() { return std::is_signed<int32_t>::value; }
        static int planes() { return 1; }
        static int components() { return 1; }
        static int cv() { return IPL_DEPTH_32S; }
        static int depth() { return DS_32S; }
        static int bytes() { return 4; }
        static int bits() { return 32; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_INT; }
        static int32_t minimum() { return numeric_limits<int32_t>::min(); }
        static int32_t maximum() { return numeric_limits<int32_t>::max(); }
    };
    
    template <>
    struct dsPixelTraits<float, 1, 1>
    {
        
        typedef float value_type;
        static bool is_floating_point() { return true; }
        static bool is_integral() { return std::is_integral<float>::value; }
        static bool is_signed() { return std::is_signed<float>::value; }
        static int planes() { return 1; }
        static int components() { return 1; }
        static int cv() { return IPL_DEPTH_32F; }
        static int depth() { return DS_32F; }
        static int bytes() { return 4; }
        static int bits() { return 32; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_FLOAT; }
        static float minimum() { return numeric_limits<float>::min(); }
        static float maximum() { return numeric_limits<float>::max(); }
    };
    
    template <>
    struct dsPixelTraits<uint8_t, 1, 3>
    {
        
        typedef uint8_t value_type;
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<uint8_t>::value; }
        static bool is_signed() { return std::is_signed<uint8_t>::value; }
        static int planes() { return 1; }
        static int components() { return 3; }
        static int cv() { return IPL_DEPTH_8U; }
        static int depth() { return DS_8UC3; }
        static int bytes() { return 3; }
        static int bits() { return 24; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_RGB; }
        static uint8_t minimum() { return numeric_limits<uint8_t>::min(); }
        static uint8_t maximum() { return numeric_limits<uint8_t>::max(); }
    };
    
    template <>
    struct dsPixelTraits<uint8_t, 1, 4>
    {
        
        typedef uint8_t value_type;
        static bool is_floating_point() { return false; }
        static bool is_integral() { return std::is_integral<uint8_t>::value; }
        static bool is_signed() { return std::is_signed<uint8_t>::value; }
        static int planes() { return 1; }
        static int components() { return 4; }
        static int cv() { return IPL_DEPTH_8U; }
        static int depth() { return DS_8UC4; }
        static int bytes() { return 4; }
        static int bits() { return 32; }
        static int origin_style() { return 0; }
        static int data_order() { return 0; }
        static GLenum gl_type() { return GL_RGBA; }
        static uint8_t minimum() { return numeric_limits<uint8_t>::min(); }
        static uint8_t maximum() { return numeric_limits<uint8_t>::max(); }
    };
    
    typedef dsPixelTraits<int8_t, 1, 1> P8S;
    typedef dsPixelTraits<uint8_t, 1, 1> P8U;
    typedef dsPixelTraits<uint16_t, 1, 1> P16U;
    typedef dsPixelTraits<int16_t, 1, 1> P16S;
    typedef dsPixelTraits<int32_t, 1, 1> P32S;
    typedef dsPixelTraits<float, 1, 1> P32F;
    typedef dsPixelTraits<uint8_t, 1, 3> P8UC3;
    typedef dsPixelTraits<uint8_t, 1, 4> P8UC4;
    
    // Primary template class
    template <typename P>
    class PixelType;
    
    template <>
    class PixelType<P8U>
    {
    public:
        static enum ci::ImageIo::ColorModel cm () { return ci::ImageIo::ColorModel::CM_GRAY; }
        static enum ci::ImageIo::DataType ct () { return ci::ImageIo::DataType::UINT8; }
        static enum ci::ImageIo::ChannelOrder co () { return ci::ImageIo::ChannelOrder::Y; }
        
        
        typedef uint8_t pixel_t;
        typedef uint8_t * pixel_ptr_t;
        
        static GLenum data_type () { return GL_UNSIGNED_BYTE; }
        static GLenum display_type () { return GL_LUMINANCE; }
        
        typedef std::pair<pixel_t, uint16_t> run_t;
        
    };
    
    template <>
    class PixelType<P16U>
    {
    public:
        static enum ci::ImageIo::ColorModel cm () { return  ci::ImageIo::ColorModel::CM_GRAY; }
        static enum ci::ImageIo::DataType ct () { return ci::ImageIo::DataType::UINT16; }
        static enum ci::ImageIo::ChannelOrder co () { return ci::ImageIo::ChannelOrder::Y; }
        
        typedef uint16_t pixel_t;
        typedef uint16_t * pixel_ptr_t;
        static GLenum data_type () { return GL_UNSIGNED_SHORT; }
        static GLenum display_type () { return GL_LUMINANCE; }
        
        typedef std::pair<pixel_t, uint16_t> run_t;
        
    };
    
    template <>
    class PixelType<P32F>
    {
    public:
        static enum ci::ImageIo::ColorModel cm () {return  ci::ImageIo::ColorModel::CM_GRAY; }
        static enum ci::ImageIo::DataType ct () { return  ci::ImageIo::DataType::FLOAT32; }
        static enum ci::ImageIo::ChannelOrder co () { return ci::ImageIo::ChannelOrder::Y; }
        
        typedef float pixel_t;
        typedef float * pixel_ptr_t;
        static GLenum data_type () { return GL_FLOAT; }
        static GLenum display_type () { return GL_LUMINANCE; }
        
        typedef std::pair<pixel_t, uint16_t> run_t;
        
    };
    
    template <>
    class PixelType<P32S>
    {
    public:
        static enum ci::ImageIo::ColorModel cm () {return  ci::ImageIo::ColorModel::CM_GRAY; }
        static enum ci::ImageIo::DataType ct () { return  ci::ImageIo::DataType::DATA_UNKNOWN; }
        static enum ci::ImageIo::ChannelOrder co () { return ci::ImageIo::ChannelOrder::Y; }
        
        typedef int32_t pixel_t;
        typedef int32_t* pixel_ptr_t;
        static GLenum data_type () { return GL_FLOAT; }
        static GLenum display_type () { return GL_LUMINANCE; }
        
        typedef std::pair<pixel_t, uint16_t> run_t;
        
    };
    
    
    template <>
    class PixelType<P8UC3>
    {
    public:
        static enum ci::ImageIo::ColorModel cm () { return  ci::ImageIo::ColorModel::CM_RGB; }
        static enum ci::ImageIo::DataType ct () { return ci::ImageIo::DataType::UINT8; }
        static enum ci::ImageIo::ChannelOrder co () { return ci::ImageIo::ChannelOrder::RGB; }
        
        typedef uint8_t pixel_t;
        typedef uint8_t * pixel_ptr_t;
        static GLenum data_type () { return GL_UNSIGNED_BYTE; }
        static GLenum display_type () { return GL_RGB; }
        typedef std::pair<pixel_t, uint16_t> run_t;
        
    };
    
    
    template <>
    class PixelType<P8UC4>
    {
    public:
        static enum ci::ImageIo::ColorModel cm () { return  ci::ImageIo::ColorModel::CM_RGB; }
        static enum ci::ImageIo::DataType ct () { return ci::ImageIo::DataType::UINT8; }
        static enum ci::ImageIo::ChannelOrder co () { return ci::ImageIo::ChannelOrder::RGBX; }
        
        typedef uint8_t pixel_t;
        typedef uint8_t * pixel_ptr_t;
        static GLenum data_type () { return GL_UNSIGNED_BYTE; }
        static GLenum display_type () { return GL_RGB; }
        typedef std::pair<pixel_t, uint16_t> run_t;
        
    };
    
    
}


#endif
