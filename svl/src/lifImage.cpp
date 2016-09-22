/*
 *  lifImage.cpp
 *  lifImage
 *
 *  Created by Arman Garakani on 9/3/16.
 *  Copyright © 2016 Arman Garakani. All rights reserved.
 *
 */


#include <iostream>
#include <fstream>
#include "otherIO/lifImage.hpp"
#include "otherIO/filetools.h"
#include "otherIO/stringconversion.h"
#include "pugixml.hpp"


const char lifImage::LIF_MAGIC_BYTE = 0x70;
const char lifImage::LIF_MEMORY_BYTE = 0x2a;


lifImage* lifImage::open(const std::string& fileName)
{
    std::string ext = svl::extractFileExtension(fileName);
    if (ext == "lif") {
        
        lifImage* img = new lifImage;
        if (img && img->initialize(fileName))
        {
            return img;
        }
    }
        return nullptr;
}

/**
 * Converts from two-word tick representation to milliseconds. Mainly useful
 * in conjunction with COBOL date conversion.
 */

static double getMillisFromTicks(long hi, long lo)
{
    int64_t ticks = ((hi << 32) | lo);
    return ticks / 10000.0; // 100 ns = 0.0001 ms
}

static double getMillsFromEncodedTicks (std::string& token)
{
    if(token.length() > 8)
    {
        long los = svl::fromstring<long>(token.substr((token.length()-8), 8));
        long hos = svl::fromstring<long>(token.substr(0, token.length()-8));
        return getMillisFromTicks(hos, los);
    }
    else
    {
        return 0.0;
    }
}

static double get_file_timestamp (pugi::xml_document& doc)
{
    pugi::xml_node ts = doc.select_node("//Data//Experiment").node();
    double cur_ts = 0.0;
    std::string hiStr = ts.begin()->attribute("HighInteger").value();
    std::string loStr = ts.begin()->attribute("LowInteger").value();
    long hiInt = svl::fromstring<long>(hiStr);
    long loInt = svl::fromstring<long>(loStr);
    cur_ts = getMillisFromTicks(hiInt, loInt);
    return cur_ts;
}


lifImage::lifImage() :   baseImage(), _lastChannel(0), _alternateCenter(false), _selectedSeries(-1), _fileSize(0), _fileName("") {
}

lifImage::~lifImage() {
    cleanup();
}

void lifImage::cleanup() {
    _fileSize = 0;
    _fileName = "";
    _realChannel.clear();
    _lastChannel = 0;
    _selectedSeries = -1;
    
    _lutNames.clear();
    _physicalSizeXs.clear();
    _physicalSizeYs.clear();
    _fieldPosX.clear();
    _fieldPosY.clear();
    _seriesInfo.clear ();
    _descriptions.clear();
    _microscopeModels.clear();
    _serialNumber.clear();
    _pinholes.clear();
    _zooms.clear();
    _zSteps.clear();
    _tSteps.clear();
    _lensNA.clear();
    _expTimes.clear();
    _gains.clear();
    _detectorOffsets.clear();
    _channelNames.clear();
    _detectorModels.clear();
    _exWaves.clear();
    _activeDetector.clear();
    _detectorIndexes.clear();
    
    _immersions.clear();
    _corrections.clear();
    _objectiveModels.clear();
    _magnification.clear();
    _posX.clear();
    _posY.clear();
    _posZ.clear();
    _refractiveIndex.clear();
    _cutIns.clear();
    _cutOuts.clear();
    _filterModels.clear();
    _timestamps.clear();
    _laserWavelength.clear();
    _laserIntensity.clear();
    _alternateCenter = false;
    _imageNames.clear();
    _acquiredDate.clear();
    _dataTypes.clear();
    _colorTypes.clear();
    _dimensionOrder.clear();
    _seriesDimensions.clear();
    _imageCount.clear();
    
    baseImage::cleanup();
    
}

