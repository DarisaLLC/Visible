#ifndef PatchH
#define PatchH

#include <vector>
#include "otherIO/lif.h"

using namespace lif;


template<typename T>
class Patch : public imageData 
{

private:
  T* _buffer;
  unsigned long long _bufferSize;
  bool _ownData;
  std::vector<unsigned long long> _dimensions;
  std::vector<unsigned long long> _strides;
  void calculateStrides();

  void swap(Patch<T>& first, Patch<T>& second);
    
  std::vector<double> _spacing;
  unsigned int _samplesPerPixel;
  lif::ColorType _colorType;
  lif::DataType _dataType;
  bool _isValid;

public :

  Patch();
  ~Patch();
  Patch(const Patch& rhs);
  Patch& operator=(const Patch rhs);
  Patch(const std::vector<unsigned long long>& dimensions, const lif::ColorType& ctype = lif::Monochrome, T* data = NULL, bool ownData = true);

  // Arithmetic operators
  Patch<T> operator*(const T& val);
  Patch<T>& operator*=(const T& val);
  Patch<T> operator/(const T& val);
  Patch<T>& operator/=(const T& val);
  Patch<T> operator+(const T& val);
  Patch<T>& operator+=(const T& val);
  Patch<T> operator-(const T& val);
  Patch<T>& operator-=(const T& val);

  const T* getPointer() const;
  T* getPointer();

  std::vector<unsigned long long> getStrides();

  double getMinValue(int channel = -1);
  double getMaxValue(int channel = -1);

  T getValue(const std::vector<unsigned long long>& index) const;
  void setValue(const std::vector<unsigned long long>& index, const T& value);
  void fill(const T& value);
  void setSpacing(const std::vector<double>& spacing);

  bool empty();

  const std::vector<unsigned long long> getDimensions() const;
  const int getSamplesPerPixel() const;
  const unsigned long long getBufferSize() const;

  const lif::DataType getDataType() const;

};

#include "Patch.hpp"

#endif