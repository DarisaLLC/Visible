/** @file lifFile.hpp
 * @brief Namespace @ref LifReader
 */




/**
 
 
 * \file lifFile.cpp
 * \brief Define classes to read directly from Leica .lif file
 * \author Mathieu Leocmach
 * \date 28 March 2009
 *
 * Strongly inspired from BioimageXD VTK implementation but using standard library instead of VTK objects
 *
 */


#ifndef lif_reader_H
#define lif_reader_H

#include "core/core.hpp"
#include "core/timestamp.h"
#include "tinyxml.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include <stdexcept>
#include <memory>
#include <boost/utility.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>




namespace lifIO
{
    struct ChannelData;
    struct DimensionData;
    struct ScannerSettingRecord;
    struct FilterSettingRecord;
    
 
    
    class LifSerieHeader
    {
        
    public:
        explicit LifSerieHeader(TiXmlElement *root);
        
        typedef unsigned long long timestamp_t;
        
        std::string getName() const {return this->name;};
        double getVoxelSize(const size_t d) const;
        int getResolution(const size_t channel) const;
        size_t getNbTimeSteps() const;
        std::vector<size_t> getSpatialDimensions() const;
        size_t getNbPixelsInOneTimeStep() const;
        size_t getNbPixelsInOneSlice() const;
        double getZXratio() const;
        bool hasTimeChannel () const;
        const std::map<std::string, DimensionData>& getDimensionsData() const {return dimensions;};
        const std::vector<ChannelData>& getChannels() const {return channels;};
        const std::vector<timestamp_t>& getTimestamps () const { return timeStamps; }
        const std::vector<timestamp_t>& getDurations () const { return m_frame_durations; }
        float total_duration () const
        {
            if (timeStamps.size() < 2) return -1.0f;
            auto timeL = (timeStamps.end() - timeStamps.begin()) / 10000.0;
            if (std::signbit(timeL)) return -1.0f;
            return timeL;
        }
        
        float frame_duration_ms () const;
        float frame_rate () const { if (frame_duration_ms() > 0.0) return 1000.0/frame_duration_ms ();return -1.0; }
        
    protected:
        std::string name;
        std::map<std::string, DimensionData> dimensions;
        std::vector<ChannelData> channels;
        std::vector<timestamp_t> timeStamps;
        std::map<std::string, ScannerSettingRecord> scannerSettings;
        TiXmlElement *rootElement;
        
        mutable std::vector<timestamp_t> m_frame_durations;
        mutable std::pair<bool, float> m_cached_frame_duration;
        
    private:
        void parseImage(TiXmlNode *elementImage);
        void parseImageDescription(TiXmlNode *elementImageDescription);
        void parseTimeStampList(TiXmlNode *elementTimeStampList);
        void parseHardwareSettingList(TiXmlNode *elementHardwareSettingList);
        void parseScannerSetting(TiXmlNode *elementScannerSetting);
        
    };
    
    class LifSerie : public LifSerieHeader, boost::noncopyable
    {
        
    public:
        explicit LifSerie(LifSerieHeader serie, const std::string &filename, unsigned long long offset, unsigned long long memorySize);
        
        void fill3DBuffer(void* buffer, size_t t=0) const;
        void fill2DBuffer(void* buffer, size_t t=0, size_t z=0) const;
        
        
        std::istreambuf_iterator<char> begin(size_t t=0);
        std::streampos tellg(){return fileRef->tellg();}
        unsigned long long getOffset(size_t t=0) const;
        
    private:
        unsigned long long offset;
        unsigned long long memorySize;
        std::shared_ptr<std::ifstream> fileRef;
//        mutable std::ifstream file;
        std::streampos fileSize;
        
        
    };
    
    class LifHeader : boost::noncopyable
    {
        
    public:
        explicit LifHeader(TiXmlDocument &header);
        explicit LifHeader(std::string &header);
        
        const TiXmlDocument& getXMLHeader() const{return this->header;};
        std::string getName() const {return this->name;};
        const int& getVersion() const {return this->lifVersion;};
        size_t getNbSeries() const {return this->series.size();}
        bool contains(size_t s) const {return s<this->series.size();}
        LifSerieHeader& getSerieHeader(size_t s){return this->series[s];};
        const LifSerieHeader& getSerieHeader(size_t s) const {return this->series[s];};
        
        
    protected:
        TiXmlDocument header;
        boost::ptr_vector<LifSerieHeader> series;
        
    private:
        void parseHeader();
        int lifVersion;
        std::string name;
        
    };
    
    class LifReader : boost::noncopyable
    {
        
