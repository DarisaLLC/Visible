//
//  dense_motion.hpp
//  VisibleGtest
//
//  Created by arman on 3/2/19.
//

#ifndef dense_motion_hpp
#define dense_motion_hpp

#include <atomic>
#include "opencv2/opencv.hpp"
#include "core/pair.hpp"
#include "core/core.hpp"
#include "localvariance.h"
#include "rowfunc.h"  // for correlation parts

namespace svl
{
    
    /*!
     Dense Motion Field using Integral Image
     first,second correspond to fixed,movin
     */
    /* Moving ( contains fixed )        */
    /* +--------------+---------------+ */
    /* |              |               | */
    /* |              |               | */
    /* |       Fixed  |               | */
    /* |      *=======|======*        | */
    /* |      !       |      !        | */
    /* |      !       |      !        | */
    /* |      !       |      !        | */
    /* |      !       |      !        | */
    /* |      *=======|======*        | */
    /* |              |               | */
    /* |              |               | */
    /* |              |               | */
    /* +--------------+---------------+ */
     

    class dmf
    {
    public:
        dmf(const uiPair& frame, const uiPair& fixed_half_size,
                     const uiPair moving_half_size, const fPair = fPair(1.0f,1.0f)):
        m_isize(frame), m_fsize(fixed_half_size * 2 + 1),
        m_msize(moving_half_size * 2 + 1)
        {
            
        }

        void update(const cv::Mat& image){
            std::lock_guard<std::mutex> lock( m_mutex );
            
            m_f_m.second.copyTo(m_f_m.first);
            m_s_fm.second.copyTo(m_s_fm.first);
            m_ss_fm.second.copyTo(m_ss_fm.first);
            image.copyTo(m_f_m.second);
            cv::integral (m_f_m.second, m_s_fm.second, m_ss_fm.second);
            m_count += 1;
        }

        const uiPair fixed () const { return m_fsize; }
        const uiPair moving () const { return m_msize; }

        
        // Compute ncc at tl location row_0,col_0 in fixed and row_1,col_1 moving using integral images
        double normalizedCorrelation(const uiPair& fixed, const uiPair& moving)
        {
            const uint32_t& col_0 = fixed.first;
            const uint32_t& row_0 = fixed.second;
            const uint32_t& col_1 = moving.first;
            const uint32_t& row_1 = moving.second;
            
            CorrelationParts cp;
            double ab = 0.0;
            const unsigned int height_tpl = m_msize.second, width_tpl = m_msize.first;
            const int nP = height_tpl * width_tpl;
            if (true) {
                for (unsigned int i = 0; i < m_msize.second; i++) {
                    for (unsigned int j = 0; j < m_msize.first; j++) {
                        ab += (m_f_m.first.ptr(row_0 + i)[col_0 + j]) * m_f_m.second.ptr(row_1 + i)[col_1 + j];
                    }
                }
            }
            
            
            /*  row,col
             (row_0)[col_0]             ---->          (row_0)[col_0 + width_tpl]
             
             
             (row_0 + height_tpl)[col_0] ---->          (row_0 + height_tpl)[col_0 + width_tpl]
             */
            auto lsum_int32 = [](const cv::Mat& m, uint32_t row, uint32_t col, uint32_t rows, uint32_t cols){
                iPair tl_(row,col);
                iPair tr_(row,col + cols);
                iPair bl_(row + rows,col);
                iPair br_(row + rows,col + cols);
                
                return   ((const int*)m.ptr(br_.first))[br_.second] +
                ((const int*)m.ptr(tl_.first))[tl_.second] -
                ((const int*)m.ptr(tr_.first))[tr_.second] -
                ((const int*)m.ptr(bl_.first))[bl_.second];
            };
            auto lsum_double = [](const cv::Mat& m, uint32_t row, uint32_t col, uint32_t rows, uint32_t cols){
                iPair tl_(row,col);
                iPair tr_(row,col + cols);
                iPair bl_(row + rows,col);
                iPair br_(row + rows,col + cols);
                
                return   ((const double*)m.ptr(br_.first))[br_.second] +
                ((const double*)m.ptr(tl_.first))[tl_.second] -
                ((const double*)m.ptr(tr_.first))[tr_.second] -
                ((const double*)m.ptr(bl_.first))[bl_.second];
            };
            
         
            double sum1 = lsum_int32(m_s_fm.first, row_0, col_0, height_tpl, width_tpl);
            double sum2 = lsum_int32(m_s_fm.second, row_1, col_1, height_tpl, width_tpl);
            double a2 = lsum_double( m_ss_fm.first, row_0, col_0, height_tpl, width_tpl);
            double b2 = lsum_double( m_ss_fm.second, row_1, col_1, height_tpl, width_tpl);
            cp.n(nP);
            /*
             mSim += sim;
             mSii += sii;
             mSmm += smm;
             mSi += si;
             mSm += sm;
             */
            cp.accumulate(ab, a2, b2, sum1, sum2);
            cp.compute();
            
            return cp.r();
            
        }
    private:
        mutable uiPair m_msize, m_fsize, m_isize;
        std::pair<cv::Mat,cv::Mat> m_s_fm;
        std::pair<cv::Mat,cv::Mat> m_ss_fm;
        std::pair<cv::Mat,cv::Mat> m_f_m;
        std::atomic<int64_t> m_count;
        std::mutex m_mutex;
        
        bool allocate_buffers ()
        {
            // Allocate intergral image: 1 bigger in each dimension
            cv::Size iSize(m_isize.first,m_isize.second);
            cv::Size plusOne(m_isize.first+1,m_isize.second+1);
            m_ss_fm.first = cv::Mat(plusOne, CV_64FC1);
            m_ss_fm.second = cv::Mat(plusOne, CV_64FC1);
            m_s_fm.first = cv::Mat(plusOne, CV_32SC1);
            m_s_fm.second = cv::Mat(plusOne, CV_32SC1);
            m_f_m.first = cv::Mat(iSize,CV_8UC1);
            m_f_m.second = cv::Mat(iSize,CV_8UC1);
            m_count = 0;
            return true;
        }

  

    };
    
}


#endif /* dense_motion_hpp */
