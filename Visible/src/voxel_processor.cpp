
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
#include "time_series/persistence1d.hpp"
#include "cvplot/cvplot.h"

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

const cv::Mat& scaleSpace::voxel_range() const { return m_voxel_range; }

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

const std::vector<cv::Rect>& scaleSpace::modeled_ends() const {
	return m_rects;
}

bool scaleSpace::detect_moving_profile(const cv::Mat& mof, const iPair& e_size, const iPair& margin){
	static iPair zerop (0,0);

	// Sanity check: margin and structuring element required to be less than quarter of the input image size
	iPair isize(mof.cols/4, mof.rows/4);
	assert(margin > zerop && e_size > zerop);
	assert(isize > margin && isize > e_size);
	
	
	cv::Mat eroded_input_image, thresholded_input_image, thresholded_input_8bit;
	
	cv::Mat element = cv::getStructuringElement(
		cv::MORPH_ELLIPSE, cv::Size(2 * e_size.first + 1, e_size.second * 3 + 1),
		cv::Point(e_size.first, e_size.second));
	
	cv::erode(mof, eroded_input_image, element);
	m_motion_field_minimas = mof.clone();
	cv::compare(mof, eroded_input_image, m_motion_field_minimas, cv::CMP_EQ);

	m_profile_threshold = cv::threshold(mof, thresholded_input_image, 0, 255,
							 cv::THRESH_BINARY_INV | cv::THRESH_OTSU );

	thresholded_input_image.convertTo(thresholded_input_8bit, CV_8U);
	cv::bitwise_and(m_motion_field_minimas, thresholded_input_8bit, m_motion_field_minimas);
	
	std::vector<Point2f> minimas;
	for (int j = margin.second; j < (m_motion_field_minimas.rows - margin.second); j++)
		for (int i = margin.first; i < (m_motion_field_minimas.cols - margin.first); i++){
		if (m_motion_field_minimas.at<uint8_t>(j,i) > 0)
			minimas.emplace_back(i,j);
		}
	if (minimas.size() < 5){
		std::cout << " Too Few Points for Profile Detection " << std::endl;
		return false;
	}
	
	m_body = fitEllipseDirect(minimas);
	m_ellipse = ellipseShape(m_body);

	
	return true;
}

/*
  This code needs to  be refactored to support cells at any angle.
*/

bool scaleSpace::process_motion_peaks(int model_frame_index, const iPair& e_size, const iPair& margin){
    if (! isLoaded() || ! spaceDone() || ! fieldDone() ) return false;
    
    m_all_rects.resize(0);
	m_segmented_ends.resize(2);
	auto ok = scaleSpace::detect_moving_profile(m_motion_field, e_size, margin);
	
	if (! ok) return ok;
	m_ellipse.wide_ends(m_segmented_ends);
	m_ellipse.focal_points(m_focals);
	m_ellipse.directrix_points(m_directrix);
	
	std::sort(m_segmented_ends.begin(), m_segmented_ends.end(), [](Point2f& a, Point2f&b){ return a.x < b.x; });
	
	m_rects.resize(2);
	iPair halfwidths;
	halfwidths.first = m_segmented_ends[0].x;
	halfwidths.second = m_motion_field.cols - m_segmented_ends[1].x;
	m_rects[0] = cv::Rect(cv::Point2i(m_segmented_ends[0].x,10), cv::Point2i(m_ellipse.x-30, m_motion_field.rows-10));
	m_rects[1] = cv::Rect(cv::Point2i(m_ellipse.x+30,10), cv::Point2i(m_segmented_ends[1].x+20, m_motion_field.rows-10));
	iPair origins (0, m_rects[1].size().width);
    
    auto model_frame = m_inputs[model_frame_index];
    for (auto rr : m_rects){
        m_models.emplace_back(model_frame, rr);
    }
    
    m_lengths.resize(0);
    m_ends.first.resize(0);
    m_ends.second.resize(0);
    

    for (const cv::Mat& img : m_inputs){
        
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
		
		points[0].x += origins.first;
		points[1].x += origins.second;
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
	m_inputs.resize(0);

    auto step_range = end_sigma - start_sigma;
    if (step <= 0 || step_range <= 0) return false;
    int steps = (end_sigma - start_sigma)/step;
    if (steps < 4) return false;

    // make a cv::mat clone of input images
    cv::Size isize(images[0].cols, images[0].rows);
	m_voxel_range = cv::Mat(isize.height, isize.width, CV_8U);
	cv::Mat imin = cv::Mat(isize.height, isize.width, CV_8U);
	cv::Mat imax = cv::Mat(isize.height, isize.width, CV_8U);
	imin = 255;
	imax = 0;
	
    for (const auto& rw : images){
        if(rw.cols != isize.width || rw.rows != isize.height) return false;
        auto clone = rw.clone();
        clone.convertTo(clone, CV_64F);
        m_filtered.push_back(clone);
		clone = rw.clone();
		m_inputs.push_back(clone);
		cv::min(m_inputs.back(), imin, imin);
		cv::max(m_inputs.back(), imax, imax);
    }
	auto min_max_d = imax - imin;
	cv::GaussianBlur(min_max_d, m_voxel_range, cv::Size(0,0), 4.0);
	cv::threshold(m_voxel_range, m_voxel_range, 0, 255, THRESH_BINARY | THRESH_OTSU);
	
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
                    std::cout << " Voxel " << org_col << "," << org_row << std::endl;
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

