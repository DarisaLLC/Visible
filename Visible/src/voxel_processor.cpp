
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"

#include <mutex>
#include "core/pair.hpp"
#include "logger/logger.hpp"
#include "sm_producer.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "cinder_xchg.hpp"  // For Rectf @todo remove dependency on cinder
#include <future>
#include <memory>
#include "algo_runners.hpp"
#include "nms.hpp"
#include "core/stl_utils.hpp"
#include "core/fit.hpp"

using namespace svl;
using namespace stl_utils;



bool scaleSpace::generate(const std::vector<roiWindow<P8U>> &images, float start_sigma, float end_sigma, float step){
    std::vector<cv::Mat> mats;
    for (auto& rw : images){
        cvMatRefroiP8U(rw,cvmat,CV_8U);
        mats.push_back(cvmat.clone());
    }
    return generate(mats, start_sigma, end_sigma, step);
}

const cv::Mat& scaleSpace::motion_field() const {
    if (fieldDone()) return m_motion_field;
    
    m_motion_field = cv::Mat(space()[0].rows, space()[0].cols, CV_8U);
    m_motion_field = 255;
    // Calculate Difference of Gaussians
    m_dogs.resize(0);
    for (auto ss = 1; ss < m_scale_space.size() - 1; ss++){
        const cv::Mat& current = m_scale_space[ss];
        const cv::Mat& prev = m_scale_space[ss-1];
        cv::Mat dog = cv::Mat(space()[0].rows, space()[0].cols, CV_64F);
        cv::Mat dog_8 = cv::Mat(space()[0].rows, space()[0].cols, CV_8U);
        dog = current - prev;
        cv::normalize(dog,dog_8,0,255, NORM_MINMAX, CV_8U);
        cv::min(dog_8, m_motion_field, m_motion_field);
        m_dogs.push_back(dog_8);
    }
    
    m_field_done = true;
    return m_motion_field;
}
    
const std::vector<cv::Mat>& scaleSpace::dog() const {
    motion_field();
    return m_dogs;
}

void scaleSpace::detect_profile_extremas(const cv::Mat&, fPair& horizontal_ends){
	
	
	
	
	
	
	
	
}
void scaleSpace::detect_extremas(const cv::Mat& space, std::vector<cv::Rect>& output, const int threshold, const iPair& trim, bool detect_valleys){
        // Make sure it is empty
    std::vector<cv::Rect> peaks;
    
    std::vector<float> scores;
    peaks.resize(0);
    iPair rsize(5);
    int height = space.rows - 3 - trim.second;
    int width = space.cols - 3 - trim.first;
    int peak_threshold = 255 - threshold;
    int valley_threshold = threshold;
    
    if (! detect_valleys){
    for (int row = 3 + trim.second; row < height; row++){
        for (int col = 3 + trim.first; col < width; col++){
            uint8_t pel = space.at<uint8_t>(row,col);
            if (pel >= peak_threshold &&
                pel >= space.at<uint8_t>(row, col-1) &&
                pel >= space.at<uint8_t>(row, col+1) &&
                pel >= space.at<uint8_t>(row-1,col-1) &&
                pel >= space.at<uint8_t>(row-1, col+1) &&
                pel >= space.at<uint8_t>(row+1,col-1) &&
                pel >= space.at<uint8_t>(row+1, col+1) &&
                pel >= space.at<uint8_t>(row-1, col) &&
                pel >= space.at<uint8_t>(row+1, col)){
                    peaks.emplace_back(col-trim.first,row-trim.second,rsize.first,rsize.second );
                    scores.emplace_back(pel/255.);
                }
            }
        }
    }else{
        for (int row = 3 + trim.second; row < height; row++){
            for (int col = 3 + trim.first; col < width; col++){
                uint8_t pel = space.at<uint8_t>(row,col);
                if (pel < valley_threshold &&
                    pel <= space.at<uint8_t>(row, col-1) &&
                    pel <= space.at<uint8_t>(row, col+1) &&
                    pel <= space.at<uint8_t>(row-1,col-1) &&
                    pel <= space.at<uint8_t>(row-1, col+1) &&
                    pel <= space.at<uint8_t>(row+1,col-1) &&
                    pel <= space.at<uint8_t>(row+1, col+1) &&
                    pel <= space.at<uint8_t>(row-1, col) &&
                    pel <= space.at<uint8_t>(row+1, col)){
                    peaks.emplace_back(col-trim.first,row-trim.second,rsize.first,rsize.second );
                    scores.emplace_back(1.0 - pel/255.);
                }
            }
        }
    }
    
    /*
     void nms2(const std::vector<cv::Rect>& srcRects,
     const std::vector<float>& scores,
     std::vector<cv::Rect>& resRects,
     float thresh,
     int neighbors,
     float minScoresSum)
    */
    nms2(peaks, scores, output, 0.5, 2, 0.5);
    output = peaks;
//    auto sorted_indicies = stl_utils::sort_indexes(scores, detect_valleys);
//    auto take = std::min(size_t(8), sorted_indicies.size());
//    for (auto ii = 0; ii < take; ii++)
//        output.push_back(peaks[sorted_indicies[ii]]);
    
    }

