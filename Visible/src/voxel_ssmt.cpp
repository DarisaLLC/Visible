//
//  voxel_segmentation.cpp
//  Visible
//
//  Created by Arman Garakani on 5/26/19.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <map>
#include "timed_types.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "ssmt.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"
#include "segmentation_parameters.hpp"
#include <OpenImageIO/imageio.h>
#include "algo_runners.hpp"
#include "etw_utils.hpp"

using namespace stl_utils;
using namespace boost;
namespace bfs=boost::filesystem;
using namespace OIIO;

namespace anonymous{

    void ransac_non_maximal_suppression(const std::vector<Eigen::Vector3d> &N,
                          Eigen::Vector3d &M) {
        M = etw_utils::ransac(
                              N, []() -> Eigen::Vector3d { return Eigen::Vector3d::Zero(); },
                              [](auto &n, auto &nest) {
            return std::fabs(nest.z() - n.z()) > 3;
        },
                              [](const auto &ave, const auto &n, const auto &est) -> Eigen::Vector3d {
            if (std::fabs(est.z() - n.z()) < 2)
                return ave - n;
            else
                return ave + n;
        });
    }
}

// Return 2D latice of pixels over time
void ssmt_processor::generateVoxels_on_channel (const int channel_index){
    generateVoxelsAndSelfSimilarities(m_all_by_channel[channel_index]);
}


// Generate latice of voxel self-similarity
/*
 * parameters: m_voxel_sample, m_expected_segmented_size,
 */

void ssmt_processor::generateVoxelsAndSelfSimilarities (const std::vector<roiWindow<P8U>>& images){
    
    bool cache_ok = false;
    std::shared_ptr<internalContainer> ssref;

    voxel_processor vp;
    vp.sample(m_voxel_sample.first, m_voxel_sample.second);
    vp.image_size(m_loaded_spec.getSectionSize().first, m_loaded_spec.getSectionSize().second);
    
    if(bfs::exists(mCurrentCachePath)){
        auto cache_path = mCurrentCachePath / m_params.internal_container_cache_name ();
        if(bfs::exists(cache_path)){
            ssref = internalContainer::create(cache_path);
        }
        cache_ok = ssref && ssref->size_check(m_expected_segmented_size.first*m_expected_segmented_size.second);
    }
    
    if(cache_ok){
        vlogger::instance().console()->info(" IC container cache : Hit ");
        m_voxel_entropies.insert(m_voxel_entropies.end(), ssref->data().begin(),
                                 ssref->data().end());
            // Call the voxel ready cb if any
        if (signal_ss_voxel_ready && signal_ss_voxel_ready->num_slots() > 0)
            signal_ss_voxel_ready->operator()(m_voxel_entropies);
        
    }else{ // Fill Cache
        
        auto cache_path = mCurrentCachePath / m_params.internal_container_cache_name ();
        
        vlogger::instance().console()->info("starting generating voxel self-similarity");
   
        if (vp.generate_voxel_space(images)){
            
            vlogger::instance().console()->info("copying results of voxel self-similarity");
            m_voxel_entropies = vp.entropies();
            
                // Call the voxel ready cb if any
            if (signal_ss_voxel_ready && signal_ss_voxel_ready->num_slots() > 0)
                signal_ss_voxel_ready->operator()(m_voxel_entropies);
            
                // Fill the Cache
            bool ok = internalContainer::store(cache_path, m_voxel_entropies);
            if(ok)
                vlogger::instance().console()->info(" SS result container cache : filled ");
            else
                vlogger::instance().console()->info(" SS result container cache : failed ");
        }
    }
    assert(m_voxel_entropies.empty() == false);
}
    
void ssmt_processor::create_voxel_surface (std::vector<float>& env){
    voxel_processor vp;
    vp.sample(m_voxel_sample.first, m_voxel_sample.second);
    vp.image_size(m_loaded_spec.getSectionSize().first, m_loaded_spec.getSectionSize().second);
    
    if (vp.generate_voxel_surface(env)){
        
        m_temporal_ss = vp.temporal_ss();
        
        cv::Mat bi_level;
        int erosion_size = 3;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                            Size( 2*erosion_size + 1, 2*erosion_size+1 ),
                                            Point( erosion_size, erosion_size ) );
        
        // Get threshold using ransac for doing non_maximal_suppression
        std::vector<Eigen::Vector3d> M(3);
        anonymous::ransac_non_maximal_suppression(vp.cloud(), M[0]);
        auto rthreshold = (int) M[0][2];
        
        auto othreshold = threshold(m_temporal_ss, bi_level, 0, 255, THRESH_OTSU | THRESH_BINARY);
        threshold(m_temporal_ss, bi_level, (rthreshold+othreshold)/2, 255, THRESH_BINARY);
        
        std::string msg = " Otsu Threshold  @ (" + to_string(othreshold) +  " RANSAC Threshold    " + to_string(rthreshold) + ")";
        vlogger::instance().console()->info("starting " + msg);
        
        
        cv::erode(bi_level, bi_level, element);
        
        vlogger::instance().console()->info("finished voxel surface");
        if(bfs::exists(mCurrentCachePath)){
            std::string imagename = "voxel_ss_.png";
            auto image_path = mCurrentCachePath / imagename;
            std::unique_ptr<ImageOutput> out = ImageOutput::create (image_path.c_str());
            if (out){
                ImageSpec spec (m_temporal_ss.cols, m_temporal_ss.rows, m_temporal_ss.channels(), TypeDesc::UINT8);
                out->open (image_path.c_str(), spec);
                out->write_image (TypeDesc::UINT8, m_temporal_ss.data);
                out->close ();
            }
            
        }
        
            // Call the voxel ready cb if any
        if (signal_segmented_view_ready && signal_segmented_view_ready->num_slots() > 0){
            signal_segmented_view_ready->operator()(m_temporal_ss, bi_level);
        }
    }
}

#pragma GCC diagnostic pop
