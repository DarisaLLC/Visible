//
//  algo_shortterm.cpp
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
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "cinder_cv/cinder_opencv.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "algo_Lif.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"




/**
 Experimental: compute short term self-similarity
 
 @param halfWinSz half size of the temporal window
 */
void lif_serie_processor::compute_shortterm (const uint32_t halfWinSz) const{
    
    uint32_t tWinSz = 2 * halfWinSz + 1;
    
    auto shannon = [](double r) { return (-1.0 * r * log2 (r)); };
    double _log2MSz = log2(tWinSz);
    
    
    for (uint32_t diag = halfWinSz; diag < (m_frameCount - halfWinSz); diag++) {
        // tWinSz x tWinSz centered at diag,diag putting out a readount at diag
        // top left at row, col
        auto tl_row = diag - halfWinSz; auto br_row = tl_row + tWinSz;
        auto tl_col = diag - halfWinSz; auto br_col = tl_col + tWinSz;
        
        // Sum rows in to _sums @note possible optimization by constant time implementation
        std::vector<double> _sums (tWinSz, 0);
        std::vector<double> _entropies (tWinSz, 0);
        for (auto row = tl_row; row < br_row; row++){
            auto proj_row = row - tl_row;
            _sums[proj_row] = m_smat[row][row];
        }
        
        for (auto row = tl_row; row < br_row; row++)
            for (auto col = (row+1); col < br_col; col++){
                auto proj_row = row - tl_row;
                auto proj_col = col - tl_col;
                _sums[proj_row] += m_smat[row][col];
                _sums[proj_col] += m_smat[row][col];
            }
        
        
        for (auto row = tl_row; row < br_row; row++)
            for (auto col = row; col < br_col; col++){
                auto proj_row = row - tl_row;
                auto proj_col = col - tl_col;
                double rr = m_smat[row][col]/_sums[proj_row]; // Normalize for total energy in samples
                _entropies[proj_row] += shannon(rr);
                
                if ((proj_row) != (proj_col)) {
                    rr = m_smat[row][col]/_sums[proj_col];//Normalize for total energy in samples
                    _entropies[proj_col] += shannon(rr);
                }
                _entropies[proj_row] = _entropies[proj_row]/_log2MSz;// Normalize for cnt of samples
            }
        
        std::lock_guard<std::mutex> lk(m_shortterms_mutex);
        m_shortterms.push(1.0f - _entropies[halfWinSz]);
        m_shortterm_cv.notify_one();
    }
}



/**
 Experimental: short term self-similarity
 
 @param halfWinSz half size of the temporal window
 */
void lif_serie_processor::shortterm_pci (const uint32_t& halfWinSz) {
    
    
    // Check if full sm has been done
    m_shortterm_pci_tracks.at(0).second.clear();
    for (auto pp = 0; pp < halfWinSz; pp++){
        timedVal_t res;
        index_time_t ti;
        ti.first = pp;
        res.first = ti;
        res.second = -1.0f;
        m_shortterm_pci_tracks.at(0).second.emplace_back(res);
    }
    
    compute_shortterm(halfWinSz);
    //    auto twinSz = 2 * halfWinSz + 1;
    auto ii = halfWinSz;
    auto last = m_frameCount - halfWinSz;
    while(true){
        std::unique_lock<std::mutex> lock( m_shortterms_mutex);
        m_shortterm_cv.wait(lock,[this]{return !m_shortterms.empty(); });
        float val = m_shortterms.front();
        timedVal_t res;
        m_shortterms.pop();
        lock.unlock();
        index_time_t ti;
        ti.first = ii++;
        res.first = ti;
        res.second = static_cast<float>(val);
        m_shortterm_pci_tracks.at(0).second.emplace_back(res);
        //        std::cout << m_shortterm_pci_tracks.at(0).second.size() << "," << m_shortterms.size() << std::endl;
        
        if(ii == last)
            break;
    }
    // Fill the pad in front with first valid read
    for (auto pp = 0; pp < halfWinSz; pp++){
        m_shortterm_pci_tracks.at(0).second.at(pp) = m_shortterm_pci_tracks.at(0).second.at(halfWinSz);
    }
    // Fill the pad in the back with the last valid read
    for (auto pp = 0; pp < halfWinSz; pp++){
        m_shortterm_pci_tracks.at(0).second.emplace_back (m_shortterm_pci_tracks.at(0).second.back());
    }
    
}