bool scaleSpace::process_motion_peaks(int model_frame_index){
    if (! isLoaded() || ! spaceDone() || ! fieldDone() ) return false;
    
    m_all_rects.resize(0);
    scaleSpace::detect_extremas(m_motion_field, m_all_rects, 10, m_trim);
    std::sort(m_all_rects.begin(), m_all_rects.end(), [](cv::Rect& a, cv::Rect&b){ return a.tl().x > b.tl().x; });
    m_rects.resize(0);
    m_rects.push_back(m_all_rects[0]);
    m_rects.push_back(m_all_rects[m_all_rects.size()-1]);
    
    auto model_frame = m_scale_space[model_frame_index];
    for (auto rr : m_rects){
        m_models.emplace_back(model_frame, rr);
    }
    
    m_lengths.resize(0);
    m_ends.first.resize(0);
    m_ends.second.resize(0);
    

    for (const cv::Mat& img : m_scale_space){
        
        std::vector<cv::Point2f> points;
        std::vector<float> scores;

        // Run over Two Models, record template matches
        for (const cv::Mat& templ : m_models){
            Mat result;
            int result_cols =  img.cols - templ.cols + 1;
            int result_rows = img.rows - templ.rows + 1;
            result.create( result_rows, result_cols, CV_32FC1 );
            matchTemplate( img, templ, result, TM_CCORR_NORMED);
            normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );
            double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
            minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );
            cv::Point matchLoc = maxLoc;
            float par_x, par_y;
            par_x = par_y = 0;
            parabolicFit<float,float>(result.at<float>(matchLoc.y,matchLoc.x-1), result.at<float>(matchLoc.y,matchLoc.x),result.at<float>(matchLoc.y,matchLoc.x+1),&par_x);
            parabolicFit<float,float>(result.at<float>(matchLoc.y-1,matchLoc.x), result.at<float>(matchLoc.y,matchLoc.x),result.at<float>(matchLoc.y+1,matchLoc.x),&par_y);
            points.emplace_back(matchLoc.x+par_x,matchLoc.y+par_y);
            scores.push_back(maxVal);
        }
        std::sort(points.begin(), points.end(), [](Point2f& a, Point2f&b){ return a.x > b.x; });
        auto length = std::sqrt(squareOf(points[points.size()-1].x - points[0].x) + squareOf(points[points.size()-1].y - points[0].y));
        m_lengths.push_back(length);
    }
    return true;
}

bool scaleSpace::length_extremes(fPair& extremes) const {
    if (m_scale_space.empty() || m_lengths.empty() || m_scale_space.size() != m_lengths.size())
        return false;
    
    vector<float>::iterator min_length_iter = std::min_element(m_lengths.begin(), m_lengths.end());
    vector<float>::iterator max_length_iter = std::max_element(m_lengths.begin(), m_lengths.end());
    extremes.first = *min_length_iter;
    extremes.second = *max_length_iter;
    m_length_extremes = extremes;
    return (extremes.second - extremes.first) > 1.0f;
    
}
bool scaleSpace::generate(const std::vector<cv::Mat>& images,
                          float start_sigma, float end_sigma, float step){
    
    // check start, end and step are positive and end-start is multiple of step
    m_filtered.resize(0);
    m_scale_space.resize(0);
    
    auto step_range = end_sigma - start_sigma;
    if (step <= 0 || step_range <= 0) return false;
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
    for (auto scale = start_sigma; scale < end_sigma; scale+=step){
        cv::Mat sum = cv::Mat(isize.height, isize.width, CV_64F);
        cv::Mat sumsq = cv::Mat(isize.height, isize.width, CV_64F);
        sum = 0;
        sumsq = 0;
        for (auto ii = 0; ii < m_filtered.size(); ii++){
            auto mm = m_filtered[ii].clone();
            cv::GaussianBlur(mm, mm, cv::Size(0,0), scale );
            // Sum
            sum += mm;
            // square at Sum Squared
            mm = mm.mul(mm);
            sumsq += mm;
        }
        // n SS - S * S
        cv::multiply(sum, sum, sum);
        sumsq *= n;
        cv::subtract(sumsq, sum, sumsq);
        // n * ( n - 1 )
        sumsq /= n2;
        // Produce variance. Since we will be summing the square at the next step
        m_scale_space.push_back(sumsq);
    }
    auto scale_steps = (end_sigma - start_sigma + 1.0 ) / step;
    assert(m_scale_space.size() == scale_steps);
    m_space_done = true;
    return m_space_done;
    
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

