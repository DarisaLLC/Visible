//
//  dense_motion.cpp
//  VisibleGtest
//
//  Created by arman on 3/2/19.
//
#include <iterator>
#include "core/rectangle.h"
#include "logger/logger.hpp"
#include "dense_motion.hpp"
using namespace svl;


denseMotion::denseMotion(const iPair& frame, const iPair& fixed_half_size, const iPair& moving_half_size, const fPair& phase ):
   m_isize(frame), m_fsize(fixed_half_size * 2 + 1), m_msize(moving_half_size * 2 + 1)
{
    std::string msg = std::to_string(frame.first);
    msg = "Frame: " + msg + " x " + std::to_string(frame.second);
    vlogger::instance().console()->info(msg);
    assert(moving_half_size > fixed_half_size);
    
    allocate_buffers();
}



const iPair& denseMotion::fixed_size () const { return m_fsize; }
const iPair& denseMotion::moving_size () const { return m_msize; }

const std::vector<svl::denseMotion::buffers>::const_iterator denseMotion::fixed_buffers () const{
    auto bi = m_links.begin();
    std::advance(bi,m_minus_index);
    return bi;
}
const std::vector<svl::denseMotion::buffers>::const_iterator denseMotion::moving_buffers () const{
    auto bi = m_links.begin();
    std::advance(bi,m_plus_index);
    return bi;
}

void denseMotion::update(const cv::Mat& image){
    std::lock_guard<std::mutex> lock( m_mutex );
    
    m_data[1][0].copyTo(m_data[0][0]);
    m_data[1][1].copyTo(m_data[0][1]);
    m_data[1][2].copyTo(m_data[0][2]);
    image.copyTo(m_data[1][0]);
    cv::integral (m_data[1][0], m_data[1][1], m_data[1][2]);
    m_count += 1;
    m_minus_index = (m_minus_index + 1)%ring_size();
    m_plus_index = (m_minus_index + 1)%ring_size();
}



bool denseMotion::allocate_buffers ()
{
    m_data.resize(2);
    m_links.resize(2);
    
    m_data[0].resize(3);
    m_data[1].resize(3);
    // Allocate intergral image: 1 bigger in each dimension
    cv::Size iSize(m_isize.first,m_isize.second);
    cv::Size plusOne(m_isize.first+1,m_isize.second+1);
    
    m_data[0][0] = cv::Mat(iSize,CV_8UC1);
    m_data[0][1] = cv::Mat(plusOne, CV_32SC1);
    m_data[0][2] = cv::Mat(plusOne, CV_64FC1);
    
    m_data[1][0] = cv::Mat(iSize,CV_8UC1);
    m_data[1][1] = cv::Mat(plusOne, CV_32SC1);
    m_data[1][2] = cv::Mat(plusOne, CV_64FC1);
    
    m_links[0].m_i = cvMatRef(&m_data[0][0], stl_utils::null_deleter());
    m_links[0].m_s = cvMatRef(&m_data[0][1], stl_utils::null_deleter());
    m_links[0].m_ss = cvMatRef(&m_data[0][2],stl_utils::null_deleter());
    
    m_links[1].m_i = cvMatRef(&m_data[1][0], stl_utils::null_deleter());
    m_links[1].m_s = cvMatRef(&m_data[1][1], stl_utils::null_deleter());
    m_links[1].m_ss = cvMatRef(&m_data[1][2], stl_utils::null_deleter());
    
    m_count = 0;
    m_minus_index = 1;
    return true;
}
//
//void dmf::block_match(const iPair& fixed, const iPair& moving, direction dir, match& result){
//    fRect ff(fixed.first,fixed.second, fixed_size().first, fixed_size().second);
//    fRect mm(moving.first,moving.second, moving_size().first, moving_size().second);
//
//    assert(mm.contains(ff));
//
//}