bool lifImage::initialize(const std::string& imagePath) {
    cleanup();
    if (!svl::fileExists(imagePath)) {
        return false;
    }
    std::ifstream lif;
    lif.open(imagePath.c_str(), std::ios::in | std::ios::binary);
    if (lif.good()) {
        _fileName = imagePath;
        // First check file size
        lif.seekg(0, std::ifstream::end);
        _fileSize = lif.tellg();
        lif.seekg(0);
        
        // Buffers to hold int and long
        char* memblock = new char[4];
        char* memblockLong = new char[8];
        
        char checkOne;
        lif.read(&checkOne,1);
        lif.seekg(2, std::ios::cur);
        char checkTwo;
        lif.read(&checkTwo,1);
        if (checkOne != LIF_MAGIC_BYTE && checkTwo != LIF_MAGIC_BYTE) {
            return false;
        }
        
        lif.seekg(4, std::ios::cur);
        lif.read(&checkTwo,1);
        if (checkTwo != LIF_MEMORY_BYTE) {
            return false;
        }
        
        // Reading and cleaning up XML
        lif.read(memblock,4);
        int nc = *reinterpret_cast<int*>(memblock);
        char* rawXml = new char[nc*2];
        char* editedXml = new char[nc*2];
        lif.read(rawXml, nc*2);
        int index = 0;
        for (int i =0; i < nc*2; ++i) {
            if(rawXml[i]) {
                editedXml[index] = rawXml[i];
                ++index;
            }
        }
        if (index < nc*2) {
            editedXml[index] = NULL;
        }
        std::string xml(editedXml);
        
        // Read the image offsets
        while (lif.tellg() < _fileSize) {
            lif.read(memblock,4);
            int magicByte = *reinterpret_cast<int*>(memblock);
            if (magicByte != LIF_MAGIC_BYTE) {
                return false;
            }
            
            lif.seekg(4, std::ios::cur);
            lif.read(&checkTwo, 1);
            if (checkTwo != LIF_MEMORY_BYTE) {
                return false;
            }
            
            lif.read(memblock,4);
            long blockLength = *reinterpret_cast<int*>(memblock);
            lif.read(&checkTwo, 1);
            if (checkTwo != LIF_MEMORY_BYTE) {
                lif.seekg(-5, std::ios::cur);
                lif.read(memblockLong,8);
                blockLength = *reinterpret_cast<long*>(memblock);
                lif.read(&checkTwo,1);
                if (checkTwo != LIF_MEMORY_BYTE) {
                    return false;
                }
            }
            lif.read(memblock,4);
            int descrLength =  (*reinterpret_cast<int*>(memblock)) * 2;
            
            if (blockLength > 0) {
                long curPos = lif.tellg();
                long offset = curPos + descrLength;
                _offsets.push_back(offset);
            }
            
            lif.seekg(descrLength + blockLength, std::ios::cur);
        }
        // Start parsing the XML MetaData
        pugi::xml_document doc;
        doc.load(xml.c_str());
        translateMetaData(doc);
        // Set the internals
        std::vector<unsigned long long> dims;
        dims.push_back(_seriesDimensions[_selectedSeries]["x"]);
        dims.push_back(_seriesDimensions[_selectedSeries]["y"]);
        dims.push_back(_seriesDimensions[_selectedSeries]["x"]);
        _spacing.clear();
        if (!_physicalSizeXs.empty() && !_physicalSizeYs.empty()) {
            _spacing.push_back(_physicalSizeXs[0]);
            _spacing.push_back(_physicalSizeYs[0]);
        }
        _colorType = _colorTypes[_selectedSeries];
        _dataType = _dataTypes[_selectedSeries];
        _samplesPerPixel = static_cast<uint32_t> (_seriesDimensions[_selectedSeries]["c"]);
        _isValid = true;
        
        return _isValid;
    }
    return false;
}

