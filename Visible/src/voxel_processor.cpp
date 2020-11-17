

#include "core/pair.hpp"
#include "logger/logger.hpp"
#include "sm_producer.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "cinder_xchg.hpp"  // For Rectf @todo remove dependency on cinder
#include <future>
#include <memory>
#include "algo_runners.hpp"



bool voxel_processor::generate_voxel_space (const std::vector<roiWindow<P8U>>& images){
    if (m_load(images, m_voxel_sample.first, m_voxel_sample.second))
        return m_internal_generate();
    return false;
}

voxel_processor::voxel_processor(){
        // semilarity producer
    m_sm_producer = std::shared_ptr<sm_producer> ( new sm_producer () );
}


const smProducerRef  voxel_processor::similarity_producer () const {
    return m_sm_producer;
}

bool  voxel_processor::m_internal_generate() {
    auto sp = similarity_producer();
    sp->load_images(m_voxels);
    std::packaged_task<bool()> task([sp]() { return sp->operator()(0); });       // wrap the function
    std::future<bool> future_ss = task.get_future(); // get a future
    std::thread(std::move(task)).join();             // Finish on a thread
    vlogger::instance().console()->info("dispatched voxel self-similarity");
    if (future_ss.get()){
        vlogger::instance().console()->info("copying results of voxel self-similarity");
        const deque<double>& entropies = sp->shannonProjection ();
        m_voxel_entropies.insert(m_voxel_entropies.end(), entropies.begin(), entropies.end());
        auto extremes = svl::norm_min_max(m_voxel_entropies.begin(),m_voxel_entropies.end());
        std::string msg = to_string(extremes.first) + "," + to_string(extremes.second);
        vlogger::instance().console()->info(msg);
        return m_voxel_entropies.size() == m_voxels.size();
    }
    return false;
}

bool  voxel_processor::generate_voxel_surface (const std::vector<float>& ven){

    uiPair size_diff = m_half_offset + m_half_offset;
    uint32_t width = m_expected_segmented_size.first;
    uint32_t height = m_expected_segmented_size.second;
    assert(width*height == ven.size());

    
    cv::Mat ftmp = cv::Mat(height, width, CV_32F);
    std::vector<float>::const_iterator start = ven.begin();
    m_hist.resize(256,0);
    
    
        // Fillup floating image and hist of 8bit values
    for (auto row =0; row < height; row++){
        float* f_row_ptr = ftmp.ptr<float>(row);
        auto end = start + width; // point to one after
        for (; start < end; start++, f_row_ptr++){
            if(std::isnan(*start)) continue; // nan is set to 1, the pre-set value
            float valf(*start);
            uint8_t val8 (valf*255);
            m_hist[val8]++;
            *f_row_ptr = valf;
        }
        start = end;
    }
    
        // Median Smooth, Expand, CheckSize
    cv::medianBlur(ftmp, ftmp, 5);
    
        // Straight resize. Add pads afterwards
    cv::Size bot(ftmp.cols * m_voxel_sample.first,ftmp.rows * m_voxel_sample.second);
    cv::Mat f_temporal (bot, CV_32F);
    cv::resize(ftmp, f_temporal, bot, 0, 0, INTER_NEAREST);
    ftmp = getPadded(f_temporal, m_half_offset, 0.0);
    ftmp = ftmp * 255.0f;
    ftmp.convertTo(m_temporal_ss, CV_8U);
    
    m_measured_area = Rectf(0,0,bot.width, bot.height);
    bool ok = m_temporal_ss.cols == m_measured_area.getWidth() + size_diff.first;
    ok = ok && m_temporal_ss.rows == m_measured_area.getHeight() + size_diff.second;
    
    return ok;
}


bool  voxel_processor::m_load(const std::vector<roiWindow<P8U>> &images, uint32_t sample_x,uint32_t sample_y) {
    sample(sample_x, sample_y);
    m_voxel_length = images.size();
    image_size(images[0].width(), images[0].height());
    uint32_t expected_width = m_expected_segmented_size.first;
    uint32_t expected_height = m_expected_segmented_size.second;
    
    m_voxels.resize(0);
    std::string msg = " Generating Voxels @ (" +
    to_string(m_voxel_sample.first) + "," +
    to_string(m_voxel_sample.second) + ")";
    msg += "Expected Size: " + to_string(expected_width) + " x " +
    to_string(expected_height);
    vlogger::instance().console()->info("starting " + msg);
    
    int count = 0;
    int row, col;
    for (row = m_half_offset.second; row < m_image_size.second;
         row += m_voxel_sample.second) {
        for (col = m_half_offset.first; col < m_image_size.first;
             col += m_voxel_sample.first) {
            std::vector<uint8_t> voxel(m_voxel_length);
            for (auto tt = 0; tt < m_voxel_length; tt++) {
                voxel[tt] = images[tt].getPixel(col, row);
            }
            count++;
            m_voxels.emplace_back(voxel);
        }
        expected_height -= 1;
    }
    expected_width -= (count / m_expected_segmented_size.second);
    
    bool ok = expected_width == expected_height && expected_width == 0;
    msg = " remainders: " + to_string(expected_width) + " x " +
    to_string(expected_height);
    vlogger::instance().console()->info(ok ? "finished ok "
                                        : "finished with error " + msg);
    return ok;
}

