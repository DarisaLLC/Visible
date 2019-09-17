//
//  result_ssmt.cpp
//  Visible
//
//  Created by Arman Garakani on 8/20/18.
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
#include "ssmt.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"


using cc_t = ssmt_processor::contractionContainer_t;

////////// ssmt_result Implementataion //////////////////
const ssmt_result::ref_t ssmt_result::create (std::shared_ptr<ssmt_processor>& parent, const moving_region& child,
                                              size_t idx,const input_channel_selector_t& in){
    ssmt_result::ref_t this_child (new ssmt_result(child, idx,in ));
    this_child->m_weak_parent = parent;

    // Support lifProcessor::initial ss results available
    std::function<void (const input_channel_selector_t&)> sm1d_ready_cb = boost::bind (&ssmt_result::signal_sm1d_ready, this_child, _1);
    boost::signals2::connection nl_connection = parent->registerCallback(sm1d_ready_cb);
    return this_child;
}

ssmt_result::ssmt_result(const moving_region& mr,size_t idx,const input_channel_selector_t& in):
moving_region(mr), m_idx(idx), m_input(in) {
    // Create a contraction object
    m_caRef = contractionLocator::create ();

    
    // Suport lif_processor::Contraction Analyzed
    std::function<void (cc_t&)>ca_analyzed_cb = boost::bind (&ssmt_result::contraction_ready, this, _1);
    boost::signals2::connection ca_connection = m_caRef->registerCallback(ca_analyzed_cb);
    
}


void ssmt_result::contraction_ready (ssmt_processor::contractionContainer_t& contractions)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Call the contraction available cb if any
    auto shared_parent = m_weak_parent.lock();
    if (shared_parent && shared_parent->signal_contraction_ready && shared_parent->signal_contraction_ready->num_slots() > 0)
    {
        cc_t copied = m_caRef->contractions();
        shared_parent->signal_contraction_ready->operator()(copied);
    }
    vlogger::instance().console()->info(" Contractions Analyzed: ");
}

//@todo remove this hack as it is to run a particular file and get results.
void ssmt_result::signal_sm1d_ready(const input_channel_selector_t& in){
    if(m_input.region() != in.region()) return;
    m_caRef->find_best();
    
    
}

const input_channel_selector_t& ssmt_result::input() const { return m_input; }

size_t ssmt_result::Id() const { return m_idx; }
const std::shared_ptr<contractionLocator> & ssmt_result::locator () const { return m_caRef; }

const ssmt_processor::channel_vec_t& ssmt_result::content () const { return m_all_by_channel; }

// @TBD not-used check if this is necessary 
void ssmt_result::process (){
    bool done = get_channels(m_input.channel());
    if(done){
        auto parent = m_weak_parent.lock();
        m_contraction_pci_tracks_asyn = std::async(std::launch::async, &ssmt_processor::run_contraction_pci_on_selected_input, parent, m_input);
    }
}

bool ssmt_result::get_channels (int channel){
    auto parent = m_weak_parent.lock();
    if (parent.get() == 0) return false;
    m_channel_count = parent->channel_count();
    assert(parent->channel_count() > 0);
    m_all_by_channel.resize (m_channel_count);
    
    assert(channel>=0 && channel < m_channel_count);
    const vector<roiWindow<P8U> >& rws = parent->content()[channel];
    
    
    auto affineCrop = [] (const vector<roiWindow<P8U> >::const_iterator& rw_src, const cv::RotatedRect& rect){
        
        
        auto matAffineCrop = [] (Mat input, const RotatedRect& box){
            double angle = box.angle;
            auto size = box.size;
            
            auto transform = getRotationMatrix2D(box.center, angle, 1.0);
            Mat rotated;
            warpAffine(input, rotated, transform, input.size(), INTER_CUBIC);
            
            //Crop the result
            Mat cropped;
            getRectSubPix(rotated, size, box.center, cropped);
            copyMakeBorder(cropped,cropped,0,0,0,0,BORDER_CONSTANT,Scalar(0));
            roiWindow<P8U> r8(cropped.cols, cropped.rows);
            cpCvMatToRoiWindow8U (cropped, r8);
            return r8;
        };
        
        cvMatRefroiP8U(*rw_src, src, CV_8UC1);
        return matAffineCrop(src, rect);
        
    };
    

    vector<roiWindow<P8U> >::const_iterator vitr = rws.begin();
    uint32_t count = 0;
    uint32_t total = static_cast<uint32_t>(rws.size());
    do
    {
        const auto& rr = rotated_roi();
        auto affine = affineCrop(vitr, rr);
        m_all_by_channel[channel].emplace_back(affine);
        count++;
    }
    while (++vitr != rws.end());
    bool check = m_all_by_channel[channel].size() == total;
    return check && count == total;
}