void lifImage::translateMetaData(pugi::xml_document& doc)
{
    _capture_timestamp = get_file_timestamp(doc);
    
    pugi::xpath_node_set images = doc.select_nodes("//Image");
    
    // Initialize variables based on the image size
    _acquiredDate = std::vector<double>(images.size(), 0);
    _descriptions = std::vector<std::string>(images.size(), "");
    _laserWavelength = std::vector<int>(images.size(), 0);
    _laserIntensity = std::vector<int>(images.size(), 0);
    _timestamps = std::vector<std::vector<double> >(images.size(), std::vector<double>());
    _timestamps_first_diffs = std::vector<std::vector<double> >(images.size(), std::vector<double>());
    _activeDetector = std::vector<std::string>(images.size(), "");
    _serialNumber = std::vector<std::string>(images.size(), "");
    _lensNA = std::vector<double>(images.size(), 0);
    _magnification = std::vector<double>(images.size(), 0);
    _immersions = std::vector<std::string>(images.size(), "");
    _corrections = std::vector<std::string>(images.size(), "");
    _objectiveModels = std::vector<std::string>(images.size(), "");
    _posX = std::vector<double>(images.size(), 0);
    _posY = std::vector<double>(images.size(), 0);
    _posZ = std::vector<double>(images.size(), 0);
    _refractiveIndex = std::vector<double>(images.size(), 0);
    _cutIns = std::vector<int>(images.size(), 0);
    _cutOuts = std::vector<int>(images.size(), 0);
    _filterModels = std::vector<int>(images.size(), 0);
    _microscopeModels = std::vector<std::string>(images.size(), "");
    _detectorModels = std::vector<std::string>(images.size(), "");
    _zSteps = std::vector<double>(images.size(), 0);
    _tSteps = std::vector<double>(images.size(), 0);
    _pinholes = std::vector<double>(images.size(), 0);
    _zooms = std::vector<double>(images.size(), 0);
    _imageCount = std::vector<unsigned int>(images.size(), 0);
    _seriesDimensions = std::vector<std::map<std::string, int64_t> >(images.size(), std::map<std::string, int64_t>());
    _dimensionOrder = std::vector<std::string >(images.size(), "");
    _colorTypes = std::vector<ColorType>(images.size(), ColorType::InvalidColorType);
    _dataTypes = std::vector<ci::ImageIo::DataType>(images.size(), ci::ImageIo::DataType::DATA_UNKNOWN);
     _seriesInfo.resize (_dataTypes.size());
    _expTimes = std::vector<std::vector<double> >(images.size(), std::vector<double>());
    _gains = std::vector<std::vector<double> >(images.size(), std::vector<double>());
    _detectorOffsets = std::vector<std::vector<double> >(images.size(), std::vector<double>());
    _channelNames = std::vector<std::vector<std::string> > (images.size(), std::vector<std::string>());
    _exWaves = std::vector<std::vector<double> >(images.size(), std::vector<double>());
    _imageNames = std::vector<std::string>(images.size(), "");
    
    int imageNr = 0;
    _series_count = images.size();
    
    for (pugi::xpath_node_set::const_iterator it = images.begin(); it != images.end(); ++it) {
        pugi::xpath_node node = *it;
        translateImageNames(node, imageNr);
        translateImageNodes(node,imageNr);
        translateAttachmentNodes(node, imageNr);
        translateScannerSettings(node, imageNr);
        translateFilterSettings(node, imageNr);
        translateTimestamps(node, imageNr);
        translateLaserLines(node, imageNr);
        translateDetectors(node, imageNr);
        ++imageNr;
    }
    
    std::vector<std::map<std::string, int64_t> > _newSeriesDimensions;
    std::vector<std::string > _newDimensionOrder;
    std::vector<ColorType> _newColorTypes;
    std::vector<ci::ImageIo::DataType> _newDataTypes;
    std::vector<unsigned int> _newImageCount;
    
    for (int i = 0; i < _dataTypes.size(); ++i)
    {
            _newSeriesDimensions.push_back(_seriesDimensions[i]);
            _newDimensionOrder.push_back(_dimensionOrder[i]);
            _newColorTypes.push_back(_colorTypes[i]);
            _newImageCount.push_back(_imageCount[i]);
            _newDataTypes.push_back(_dataTypes[i]);
    }
    
    _seriesDimensions = _newSeriesDimensions;
    _dimensionOrder = _newDimensionOrder;
    _colorTypes = _newColorTypes;
    _dataTypes = _newDataTypes;
    _imageCount = _newImageCount;
   
    
    _selectedSeries = 0;
    
    _dims.first = static_cast<uint32_t> (_seriesDimensions[_selectedSeries]["x"]);
    _dims.second = static_cast<uint32_t> (_seriesDimensions[_selectedSeries]["y"]);
}

