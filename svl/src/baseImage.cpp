#include "otherIO/lifImage.hpp"
#include "otherIO/filetools.h"
#include "otherIO/baseImage.h"
#include <mutex>
#include <cmath>

std::shared_ptr<baseImage> baseImage::open(const std::string& fileName)
{
    std::string ext = svl::extractFileExtension(fileName);
    std::shared_ptr<baseImage> rtn;
    if (ext == "lif") {
        
        std::shared_ptr<baseImage> img (new lifImage);
        if (img->initialize(fileName))
        {
            rtn = img;
        }
        
    }
    return rtn;
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
  const unsigned long long& height, unsigned char*& data) {

    unsigned int nrSamples = getSamplesPerPixel();
    if (this->getDataType()==lif::Float) {
      float * temp = (float*)readDataFromImage(startX, startY, width, height);
      std::copy(temp, temp + width*height*nrSamples, data);
      delete[] temp;
    }
    else if (this->getDataType()==lif::UChar) {
      delete[] data;
      data = (unsigned char*)readDataFromImage(startX, startY, width, height);
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

baseImage::baseImage() :
  imageData(),
  _cacheSize(0),
  _cache()
{
  _cacheMutex.reset(new std::mutex());
  _openCloseMutex.reset(new std::mutex());
}




baseImage::~baseImage() {
    std::unique_lock<std::mutex> l(*_openCloseMutex);
  cleanup();
}

void baseImage::cleanup() {

  _spacing.clear();
  _samplesPerPixel = 0;

  _colorType = InvalidColorType;
  _dataType = InvalidDataType;
  _isValid = false;
}

const unsigned long long baseImage::getCacheSize() {
  unsigned long long cacheSize = 0;
  _cacheMutex->lock();
  if (_cache && _isValid) {
    if (_dataType == UInt32) {
      cacheSize = (std::static_pointer_cast<TileCache<unsigned int> >(_cache))->maxCacheSize();
    }
    else if (_dataType == UInt16) {
      cacheSize = (std::static_pointer_cast<TileCache<unsigned short> >(_cache))->maxCacheSize();
    }
    else if (_dataType == UChar) {
      cacheSize = (std::static_pointer_cast<TileCache<unsigned char> >(_cache))->maxCacheSize();
    }
    else if (_dataType == Float) {
      cacheSize = (std::static_pointer_cast<TileCache<float> >(_cache))->maxCacheSize();
    }
  _cacheMutex->unlock();
  }
  return cacheSize;
}

void baseImage::setCacheSize(const unsigned long long cacheSize) {
  _cacheMutex->lock();
  if (_cache && _isValid) {
    if (_dataType == UInt32) {
      (std::static_pointer_cast<TileCache<unsigned int> >(_cache))->setMaxCacheSize(cacheSize);
    }
    else if (_dataType == UInt16) {
      (std::static_pointer_cast<TileCache<unsigned short> >(_cache))->setMaxCacheSize(cacheSize);
    }
    else if (_dataType == UChar) {
      (std::static_pointer_cast<TileCache<unsigned char> >(_cache))->setMaxCacheSize(cacheSize);
    }
    else if (_dataType == Float) {
      (std::static_pointer_cast<TileCache<float> >(_cache))->setMaxCacheSize(cacheSize);
    }
  _cacheMutex->unlock();
  }
}
