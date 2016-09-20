#ifndef _TIFFImage
#define _TIFFImage

#include "baseImage.h"


struct tiff;
typedef struct tiff TIFF;

class JPEG2000Codec;

class  TIFFImage : public baseImage {

public:

  TIFFImage();
  ~TIFFImage();  
  bool initialize(const std::string& imagePath);

  double getMinValue(int channel = -1);
  double getMaxValue(int channel = -1);

protected :
  void cleanup();
  
  void* readDataFromImage(const long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height, const unsigned int& level);

  template <typename T> T* FillRequestedRegionFromTIFF(const  long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height,  unsigned int nrSamples);

  TIFF* _tiff;
  std::vector<std::vector<unsigned int> > _tileSizesPerLevel;

  std::vector<double> _minValues;
  std::vector<double> _maxValues;

  JPEG2000Codec* _jp2000;

};

#endif