void lifImage::selectedSeries (unsigned s) const
{
    if (s < series())
        _selectedSeries = s;
}

const std::vector<std::map<std::string, int64_t> >& lifImage::seriesDimensions () const
{
    return _seriesDimensions;
}

const std::vector<std::string >& lifImage::seriesNames () const
{
    return _imageNames;
}

const std::vector<std::string >& lifImage::seriesOrder () const
{
    return _dimensionOrder;
}
const std::vector<ColorType>& lifImage::seriesColorTypes () const
{
    return _colorTypes;
}
const std::vector<ci::ImageIo::DataType>& lifImage::seriesDataTypes () const
{
    return _dataTypes;
}
const std::vector<unsigned int>& lifImage::seriesCounts () const
{
    return _imageCount;
}

const tpair<uint32_t> lifImage::getDimensions() const
{
    return _dims;
}

void* lifImage::readAllImageData()
{
    return readDataFromImage(0, 0, _dims[0],_dims[1]);
}

const lif_serie& lifImage::info (unsigned index) const
{
    static lif_serie clearLif;
    if (index < _imageNames.size())
        return _seriesInfo[index];
    return clearLif;
}


uint32_t lifImage::plane_size (unsigned index)
{
    return _seriesInfo[index].x * _seriesInfo[index].y * pixel_size(index);
}

uint32_t lifImage::pixel_size (unsigned index)
{
    int bytes = 4;
    if (_dataTypes[index] == ci::ImageIo::DataType::UINT16) {
        bytes = 2;
    } else if (_dataTypes[index] == ci::ImageIo::DataType::UINT8) {
        bytes = 1;
    }
    return bytes;
}

uint32_t lifImage::channels(unsigned index)
{
   return _seriesInfo[index].c;
}

void*lifImage::readDataFromImage(const uint32_t& startX, const uint32_t& startY, const uint32_t& width, const  uint32_t& height)
{
    
    unsigned long long offset = _offsets[_selectedSeries];
    uint32_t bpp = pixel_size(_selectedSeries);
    uint32_t planeSize = plane_size(_selectedSeries);
    uint32_t nrChannels = channels (_selectedSeries);
    
    int64_t nextOffset = (_selectedSeries + 1 < _offsets.size())  ? _offsets[_selectedSeries + 1] : _fileSize;
    int bytesToSkip = (int) (nextOffset - offset - planeSize * _imageCount[_selectedSeries]);
    bytesToSkip /= _seriesInfo[_selectedSeries].y;
    if ((_seriesInfo[_selectedSeries].x % 4) == 0) {
        bytesToSkip = 0;
    }
    
    if (offset + (planeSize + bytesToSkip * _seriesInfo[_selectedSeries].y) >= _fileSize) {
        return NULL;
    }
    
    std::ifstream in;
    in.open(_fileName.c_str(), std::ios::in | std::ios::binary);
    in.seekg(offset);
    
    char* buf = new char[width*height*bpp*channels(_selectedSeries)];
    in.seekg((planeSize * _imageCount[_selectedSeries]), std::ios::cur);
    in.seekg(bytesToSkip * _seriesDimensions[_selectedSeries]["y"], std::ios::cur);
    
    if (bytesToSkip == 0) {
        auto seriesWidth = _seriesDimensions[_selectedSeries]["x"];
        for (int channel=0; channel < nrChannels ; channel++) {
            in.seekg(startY * seriesWidth * bpp, std::ios::cur);
            for (int row=0; row < height; row++) {
                in.seekg(startX * bpp, std::ios::cur);
                in.read(buf + channel * width * height * bpp + row * width * bpp, width * bpp);
                if (row < height - 1 || channel < nrChannels - 1) {
                    // no need to skip bytes after reading final row of final channel
                    in.seekg(bpp * (seriesWidth - width - startX), std::ios::cur);
                }
            }
            if (channel < nrChannels - 1) {
                // no need to skip bytes after reading final channel
                in.seekg(seriesWidth * bpp * (_seriesDimensions[_selectedSeries]["y"] - startY - height), std::ios::cur);
            }
        }
    }
    else {
        in.seekg(startY * (_seriesDimensions[_selectedSeries]["x"] * bpp + bytesToSkip));
        for (int row=0; row < height; row++) {
            in.seekg(startX * bpp, std::ios::cur);
            in.read(buf + (row * width * bpp), width * bpp);
            in.seekg(bpp * (_seriesDimensions[_selectedSeries]["x"] - width - startX) + bytesToSkip, std::ios::cur);
        }
    }
    in.close();
    
    // Change planar to interleaved
    char* interLeavedbuf = new char[width*height*bpp*nrChannels];
    for (unsigned int i = 0; i < width*height; ++i) {
        for (unsigned int c = 0; c < nrChannels; ++c) {
            unsigned int cStride = width*height*bpp*c;
            for (unsigned int b = 0; b < bpp; ++b) {
                interLeavedbuf[i*bpp*nrChannels+c*bpp+b] = buf[i*bpp + cStride + b];
            }
        }
    }
    delete[] buf;
    return interLeavedbuf;
}


