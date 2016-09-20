#ifndef _JPEG2000Codec
#define _JPEG2000Codec



class  JPEG2000Codec
{  
public:
  JPEG2000Codec();
  ~JPEG2000Codec();

  void encode(char* data, unsigned int& size, const unsigned int& tileSize, const unsigned int& depth, const unsigned int& nrComponents, float& rate, bool colorImage) const;
  void decode(char* buf, const unsigned int& inSize, const unsigned int& outSize) const;
};

#endif