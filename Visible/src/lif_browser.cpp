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
#include "timed_types.h"
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
lif_browser::lif_browser (const std::string&  fqfn_path) :
mFqfnPath(fqfn_path), m_data_ready(false) {
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


void  lif_browser::internal_get_series_info () const
{
    if ( exists(boost::filesystem::path(mFqfnPath)))
    {
        m_lifRef =  lifIO::LifReader::create(mFqfnPath);
        m_series_book.clear ();
        m_series_names.clear();
        
        for (unsigned ss = 0; ss < m_lifRef->getNbSeries(); ss++)
        {
            lif_serie_data si(m_lifRef, ss);
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


