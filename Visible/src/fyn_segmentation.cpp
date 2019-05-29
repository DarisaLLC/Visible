//
//  voxel_segmentation.cpp
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
#include "sm_producer.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "cinder_cv/cinder_opencv.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "algo_Lif.hpp"
#include "logger/logger.hpp"
#include "cpp-perf.hpp"
#include "result_serialization.h"

#include "segmentation_parameters.hpp"




// Return 2D latice of pixels over time
void lif_serie_processor::generateVoxels_on_channel (const int channel_index){
    generateVoxels(m_all_by_channel[channel_index]);
}


// Return 2D latice of voxel self-similarity

void lif_serie_processor::generateVoxelsAndSelfSimilarities (const std::vector<roiWindow<P8U>>& images){
    
    generateVoxels(images);
    generateVoxelSelfSimilarities();

}



void lif_serie_processor::generateVoxels (const std::vector<roiWindow<P8U>>& images){
    size_t t_d = images.size();
    auto half_offset = m_voxel_sample / 2;
    uint32_t width = images[0].width() - half_offset.first;
    uint32_t height = images[0].height() - half_offset.second;
    uint32_t expected_width = m_expected_segmented_size.first;
    uint32_t expected_height = m_expected_segmented_size.second;
    
    m_voxels.resize(0);
    std::string msg = " Generating Voxels @ (" + to_string(m_voxel_sample.first) + "," + to_string(m_voxel_sample.second) + ")";
    msg += "Expected Size: " + to_string(expected_width) + " x " + to_string(expected_height);
    vlogger::instance().console()->info("starting " + msg);
    
    int count = 0;
    int row, col;
    for (row = half_offset.second; row < height; row+=m_voxel_sample.second){
        for (col = half_offset.first; col < width; col+=m_voxel_sample.first){
            std::vector<uint8_t> voxel(t_d);
            for (auto tt = 0; tt < t_d; tt++){
                voxel[tt] = images[tt].getPixel(col, row);
            }
            count++;
            m_voxels.emplace_back(voxel);
        }
        expected_height -= 1;
    }
    expected_width -= (count / m_expected_segmented_size.second);
    
    bool ok = expected_width == expected_height && expected_width == 0;
    msg = " remainders: " + to_string(expected_width) + " x " + to_string(expected_height);
    vlogger::instance().console()->info(ok ? "finished ok " : "finished with error " + msg);
    
    
}

void lif_serie_processor::create_voxel_surface(std::vector<float>& ven){
    
    uint32_t width = m_expected_segmented_size.first;
    uint32_t height = m_expected_segmented_size.second;
    assert(width*height == ven.size());
    auto extremes = svl::norm_min_max(ven.begin(),ven.end());
    std::string msg = to_string(extremes.first) + "," + to_string(extremes.second);
    vlogger::instance().console()->info(msg);
    
    cv::Mat tmp = cv::Mat(height, width, CV_8U);
    std::vector<float>::const_iterator start = ven.begin();
    
    for (auto row =0; row < height; row++){
        uint8_t* row_ptr = tmp.ptr<uint8_t>(row);
        auto end = start + width; // point to one after
        for (; start < end; start++, row_ptr++){
            if(std::isnan(*start)) continue; // nan is set to 1, the pre-set value
            *row_ptr = uint8_t((*start)*255);
        }
        start = end;
    }
    
    // Median Blur before up sizeing.
    cv::medianBlur(tmp, tmp, 5);
    cv::Size bot(tmp.cols * m_voxel_sample.first, tmp.rows * m_voxel_sample.second);
    m_temporal_ss = cv::Mat(bot, CV_8U);
    cv::resize(tmp, m_temporal_ss, bot, 0, 0, INTER_NEAREST);
    m_measured_area = Rectf(0,0,bot.width, bot.height);
    assert(m_temporal_ss.cols == m_measured_area.getWidth());
    assert(m_temporal_ss.rows == m_measured_area.getHeight());
    
    cv::Mat bi_level;
    
    threshold(m_temporal_ss, bi_level, 126, 255, THRESH_BINARY | THRESH_OTSU);

    // Pad both with zeros
    // Symmetric: i.e. pad on both sides
    iPair pad (m_params.normalized_symmetric_padding().first * m_temporal_ss.cols,
               m_params.normalized_symmetric_padding().second * m_temporal_ss.rows);
    
    auto get_padded = [pad = pad](const cv::Mat& src){
        cv::Mat padded (src.rows+2*pad.second, src.cols+2*pad.first, CV_8U);
        padded.setTo(0.0);
        cv::Mat win (padded, cv::Rect(pad.first, pad.second, src.cols, src.rows));
        src.copyTo(win);
        return padded;
    };
    
    auto mono_padded = get_padded(m_temporal_ss);
    auto bi_level_padded = get_padded(bi_level);
    
    // Translation to undo padding
    pad.first = -pad.first;
    pad.second = -pad.second;
    
    vlogger::instance().console()->info("finished voxel surface");
    if(fs::exists(mCurrentSerieCachePath)){
        std::string imagename = "voxel_ss_.png";
        auto image_path = mCurrentSerieCachePath / imagename;
        cv::imwrite(image_path.string(), m_temporal_ss);
    }
    
    // Call the voxel ready cb if any
    if (signal_ss_image_available && signal_ss_image_available->num_slots() > 0){
        signal_ss_image_available->operator()(mono_padded, bi_level_padded, pad);
    }
    
    
}

void lif_serie_processor::generateVoxelSelfSimilarities (){
    
    bool cache_ok = false;
    std::shared_ptr<internalContainer> ssref;
    
    if(fs::exists(mCurrentSerieCachePath)){
        auto cache_path = mCurrentSerieCachePath / m_params.internal_container_cache_name ();
        if(fs::exists(cache_path)){
            ssref = internalContainer::create(cache_path);
        }
        cache_ok = ssref && ssref->size_check(m_expected_segmented_size.first*m_expected_segmented_size.second);
    }
    
    if(cache_ok){
        vlogger::instance().console()->info(" IC container cache : Hit ");
        m_voxel_entropies.insert(m_voxel_entropies.end(), ssref->data().begin(),
                                 ssref->data().end());
        // Call the voxel ready cb if any
        if (signal_ss_voxel_available && signal_ss_voxel_available->num_slots() > 0)
            signal_ss_voxel_available->operator()(m_voxel_entropies);
        
    }else{
        
        auto cache_path = mCurrentSerieCachePath / m_params.internal_container_cache_name ();
        
        vlogger::instance().console()->info("starting generating voxel self-similarity");
        auto sp =  similarity_producer();
        sp->load_images (m_voxels);
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).join(); // Finish on a thread
        vlogger::instance().console()->info("dispatched voxel self-similarity");
        if (future_ss.get())
        {
            vlogger::instance().console()->info("copying results of voxel self-similarity");
            const deque<double>& entropies = sp->shannonProjection ();
            m_voxel_entropies.insert(m_voxel_entropies.end(), entropies.begin(), entropies.end());
            
            // Call the voxel ready cb if any
            if (signal_ss_voxel_available && signal_ss_voxel_available->num_slots() > 0)
                signal_ss_voxel_available->operator()(m_voxel_entropies);
            
            bool ok = internalContainer::store(cache_path, m_voxel_entropies);
            if(ok)
                vlogger::instance().console()->info(" SS result container cache : filled ");
            else
                vlogger::instance().console()->info(" SS result container cache : failed ");
            
            
        }
    }
}