void lifImage::translateTimestamps(pugi::xpath_node& imageNode, int imageNr) {

    
    pugi::xpath_node timestampList = imageNode.node().child ("TimeStampList");
    std::string numSlicesStr = timestampList.node().attribute("NumberOfTimeStamps").value();
    int numSlices = svl::fromstring<int>(numSlicesStr);
    assert(numSlices == _imageCount[imageNr]);
    _timestamps[imageNr].resize(numSlices);
    _timestamps_first_diffs[imageNr].resize(numSlices);
   
    auto tss = timestampList.node().text();
    if (tss)
    {
        std::istringstream buf(tss.get());
        std::istream_iterator<std::string> beg(buf), end;
        std::vector<std::string> tokens(beg, end);
        assert(_imageCount[imageNr] == tokens.size());
        
        std::vector<double>::iterator loadItr = _timestamps[imageNr].begin();
        std::vector<double>::iterator diffItr = _timestamps_first_diffs[imageNr].begin();

        // Use first one as the last one
        std::string& fs = *tokens.begin();
        double last = getMillsFromEncodedTicks(fs);

        // read and record timestamps and first differences
        for(auto& s: tokens)
        {
            double newframe = getMillsFromEncodedTicks(s);
            *diffItr++ = newframe - last;
            *loadItr++ = newframe;
             last = newframe;
        }
    }
}

void lifImage::translateImageNames(pugi::xpath_node& imageNode, int imageNr) {
    std::vector<std::string> names;
    pugi::xpath_node parent = imageNode;
    while (true) {
        parent = parent.parent();
        if (parent == NULL || std::string(parent.node().name()) == "LEICA") {
            break;
        }
        if (std::string(parent.node().name()) == "Element") {
            names.push_back(parent.node().attribute("Name").value());
        }
    }
    std::string name = "";
    for (int32_t i ((int) names.size() - 2); i>=0; i--)
    {
        name += names[i];
        if (i > 0) {
            name += "/";
        }
    }
    _imageNames[imageNr] = name;
}

void lifImage::translateAttachmentNodes(pugi::xpath_node&  imageNode, int imageNr)
{
    pugi::xpath_node_set attachmentNodes = imageNode.node().select_nodes("Attachment");
    if (attachmentNodes.empty()) {
        return;
    }
    for (int i=0; i < attachmentNodes.size(); i++) {
        pugi::xpath_node attachment = attachmentNodes[i];
        std::string attachmentName = attachment.node().attribute("Name").value();
        if (attachmentName == "ContextDescription") {
            _descriptions[imageNr] = attachment.node().attribute("Content").value();
        }
        else if (attachmentName == "TileScanInfo") {
            assert(false);
        }
    }
}