bool denseMotion::block_match(const iPair& fixed, const iPair& moving,  match& result){
    iRect mr (moving.first, moving.second, moving_size().first, moving_size().second);
    iRect fr (fixed.first, fixed.second, fixed_size().first, fixed_size().second);
    iRect cr (0,0, moving_size().first - fixed_size().first + 1, moving_size().second - fixed_size().second + 1);
    m_space.clear();
    
    iPair max_loc(-1,-1);
    int max_score = 0;
    m_total_elapsed = 0;
    m_number_of_calls = 0;
    for (int j = 0; j < cr.height(); j++){ // rows
        std::vector<int> rs;
        for (int i = 0; i < cr.width(); i++){ // cols
            const auto start = std::chrono::high_resolution_clock::now();
            iPair tmp(moving.first+j, moving.second+i);
            auto r = point_ncc_match(fixed, tmp,cp);
            const auto end = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
            m_total_elapsed += duration.count();
            m_number_of_calls += 1;
            
            int score = int(r*1000);
            if (score > max_score){
                max_loc.first = j;
                max_loc.second = i;
                max_score = score;
            }
            rs.push_back(score);
        }
        m_space.push_back(rs);
    }
    bool valid = max_loc.first >= 0 && max_loc.second >= 0 && max_loc.first < cr.height() && max_loc.second < cr.width();
    valid = valid && m_space[max_loc.first][max_loc.second] == max_score;
    result.valid = valid;
    result.peak = max_loc;
    result.motion = fVector_2d ();
    if (max_loc.first > 0 && max_loc.first < cr.width()-1){
        // Cheap Parabolic Interpolation
        auto x = ((m_space[max_loc.second][max_loc.first+1]-m_space[max_loc.second][max_loc.first-1])/2.0) /
        (m_space[max_loc.second][max_loc.first+1]+m_space[max_loc.second][max_loc.first-1]-m_space[max_loc.second][max_loc.first]);
        result.motion.x(x);
    }
    if (max_loc.second > 0 && max_loc.second < cr.height()-1){
        auto y = ((m_space[max_loc.second+1][max_loc.first]-m_space[max_loc.second-1][max_loc.first])/2.0) /
        (m_space[max_loc.second-1][max_loc.first]+m_space[max_loc.second+1][max_loc.first]-m_space[max_loc.second][max_loc.first]);
        result.motion.y(y);
    }
    return valid;
}

// For fixed and moving sizes and centered at location, compute
// forward and backward motions and validate them
// forward is prev to cur
// backward is cur to prev
// void block_bi_motion(const iPair& location);

// Assume moving is larger or equal to fixed.
// return position of the best match
//  void block_match(const iRect& fixed, const iRect& moving, direction dir, match& result){
// Calculate a range of point correlations to compute
// from dir get which image to use
//}


// Point match
// Compute ncc at tl location row_0,col_0 in fixed and row_1,col_1 moving using integral images
double denseMotion::point_ncc_match(const iPair& fixed, const iPair& moving, CorrelationParts& cp, direction dir)
{
    const uint32_t& col_0 = fixed.first;
    const uint32_t& row_0 = fixed.second;
    const uint32_t& col_1 = moving.first;
    const uint32_t& row_1 = moving.second;
    
    auto f_link = dir == forward ? fixed_buffers() : moving_buffers();
    auto m_link = dir == forward ? moving_buffers() : fixed_buffers();
    
    cp.clear();
    double ab = 0.0;
    const unsigned int height_tpl = fixed_size().second, width_tpl = fixed_size().first;
    const int nP = height_tpl * width_tpl;
    if (true) {
        for (unsigned int i = 0; i <  height_tpl; i++) {
            for (unsigned int j = 0; j < width_tpl; j++) {
                ab += (f_link->m_i->ptr(row_0 + i)[col_0 + j]) * m_link->m_i->ptr(row_1 + i)[col_1 + j];
                
            }
        }
        /*  row,col
         (row_0)[col_0]             ---->          (row_0)[col_0 + width_tpl]
         
         
         (row_0 + height_tpl)[col_0] ---->          (row_0 + height_tpl)[col_0 + width_tpl]
         */
        auto lsum_int32 = [](const cvMatRef m, uint32_t row, uint32_t col, uint32_t rows, uint32_t cols){
            iPair tl_(row,col);
            iPair tr_(row,col + cols);
            iPair bl_(row + rows,col);
            iPair br_(row + rows,col + cols);
            
            return   ((const int*)m->ptr(br_.first))[br_.second] +
            ((const int*)m->ptr(tl_.first))[tl_.second] -
            ((const int*)m->ptr(tr_.first))[tr_.second] -
            ((const int*)m->ptr(bl_.first))[bl_.second];
        };
        auto lsum_double = [](const cvMatRef m, uint32_t row, uint32_t col, uint32_t rows, uint32_t cols){
            iPair tl_(row,col);
            iPair tr_(row,col + cols);
            iPair bl_(row + rows,col);
            iPair br_(row + rows,col + cols);
            
            return   ((const double*)m->ptr(br_.first))[br_.second] +
            ((const double*)m->ptr(tl_.first))[tl_.second] -
            ((const double*)m->ptr(tr_.first))[tr_.second] -
            ((const double*)m->ptr(bl_.first))[bl_.second];
        };
        
        
        double sum1 = lsum_int32(f_link->m_s, row_0, col_0, height_tpl, width_tpl);
        double sum2 = lsum_int32(m_link->m_s, row_1, col_1, height_tpl, width_tpl);
        double a2 = lsum_double(f_link->m_ss, row_0, col_0, height_tpl, width_tpl);
        double b2 = lsum_double(m_link->m_ss, row_1, col_1, height_tpl, width_tpl);
        cp.n(nP);
        /*
         mSim += sim; mSii += sii; mSmm += smm; mSi += si;mSm += sm;
         */
        cp.accumulate(ab, a2, b2, sum1, sum2);
        cp.compute();
        
        return cp.r();
        
    }
}

