//
//  lif_serie.cpp
//  Visible
//
//  Created by Arman Garakani on 5/26/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wunused-private-field"


#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <map>
#include "async_tracks.h"
#include "core/signaler.h"
#include "algo_Lif.hpp"
#include "logger/logger.hpp"


using namespace std;




/****
 
 lif_serie implementation
 
 ****/

lif_serie_data:: lif_serie_data () : m_index (-1) {}

/**
 lif_serie_data : a directory of information inside a LIF Serie

 @param m_lifRef Reference to Lif File Decode
 @param index Serie of Interest Index
 @param ct LIF Custome file tag

 */
lif_serie_data:: lif_serie_data (const lifIO::LifReader::ref& m_lifRef, const unsigned index, const lifIO::ContentType_t& ct):
m_index(-1), m_contentType(ct) {
    
    if (index >= m_lifRef->getNbSeries()) return;
    
    m_index = index;
    m_name = m_lifRef->getSerie(m_index).getName();
    m_timesteps = static_cast<uint32_t>(m_lifRef->getSerie(m_index).getNbTimeSteps());
    m_pixelsInOneTimestep = static_cast<uint32_t>(m_lifRef->getSerie(m_index).getNbPixelsInOneTimeStep());
    m_dimensions = m_lifRef->getSerie(m_index).getSpatialDimensions();
    m_channelCount = static_cast<uint32_t>(m_lifRef->getSerie(m_index).getChannels().size());
    m_channels.clear ();
    for (lifIO::ChannelData cda : m_lifRef->getSerie(m_index).getChannels())
    {
        m_channel_names.push_back(cda.getName());
        m_channels.emplace_back(cda);
    }
    
    // Get timestamps in to time_spec_t and store it in info
    m_timeSpecs.resize (m_lifRef->getSerie(m_index).getTimestamps().size());
    
    // Adjust sizes based on the number of bytes
    std::transform(m_lifRef->getSerie(m_index).getTimestamps().begin(), m_lifRef->getSerie(m_index).getTimestamps().end(),
                   m_timeSpecs.begin(), [](lifIO::LifSerie::timestamp_t ts) { return time_spec_t ( ts / 10000.0); });
    
    m_length_in_seconds = m_lifRef->getSerie(m_index).total_duration ();
    
    auto serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(m_index), stl_utils::null_deleter());
    // opencv rows, cols
    uint64_t rows (m_dimensions[1] * m_channelCount);
    uint64_t cols (m_dimensions[0]);
    cv::Mat dst ( static_cast<uint32_t>(rows),static_cast<uint32_t>(cols), CV_8U);
    serie_ref->fill2DBuffer(dst.ptr(0), 0);
    m_poster = dst;
    
    if(m_dimensions[0] == 512 && m_dimensions[1] == 128 && m_channels.size() == 3 &&
       serie_ref->content_type() == ""){
        m_contentType = "IDLab_0";
        serie_ref->set_content_type(m_contentType);
    }
    m_lifWeakRef = m_lifRef;
    
}


/**
 readerWeakRef
 @note Check if the returned has expired
 @return a weak_ptr to the reader data
 */
const lifIO::LifReader::weak_ref& lif_serie_data::readerWeakRef () const{
    return m_lifWeakRef;
}


