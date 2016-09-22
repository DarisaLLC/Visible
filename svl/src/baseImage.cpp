#include "otherIO/lifImage.hpp"
#include "otherIO/filetools.h"
#include "otherIO/baseImage.h"
#include <mutex>
#include <cmath>


//
//void baseImage::swap(imageData& first, imageData& second) {
//    std::swap(first._colorType, second._colorType);
//    std::swap(first._dataType, second._dataType);
//    std::swap(first._spacing, second._spacing);
//    std::swap(first._samplesPerPixel, second._samplesPerPixel);
//    std::swap(first._isValid, second._isValid);
//}
//
const int baseImage::getSamplesPerPixel() const {
    if (_isValid) {
        return _samplesPerPixel;
    } else {
        return -1;
    }
}

const ColorType baseImage::getColorType() const {
    if (_isValid) {
        return _colorType;
    } else {
        return InvalidColorType;
    }
}

const std::vector<double> baseImage::getSpacing() const {
    return _spacing;
}

const cinder::ImageIo::DataType baseImage::getDataType() const {
    if (_isValid) {
        return _dataType;
    } else {
        return ci::ImageIo::DataType::DATA_UNKNOWN;
    }
}

#if 0
template<>
roiWindow<P8U> baseImage::getRoiWindow ()
{
    if (this->getDataType() != lif::UChar)
        return roiWindow<P8U> ();
    
    
}

// Subsequent specialization to not re-copy data when datatypes are the same
template <> void baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, 
  const unsigned long long& height, float*& data) {

    unsigned int nrSamples = getSamplesPerPixel();
    if (this->getDataType()==lif::Float) {
      delete[] data;
      data = (float*)readDataFromImage(startX, startY, width, height);
    }
    else if (this->getDataType()==lif::UChar) {
      unsigned char * temp = (unsigned char*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==lif::UInt16) {
      unsigned short * temp = (unsigned short*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==lif::UInt32) {
      unsigned int * temp = (unsigned int*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
}

template <> void baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, 
                                         const unsigned long long& height, std::shared_ptr<uint8_t>& data) {

    unsigned int nrSamples = getSamplesPerPixel();
    if (this->getDataType()==lif::Float) {
      float * temp = (float*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==lif::UChar) {
        data = std::shared_ptr<uint8_t> ((unsigned char*)readDataFromImage(startX, startY, width, height));
    }
    else if (this->getDataType()==lif::UInt16) {
      unsigned short * temp = (unsigned short*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==lif::UInt32) {
      unsigned int * temp = (unsigned int*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
}

template <> void baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, 
  const unsigned long long& height, unsigned short*& data) {
 
    unsigned int nrSamples = getSamplesPerPixel();
    if (this->getDataType()==DataType::Float) {
      float* temp = (float*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==DataType::UChar){
      unsigned char * temp = (unsigned char*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==DataType::UInt16) {
      delete[] data;
      data = (unsigned short*)readDataFromImage(startX, startY, width, height);
    }
    else if (this->getDataType()==lif::UInt32) {
      unsigned int* temp = (unsigned int*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
}

template <> void baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, 
  const unsigned long long& height,  unsigned int*& data) {
 
    unsigned int nrSamples = getSamplesPerPixel();
    if (this->getDataType()==DataType::Float) {
      float * temp = (float*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==DataType::UChar) {
      unsigned char * temp = (unsigned char*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==DataType::UInt16) {
      unsigned short * temp = (unsigned short*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==DataType::UInt32) {
      delete[] data;
      data = (unsigned int*)readDataFromImage(startX, startY, width, height);
    }
}

#endif

baseImage::baseImage() :

_spacing(),
_samplesPerPixel(0),
_colorType(InvalidColorType),
_dataType(ci::ImageIo::DataType::DATA_UNKNOWN),
_isValid(false)
{

}




baseImage::~baseImage() {

  cleanup();
}

void baseImage::cleanup() {

  _spacing.clear();
  _samplesPerPixel = 0;

  _colorType = InvalidColorType;
  _dataType = ci::ImageIo::DataType::DATA_UNKNOWN;
  _isValid = false;
}

