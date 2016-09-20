#ifndef _baseImage
#define _baseImage
#include <string>
#include <memory>
#include <mutex>
#include "otherIO/lif.h"
#include "otherIO/TileCache.h"
#include "otherIO/imageData.h"
#include "otherIO/Patch.h"
#include "vision/roiWindow.h"



class baseImage : public imageData
{

public :
  baseImage();
  virtual ~baseImage();

  static std::shared_ptr<baseImage> open(const std::string& fileName);
    
  //! Load the image, returns whether a valid image is obtained
  virtual bool initialize(const std::string& imagePath) = 0;

  //! Gets/Sets the maximum size of the cache
  virtual const unsigned long long getCacheSize();
  virtual void setCacheSize(const unsigned long long cacheSize);

  //! Gets the minimum value for a channel. If no channel is specified, default to the first channel
  virtual double getMinValue(int channel = -1) = 0;
  
  //! Gets the maximum value for a channel. If no channel is specified, default to the first channel
  virtual double getMaxValue(int channel = -1) = 0;


  //! Obtains pixel data for a requested region. The user is responsible for allocating
  //! enough memory for the data to fit the array and clearing the memory. Please note that in case of int32 ARGB data,  
  //! like in OpenSlide, the order of the colors depends on the endianness of your machine (Windows typically BGRA)
  template <typename T> 
  void getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, const unsigned long long& height, T*& data)
    {

      unsigned int nrSamples = getSamplesPerPixel();
      if (this->getDataType()==lif::Float) {
        float * temp = (float*)readDataFromImage(startX, startY, width, height);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==lif::UChar) {
        unsigned char * temp = (unsigned char*)readDataFromImage(startX, startY, width, height );
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==lif::UInt16) {
        unsigned short * temp = (unsigned short*)readDataFromImage(startX, startY, width, height);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==lif::UInt32) {
        unsigned int * temp = (unsigned int*)readDataFromImage(startX, startY, width, height );
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
    }
    
    

protected :

  //! To make baseImage thread-safe
  std::unique_ptr<std::mutex> _openCloseMutex;
  std::unique_ptr<std::mutex> _cacheMutex;
  std::shared_ptr<void> _cache;
  std::vector<unsigned long long> _dims;

  // Properties of the loaded slide
  unsigned long long _cacheSize;
  std::string _fileType;

  // Cleans up internals
  virtual void cleanup();

  // Reads the actual data from the image
  virtual void* readDataFromImage(const long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height) = 0;

  template <typename T> void createCache() {
    if (_isValid) {
      _cache.reset(new TileCache<T>(_cacheSize));
    }
  }
};

template <> void  baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, unsigned char*& data);

template <> void  baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, unsigned short*& data);

template <> void  baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, unsigned int*& data);

template <> void  baseImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, float*& data);

#endif