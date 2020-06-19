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
#include "timed_types.h"
#include "core/signaler.h"
#include "lif_content.hpp"
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
lif_serie_data:: lif_serie_data (const lifIO::LifReader::ref& m_lifRef, const unsigned index):
m_index(-1) {
    
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
    std::transform(m_lifRef->getSerie(m_index).getTimestamps().begin(), m_lifRef->getSerie(m_index).getTimestamps().end(),
                   m_timeSpecs.begin(), [](lifIO::LifSerie::timestamp_t ts) { return time_spec_t ( ts / 10000.0); });
    m_length_in_seconds = m_lifRef->getSerie(m_index).total_duration ();
    auto serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(m_index), stl_utils::null_deleter());
    
    // Fill in channel number of image for posters
    // For each time point, images are loaded n channels at a time.
    // height = image height * number of channels
    // width = image width;
    m_buffer2d_dimensions.resize(2);
    m_buffer2d_dimensions[1] = m_dimensions[1] * m_channelCount;
    m_buffer2d_dimensions[0] = m_dimensions[0];
    cv::Mat dst ( static_cast<uint32_t>(m_buffer2d_dimensions[1]),
                 static_cast<uint32_t>(m_buffer2d_dimensions[0]), CV_8U);
    serie_ref->fill2DBuffer(dst.ptr(0), 0);
    m_poster = dst;
    
    // Setup multichannel ROIs for this lif.
    // See above
    cv::Size2d roi_size (m_dimensions[0], m_dimensions[1]);
    m_rois_2d.resize(0);
    for (auto cc = 0; cc < m_channelCount; cc++){
        cv::Point2i tl(0,static_cast<int>(cc * m_dimensions[1]));
        m_rois_2d.emplace_back(tl,roi_size);
    }
    
    // Retain a week pointer to the parent LIF 
    m_lifWeakRef = m_lifRef;
    
}


/**
 readerWeakRef
 @note Check if the returned has expired
 @return a weak_ptr to the reader data
 */
const lifIO::LifReader::weak_ref_t& lif_serie_data::readerWeakRef () const{
    return m_lifWeakRef;
}