void lifImage::translateImageNodes(pugi::xpath_node&  imageNode, int imageNr)
{
    pugi::xpath_node_set channels = imageNode.node().child("ImageDescription").child("Channels").select_nodes("ChannelDescription");
    pugi::xpath_node_set dimensions = imageNode.node().child("ImageDescription").child("Dimensions").select_nodes("DimensionDescription");
    
    lif_serie linfo;
    std::map<long, std::string> bytesPerAxis;
    std::map<std::string, int64_t> serieDimensions;
    serieDimensions["x"]=0;
    serieDimensions["y"]=0;
    serieDimensions["z"]=0;
    serieDimensions["c"]=0;
    serieDimensions["t"]=0;
    
    double physicalSizeX = 0.0;
    double physicalSizeY = 0.0;
    double physicalSizeZ = 0.0;
    
    serieDimensions["c"]=linfo.c = channels.size();
    for (int ch=0; ch < channels.size(); ch++) {
        const pugi::xpath_node channel = channels[ch];
        
        _lutNames.push_back(channel.node().attribute("LUTName").value());
        std::string bytesInc = channel.node().attribute("BytesInc").value();
        long bytes = svl::fromstring<long>(bytesInc);
        if (bytes > 0) {
            bytesPerAxis[bytes] =  "c";
        }
    }
    
    int extras = 1;
    for (int dim=0; dim<dimensions.size(); dim++) {
        const pugi::xpath_node dimension = dimensions[dim];
        
        int id = dimension.node().attribute("DimID").as_int();
        int len = dimension.node().attribute("NumberOfElements").as_int();
        long nBytes = svl::fromstring<long>(dimension.node().attribute("BytesInc").value());
        double physicalLen = svl::fromstring<double>(dimension.node().attribute("Length").value());
        std::string unit = dimension.node().attribute("Unit").value();
        
        physicalLen /= len;
        if (unit == "Ks") {
            physicalLen /= 1000;
        }
        else if (unit == "m") {
            physicalLen *= 1000000;
        }
        switch (id) {
            case 1: // X axis
                serieDimensions["x"] = len;
                linfo.x = len;
                if ((nBytes % 3) == 0) {
                    _colorTypes[imageNr] = RGB;
                } else {
                    _colorTypes[imageNr] = Indexed;
                }
                if (_colorType == RGB) {
                    nBytes /= 3;
                }
                if (nBytes == 1) {
                    _dataTypes[imageNr] = ci::ImageIo::DataType::UINT8;
                } else if (nBytes == 2) {
                    _dataTypes[imageNr] = cinder::ImageIo::UINT16;
                } else if(nBytes == 4) {
                    _dataTypes[imageNr] = cinder::ImageIo::FLOAT32;
                }
                physicalSizeX = physicalLen;
                break;
            case 2: // Y axis
                if (serieDimensions["y"] != 0) {
                    if (serieDimensions["z"] == 1) {
                        serieDimensions["z"] = len;
                        bytesPerAxis[nBytes] =  "z";
                        physicalSizeZ = (physicalLen * len) / (len - 1);
                    }
                    else if (serieDimensions["t"] == 1) {
                        serieDimensions["t"] = len;
                        linfo.t = len;
                        bytesPerAxis[nBytes] =  "t";
                    }
                }
                else {
                    serieDimensions["y"] = len;
                    linfo.y = len;
                    physicalSizeY = physicalLen;
                }
                break;
            case 3: // Z axis
                if (serieDimensions["y"] == 0) {
                    // XZ scan - swap Y and Z
                    serieDimensions["y"] = len;
                    linfo.y = len;
                    linfo.z = 1;
                    serieDimensions["z"] = 1;
                    bytesPerAxis[nBytes] =  "y";
                    physicalSizeY = physicalLen;
                }
                else {
                    serieDimensions["z"] = len;
                    serieDimensions["z"] = len;
                    bytesPerAxis[nBytes] =  "z";
                    physicalSizeZ = (physicalLen * len) / (len - 1);
                }
                break;
            case 4: // T axis
                if (serieDimensions["y"] == 0) {
                    // XT scan - swap Y and T
                    serieDimensions["y"] = len;
                    linfo.y = len;
                    linfo.t = 1;
                    serieDimensions["t"] = 1;
                    bytesPerAxis[nBytes] = "y";
                    physicalSizeY = physicalLen;
                }
                else {
                    serieDimensions["t"] = len;
                    linfo.t = len;
                    bytesPerAxis[nBytes] = "t";
                }
                break;
            default:
                extras *= len;
        }
    }
    
    _physicalSizeXs.push_back(physicalSizeX);
    _physicalSizeYs.push_back(physicalSizeY);
    
    if (_zSteps[imageNr] == 0.0 && physicalSizeZ != 0.0) {
        _zSteps[imageNr] = abs(physicalSizeZ);
    }
    
    if (extras > 1) {
        if (serieDimensions["z"] == 1) {
            serieDimensions["z"] = extras;
            linfo.z = extras;
        }
        else {
            if (serieDimensions["t"] == 0) {
                serieDimensions["t"] = extras;
                linfo.t = extras;
            }
            else {
                serieDimensions["t"] *= extras;
                linfo.t *= extras;
            }
        }
    }
    
    if (serieDimensions["c"] == 0) {
        serieDimensions["c"] = 1; linfo.c = 1;
    }
    if (serieDimensions["z"] == 0) {
        serieDimensions["z"] = 1; linfo.z = 1;
    }
    if (serieDimensions["t"] == 0) {
        serieDimensions["t"] = 1; linfo.t = 1;
    }
    
    _imageCount[imageNr] = static_cast<uint32_t> (serieDimensions["z"] * serieDimensions["t"]);
    if (_colorTypes[imageNr] != RGB) {
        _imageCount[imageNr] *= serieDimensions["c"];
    }
    
    std::vector<long> bytes;
    for(std::map<long,std::string>::iterator it = bytesPerAxis.begin(); it != bytesPerAxis.end(); ++it) {
        bytes.push_back(it->first);
    }
    std::sort(bytes.begin(), bytes.end());
    std::string dimensionOrder = "xy";
    if (serieDimensions["c"] > 1 && serieDimensions["t"] > 1) {
        dimensionOrder += "c";
    }
    for (std::vector<long>::const_iterator it = bytes.begin(); it != bytes.end(); ++ it) {
        std::string axis = bytesPerAxis[*it];
        if (dimensionOrder.find(axis) == std::string::npos) {
            dimensionOrder += axis;
        }
    }
    
    if (dimensionOrder.find("z") == std::string::npos) {
        dimensionOrder += "z";
    }
    if (dimensionOrder.find("c") == std::string::npos) {
        dimensionOrder += "c";
    }
    if (dimensionOrder.find("t") == std::string::npos) {
        dimensionOrder += "t";
    }
    
    assert(linfo.x == serieDimensions["x"]);
    assert(linfo.y == serieDimensions["y"]);
    assert(linfo.c == serieDimensions["c"]);
    assert(linfo.z == serieDimensions["z"]); 
    assert(linfo.t == serieDimensions["t"]);
    
    _dimensionOrder[imageNr] = dimensionOrder;
    _seriesDimensions[imageNr] = serieDimensions;
    _seriesInfo[imageNr] = linfo;
}


template<>
roiWindow<P8U> lifImage::getRoiWindow ()
{
    typedef typename PixelType<P8U>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<P8U>::pixel_t pixel_t;
    if (PixelType<P8U>::ct() == _dataTypes[_selectedSeries] && _dataTypes[_selectedSeries] == cinder::ImageIo::UINT8)
    {
        uint8_t* pixels = static_cast<uint8_t*> (readAllImageData());
        sharedRoot<P8U> root (new svl::root<P8U>(pixels, _dims[0], _dims[0],_dims[1], image_memory_alignment_policy::align_first_row));
        return roiWindow<P8U> (root);
    }
    return roiWindow<P8U> ();
}

