#ifndef _baseImage
#define _baseImage
#include <string>
#include <memory>
#include <mutex>
#include "otherIO/lif.h"
#include "vision/roiWindow.h"



class baseImage 
{

public :
  baseImage();
  virtual ~baseImage();

    
    virtual bool valid() const {return _isValid;}
    
    //! Gets the dimensions of the base level of the pyramid
    virtual const tpair<uint32_t> getDimensions() const = 0;
    
    //! Returns the color type
    virtual const lif::ColorType getColorType() const;
    
    //! Returns the data type
    virtual const cinder::ImageIo::DataType getDataType() const;
    
    //! Returns the number of samples per pixel
    virtual const int getSamplesPerPixel() const;
    
    //! Returns the pixel spacing meta-data (um per pixel)
    virtual const std::vector<double> getSpacing() const;
    
    //! Gets the minimum value for a channel. If no channel is specified, default to overall minimum
    virtual double getMinValue(int channel = -1) = 0;
    
    //! Gets the maximum value for a channel. If no channel is specified, default to overall maximum
    virtual double getMaxValue(int channel = -1) = 0;
    
//    void swap(imageData& first, imageData& second);
    
    //! Load the image, returns whether a valid image is obtained
  virtual bool initialize(const std::string& imagePath) = 0;


#if 0
  //! Obtains pixel data for a requested region. The user is responsible for allocating
  //! enough memory for the data to fit the array and clearing the memory. Please note that in case of int32 ARGB data,  
  //! like in OpenSlide, the order of the colors depends on the endianness of your machine (Windows typically BGRA)
  template <typename T> 
    void getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, const unsigned long long& height, std::shared_ptr<T>& data)
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
#endif
    
    

protected :

    // Properties of an image
    std::vector<double> _spacing;
    unsigned int _samplesPerPixel;
    lif::ColorType _colorType;
    cinder::ImageIo::DataType _dataType;
    bool _isValid;

  std::vector<unsigned long long> _dims;

  unsigned long long _cacheSize;
  std::string _fileType;

  // Cleans up internals
  virtual void cleanup();

  // Reads the actual data from the image
  virtual void* readDataFromImage(const uint32_t& startX, const uint32_t& startY, const uint32_t& width, const  uint32_t& height) = 0;

  // Reads the actual data from the image
  virtual void* readAllImageData() = 0;
};



#endif