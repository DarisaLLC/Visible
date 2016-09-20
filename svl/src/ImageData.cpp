#include "otherIO/lif.h"
#include "otherIO/imageData.h"

using namespace lif;

imageData::imageData() :
  
  _spacing(),
  _samplesPerPixel(0),
  _colorType(InvalidColorType),
  _dataType(InvalidDataType),
  _isValid(false)
{
}

imageData::~imageData() {
}

void imageData::swap(imageData& first, imageData& second) {
  std::swap(first._colorType, second._colorType);
  std::swap(first._dataType, second._dataType);
  std::swap(first._spacing, second._spacing);
  std::swap(first._samplesPerPixel, second._samplesPerPixel);
  std::swap(first._isValid, second._isValid);
}

const int imageData::getSamplesPerPixel() const {
  if (_isValid) {
    return _samplesPerPixel;
  } else {
    return -1;
  }
}

const ColorType imageData::getColorType() const {
  if (_isValid) {
    return _colorType;
  } else {
    return InvalidColorType;
  }
}

const std::vector<double> imageData::getSpacing() const {
  return _spacing;
}

const DataType imageData::getDataType() const {
  if (_isValid) {
    return _dataType;
  } else {
    return InvalidDataType;
  }
 }