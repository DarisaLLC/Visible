#ifndef _imageData
#define _imageData
#include <string>
#include <vector>
#include "otherIO/lif.h"

class imageData {

public :
  imageData();
  virtual ~imageData();

  virtual bool valid() const {return _isValid;}

  //! Gets the dimensions of the base level of the pyramid
  virtual const std::vector<unsigned long long> getDimensions() const = 0;
  
  //! Returns the color type
  virtual const lif::ColorType getColorType() const;

  //! Returns the data type
  virtual const lif::DataType getDataType() const;

  //! Returns the number of samples per pixel
  virtual const int getSamplesPerPixel() const;

  //! Returns the pixel spacing meta-data (um per pixel)
  virtual const std::vector<double> getSpacing() const; 

  //! Gets the minimum value for a channel. If no channel is specified, default to overall minimum
  virtual double getMinValue(int channel = -1) = 0;

  //! Gets the maximum value for a channel. If no channel is specified, default to overall maximum
  virtual double getMaxValue(int channel = -1) = 0;

  void swap(imageData& first, imageData& second);

protected :

  // Properties of an image
  std::vector<double> _spacing;
  unsigned int _samplesPerPixel;
  lif::ColorType _colorType;
  lif::DataType _dataType;
  bool _isValid;

};

#endif