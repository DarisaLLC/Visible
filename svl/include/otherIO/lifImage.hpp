/*
 *  lifImage.hpp
 *  lifImage
 *
 *  Created by Arman Garakani on 9/3/16.
 *  Copyright © 2016 Arman Garakani. All rights reserved.
 *
 */

#ifndef lifImage_
#define lifImage_

#include <vector>
#include <map>
#include "core/stl_utils.hpp"
#include "otherIO/lif.h"
#include "otherIO/baseImage.h"
#include "pugixml.hpp"
#include <cmath>

using namespace lif;

class lifImage : public baseImage
{
public:
   lifImage();
    ~lifImage();
    
    bool initialize(const std::string& imagePath);
    
    double fileTimestamp () const;
    double acquiredDate () const;

    const std::vector<unsigned long long> getDimensions() const;
    
    size_t series () const { return _series_count; }
    unsigned selectedSeries () const { return _selectedSeries; }
    void selectedSeries (unsigned s) const;
    
    const std::vector<std::map<std::string, unsigned long long> >& seriesDimensions () const;
    const std::vector<std::string >& seriesOrder () const;
    const std::vector<ColorType>& seriesColorTypes () const;
    const std::vector<lif::DataType>& seriesDataTypes () const;
    const std::vector<unsigned int>& seriesCounts () const;
    
    bool isSingleByteChannel () const;
    bool isTrippleByteChannel () const;

    
    protected :
    
    void cleanup();
    
    void* readDataFromImage(const long long& startX, const long long& startY, const unsigned long long& width,
                            const unsigned long long& height);
    
    double getMinValue(int channel = -1) { return 0.; } // Not yet implemented
    double getMaxValue(int channel = -1) { return 3072; } // Not yet implemented
    
    private :
    
    static const char LIF_MAGIC_BYTE;
    static const char LIF_MEMORY_BYTE;


    std::vector<std::vector<int> > _realChannel;
    int _lastChannel;
    mutable int _selectedSeries;
    size_t _series_count;
    unsigned long long _fileSize;
    std::string _fileName;
    double _capture_timestamp;
    
    std::vector<std::string> _lutNames;
    std::vector<double> _physicalSizeXs;
    std::vector<double> _physicalSizeYs;
    std::vector<double> _fieldPosX;
    std::vector<double> _fieldPosY;
    
    std::vector<std::string> _descriptions, _microscopeModels, _serialNumber;
    std::vector<double> _pinholes, _zooms, _zSteps, _tSteps, _lensNA;
    std::vector<std::vector<double> > _expTimes, _gains, _detectorOffsets;
    std::vector<std::vector<std::string> > _channelNames;
    std::vector<std::string> _detectorModels;
    std::vector<std::vector<double> > _exWaves;
    std::vector<std::string> _activeDetector;
    std::map<std::string, int> _detectorIndexes;
    
    std::vector<std::string> _immersions, _corrections, _objectiveModels;
    std::vector<double> _magnification;
    std::vector<double> _posX, _posY, _posZ;
    std::vector<double> _refractiveIndex;
    std::vector<int> _cutIns, _cutOuts, _filterModels;
    std::vector<std::vector<double> > _timestamps;
    std::vector<std::vector<double> > _timestamps_first_diffs;
    std::vector<int> _laserWavelength, _laserIntensity;
    bool _alternateCenter;
    std::vector<std::string> _imageNames;
    std::vector<double> _acquiredDate;
    std::vector<int> _tileCount;
  
    std::vector<std::string > _dimensionOrder;
    std::vector<ColorType> _colorTypes;
    std::vector<lif::DataType> _dataTypes;
    std::vector<unsigned int> _imageCount;
    std::vector<unsigned long long> _offsets;
    std::vector<unsigned long long> _dims;

    // Note a vector of Maps
    std::vector<std::map<std::string, unsigned long long> > _seriesDimensions;
      
    void translateMetaData(pugi::xml_document& doc);
    void translateImageNames(pugi::xpath_node& imageNode, int imageNr);
    void translateImageNodes(pugi::xpath_node& imageNode, int imageNr);
    void translateAttachmentNodes(pugi::xpath_node& imageNode, int imageNr);
    void translateTimestamps(pugi::xpath_node& imageNode, int imageNr);
    void translateScannerSettings(pugi::xpath_node& imageNode, int imageNr) {};
    void translateFilterSettings(pugi::xpath_node& imageNode, int imageNr) {};
    void translateLaserLines(pugi::xpath_node& imageNode, int imageNr) {};
    void translateDetectors(pugi::xpath_node& imageNode, int imageNr) {};
    int getTileIndex(int index);
    
};


#endif