    public:
        typedef std::shared_ptr<LifReader> ref;
        typedef std::weak_ptr<LifReader> weak_ref;
        LifReader(const std::string &filename);
        
        static LifReader::ref create (const std::string&  fqfn_path){
            return LifReader::ref ( new LifReader (fqfn_path));
        }
        
        const LifHeader& getLifHeader() const {return *this->header;};
        const TiXmlDocument& getXMLHeader() const{return getLifHeader().getXMLHeader();};
        std::string getName() const {return getLifHeader().getName();};
        const int& getVersion() const {return getLifHeader().getVersion();};
        size_t getNbSeries() const {return this->series.size();}
        bool contains(size_t s) const {return getLifHeader().contains(s);}
        
        LifSerie& getSerie(size_t s){return series[s];};
        const LifSerie& getSerie(size_t s) const {return series[s];};
        const std::string& file_path () const { return m_path; }
        size_t file_size () const { return m_lif_file_size; }
        
        void close_file ();
        bool isValid () const { return m_Valid; }
        
    private:
        /**
         * RAIII (Resource Allocation is Initialization) Exception safe handling of openning and closing of files.
         * a functor object to delete an ifstream
         * utility functions to create
         */
  
        
        int readInt();
        unsigned int readUnsignedInt();
        unsigned long long readUnsignedLongLong();
        std::auto_ptr<LifHeader> header;
        std::shared_ptr<std::ifstream> m_fileRef;
        std::streampos fileSize;
        boost::ptr_vector<LifSerie> series;
        mutable bool m_Valid;
        mutable std::mutex m_mutex;
        std::string m_path;
        size_t m_lif_file_size;
       
        
        
    };
    
    
    /** Struct of channel data*/
    struct ChannelData
    {
        int dataType; // 0 Integer, 1 Float
        int channelTag; // 0 Gray value, 1 Red, 2 Green, 3 Blue
        int resolution; // Bits per pixel
        std::string nameOfMeasuredQuantity;
        double minimum;
        double maximum;
        std::string unit;
        std::string LUTName;
        bool isLUTInverted; // 0 Normal LUT, 1 Inverted Order
        unsigned long long bytesInc; // Distance from the first channel in Bytes
        int bitInc;
        
        explicit ChannelData(TiXmlElement *element);
        inline const std::string getName() const
        {
            return LUTName;
#if LIF3_ChannelIndex_IsAlways_0_is_fixed
            switch(channelTag)
            {
                case 0 : return "Gray";
                case 1 : return "Red";
                case 2 : return "Green";
                case 3 : return "Blue";
                default : return "Unknown color";
            }
#endif
            
        }
        
        
        
    };
    
    /** Struct of dimension data*/
    struct DimensionData
    {
        int dimID; // 0 Not valid, 1 X, 2 Y, 3 Z, 4 T, 5 Lambda, 6 Rotation, 7 XT Slices, 8 T Slices
        int numberOfElements; // Number of elements in this dimension
        double origin; // Physical position of the first element (left pixel side)
        double length; // Physical length from the first left pixel side to the last left pixel side
        std::string unit; // Physical unit
        unsigned long long bytesInc; // Distance from the one element to the next in this dimension
        int bitInc;
        
        explicit DimensionData(TiXmlElement *element);
        inline const std::string getName() const
        {
            switch(dimID)
            {
                case 1 : return "X";
                case 2 : return "Y";
                case 3 : return "Z";
                case 4 : return "T";
                case 5 : return "Lambda";
                case 6 : return "Rotation";
                case 7 : return "XT Slices";
                case 8 : return "TSlices";
                default : return "Invalid";
            }
        };
    };
    
    /**
     \brief Struct of scanner setting
     Maybe because the original Leica program was written in MS Visual Basic,
     the type of the field "Variant" has to be defined from a test on the VariantType field :
     3:long
     5:double
     8:char*
     11:bool
     17:char (or int)
     See http://en.wikipedia.org/wiki/Variant_type
     */
    struct ScannerSettingRecord
    {
        std::string identifier;
        std::string unit;
        std::string description;
        int data;
        std::string variant;
        int variantType;
        
        explicit ScannerSettingRecord(TiXmlElement *element);
    };
    
    /** Struct of Filter Setting */
    struct FilterSettingRecord
    {
        std::string objectName;
        std::string className;
        std::string attribute;
        std::string description;
        int data;
        std::string variant;
        int variantType;
        
        explicit FilterSettingRecord(TiXmlElement *element);
    };
    
    std::ostream& operator<< (std::ostream& out, const LifSerieHeader &s );
    std::ostream& operator<< (std::ostream& out, const LifHeader &r );
    std::ostream& operator<< (std::ostream& out, const DimensionData &r );
}
#endif
