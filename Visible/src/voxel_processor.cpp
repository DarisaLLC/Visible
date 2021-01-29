
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"

#include "core/pair.hpp"
#include "logger/logger.hpp"
#include "sm_producer.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "cinder_xchg.hpp"  // For Rectf @todo remove dependency on cinder
#include <future>
#include <memory>
#include "algo_runners.hpp"

using namespace svl;

bool scaleSpace::generate(const std::vector<cv::Mat>& images,
                                                  int start_sigma, int end_sigma, int step){
    
    // check start, end and step are positive and end-start is multiple of step
    m_filtered.resize(0);
    m_scale_space.resize(0);
    
    int step_range = end_sigma - start_sigma;
    if (step == 0 || step_range == 0 || (step_range%step) != 0) return false;
    int steps = (end_sigma - start_sigma)/step;
    if (steps < 4) return false;

    // make a cv::mat clone of input images
    cv::Size isize(images[0].cols, images[0].rows);
    for (const auto& rw : images){
        if(rw.cols != isize.width || rw.rows != isize.height) return false;
        auto clone = rw.clone();
        clone.convertTo(clone, CV_64F);
        m_filtered.push_back(clone);
    }
    m_loaded = m_filtered.size() == images.size();
    int n = static_cast<int>(images.size());
    int n2 = n * (n - 1);
    for (auto scale = start_sigma; scale <= end_sigma; scale+=step){
        cv::Mat sum = cv::Mat(isize.height, isize.width, CV_64F);
        cv::Mat sumsq = cv::Mat(isize.height, isize.width, CV_64F);
        sum = 0;
        sumsq = 0;
        for (auto& img : m_filtered){
            auto mm = img.clone();
            cv::GaussianBlur(mm, mm, cv::Size(0,0), scale);
            sum += mm;
            cv::multiply(mm, mm, mm);
            sumsq += mm;
        }
        cv::multiply(sum, sum, sum);
        sumsq *= n;
        cv::subtract(sumsq, sum, sumsq);
        sumsq /= n2;
        m_scale_space.push_back(sumsq);
    }


    return true;
    
}

bool voxel_processor::generate_voxel_space (const std::vector<roiWindow<P8U>>& images,
                                            const std::vector<int>& indicies){
    if (m_load(images, m_voxel_sample.first, m_voxel_sample.second, indicies))
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
//    std::memcpy(ftmp.data, ven.data(),ven.size()*sizeof(float));
    
        // Fillup floating image and hist of 8bit values
    for (auto row =0; row < height; row++){
        float* f_row_ptr = ftmp.ptr<float>(row);
        auto end = start + width; // point to one after
        for (; start < end; start++, f_row_ptr++){
            if(std::isnan(*start)) continue; // nan is set to 1, the pre-set value
            *f_row_ptr = *start;
        }
        start = end;
    }

    
        // Median Smooth, Expand, CheckSize
    cv::GaussianBlur(ftmp, ftmp, cv::Size(0,0), 2.0);
    cv::normalize(ftmp, ftmp,  1.0, 0.0, NORM_INF);
    
        // Straight resize. Add pads afterwards
    cv::Size bot(ftmp.cols * m_voxel_sample.first,ftmp.rows * m_voxel_sample.second);
    cv::Mat f_temporal (bot, CV_32F);
    cv::resize(ftmp, f_temporal, bot, 0, 0, INTER_NEAREST);
    ftmp = getPadded(f_temporal, m_half_offset, 0.0);
    ftmp = ftmp * 255.0f;
    ftmp.convertTo(m_temporal_ss, CV_8U);
    
    m_cloud.reserve(width*height);
    for(auto row = 0; row < height; row++){
        for (auto col = 0; col < width; col++){
            m_cloud.emplace_back(col, row, (double) m_temporal_ss.at<uint8_t>(row,col));
        }
    }
    
    m_measured_area = Rectf(0,0,bot.width, bot.height);
    bool ok = m_temporal_ss.cols == m_measured_area.getWidth() + size_diff.first;
    ok = ok && m_temporal_ss.rows == m_measured_area.getHeight() + size_diff.second;
    
    return ok;
}


bool  voxel_processor::m_load(const std::vector<roiWindow<P8U>> &images,
                              uint32_t sample_x,uint32_t sample_y,
                              const std::vector<int>& indicies) {
    sample(sample_x, sample_y);
    m_voxel_length = indicies.empty() ? images.size() : indicies.size();
    int first_index = indicies.empty() ? 0 : indicies[0];
    image_size(images[first_index].width(), images[first_index].height());
    uint32_t expected_width = m_expected_segmented_size.first;
    uint32_t expected_height = m_expected_segmented_size.second;
    
    m_voxels.resize(0);
    std::string msg = " Generating Voxels @ (" +
    to_string(m_voxel_sample.first) + "," +
    to_string(m_voxel_sample.second) + ")";
    msg += "Expected Size: " + to_string(expected_width) + " x " +
    to_string(expected_height);
    vlogger::instance().console()->info("starting " + msg);

    // Walk through the sampled space and fetch the voxels
    int count = 0;
    for (int row = 0; row < expected_height; row++){
        for (int col = 0; col < expected_width; col++){
            int org_col = m_half_offset.first + col * m_voxel_sample.first;
            int org_row = m_half_offset.second + row * m_voxel_sample.second;
            std::vector<uint8_t> voxel(m_voxel_length);
            for (auto tt = 0; tt < m_voxel_length; tt++) {
                int idx = indicies.empty() ? tt : indicies[tt];
                if (! images[idx].contains(org_col, org_row)){
                    std::cout << org_col << "," << org_row << std::endl;
                }
                voxel[tt] = images[idx].getPixel(org_col, org_row);
            }
            count++;
            m_voxels.emplace_back(voxel);
        }
    }
    
    bool ok = count == (expected_width * expected_height);

    vlogger::instance().console()->info(ok ? "finished ok "
                                        : "finished with error ");
    return ok;
}

#pragma GCC diagnostic pop

