//
//  lif_browser.cpp
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
#include "timed_value_containers.h"
#include "lif_content.hpp"
#include "logger/logger.hpp"

using namespace std;




/****
 
 lif_browser implementation
 
 ****/


/**
 Lif Browser ( Used in Visible App for visually browsing and selecting with a LIF File

 @param fqfn_path full qualified path to the LIF file
 @param ct Custom LIF file type
 */
lif_browser::lif_browser (const std::string&  fqfn_path, const lifIO::ContentType_t& ct) :
mFqfnPath(fqfn_path), m_content_type(ct), m_data_ready(false) {
    if ( boost::filesystem::exists(boost::filesystem::path(mFqfnPath)) )
    {
        std::string msg = " Loaded Series Info for " + mFqfnPath ;
        vlogger::instance().console()->info(msg);
        internal_get_series_info();
        
    }
}
const lif_serie_data  lif_browser::get_serie_by_index (unsigned index){
    lif_serie_data si;
    get_series_info();
    if (index < m_series_book.size())
        si = m_series_book[index];
    return si;
    
}

const std::vector<lif_serie_data>& lif_browser::get_all_series  () const{
    get_series_info();
    return m_series_book;
}

const std::vector<std::string>& lif_browser::names () const{
    get_series_info();
    return m_series_names;
    
}

/**
 get_first_frame
 @ note LIF files are plane organized. 3 Channel LIF file is 3 * rows by cols by ONE byte.
 @param si Serie Data Index
 @param frameCount Not Used
 @param out cv::Mat reference for the output
 */
void lif_browser::get_first_frame (lif_serie_data& si,  const int frameCount, cv::Mat& out)
{
    get_series_info();
    
    auto serie_ref = std::shared_ptr<lifIO::LifSerie>(&m_lifRef->getSerie(si.index()), stl_utils::null_deleter());
    // opencv rows, cols
    uint64_t rows (si.dimensions()[1] * si.channelCount());
    uint64_t cols (si.dimensions()[0]);
    cv::Mat dst ( static_cast<uint32_t>(rows),static_cast<uint32_t>(cols), CV_8U);
    serie_ref->fill2DBuffer(dst.ptr(0), 0);
    out = dst;
}


void  lif_browser::internal_get_series_info () const
{
    if ( exists(boost::filesystem::path(mFqfnPath)))
    {
        m_lifRef =  lifIO::LifReader::create(mFqfnPath, m_content_type);
        m_series_book.clear ();
        m_series_names.clear();
        
        for (unsigned ss = 0; ss < m_lifRef->getNbSeries(); ss++)
        {
            lif_serie_data si(m_lifRef, ss, m_content_type);
            m_series_book.emplace_back (si);
            m_series_names.push_back(si.name());
            // Fill up names / index map -- convenience
            auto index = static_cast<int>(ss);
            m_name_to_index[si.name()] = index;
            m_index_to_name[index] = si.name();
        }
        m_data_ready.store(true, std::memory_order_release);
    }
}

// Yield while finishing up
void  lif_browser::get_series_info () const
{
    while(!m_data_ready.load(std::memory_order_acquire)){
        std::this_thread::yield();
    }
}


