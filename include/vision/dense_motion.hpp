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
#include "core/vector2d.hpp"
#include "core/pair.hpp"
#include "core/core.hpp"
#include "localvariance.h"
#include "rowfunc.h"  // for correlation parts
#include <boost/range/irange.hpp>
#include "core/stl_utils.hpp"
#include <chrono>

using namespace stl_utils;

namespace svl
{
    using  cvMatRef = std::shared_ptr<cv::Mat>;
    
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
     

    class denseMotion
    {
    public:
        struct match{
            bool valid;
            float r;
            iPair peak;
            fVector_2d motion;
        };
        struct buffers {
            cvMatRef m_i;
            cvMatRef m_s;
            cvMatRef m_ss;
        };
        enum direction { forward = 0, backward = 1};
        
        denseMotion(const iPair& frame, const iPair& fixed_half_size, const iPair& moving_half_size, const fPair& = fPair(1.0f,1.0f));

        const iPair& fixed_size () const;
        const iPair& moving_size () const;
        
        void update(const cv::Mat& image);
        const std::vector<buffers>::const_iterator fixed_buffers () const;
        const std::vector<buffers>::const_iterator moving_buffers () const;

        // Assume moving is larger or equal to fixed.
        // return position of the best match of fixed in moving
        // Requires fixed rect at fixed location to be in the moving rect at moving location
        
        //  void block_match(const iRect& fixed, const iRect& moving, direction dir, match& result){
        // Calculate a range of point correlations to compute
        // from dir get which image to use
        //
        bool block_match(const iPair& fixed, const iPair& moving, match& result);
     
        // Point match both fixed size
        // Compute ncc at tl location row_0,col_0 in fixed and row_1,col_1 moving using integral images
        double point_ncc_match(const iPair& fixed, const iPair& moving, CorrelationParts& cp, direction dir = forward );
      
            
        static uint32_t ring_size () { return 2; }
        
        const double& elapsedTime () const { return m_total_elapsed; }
        const int& numberOfCalls () const { return m_number_of_calls; }
        
        
    private:
        void measure_space (const vector<vector<int>>& space, match& res){

            
            
        }
        bool allocate_buffers ();
        iPair m_msize, m_fsize, m_isize;
        std::vector<std::vector<cv::Mat>> m_data;
        std::vector<buffers> m_links;
        int m_number_of_calls;
        double m_total_elapsed;
        std::atomic<int64_t> m_count;
        uint32_t m_minus_index, m_plus_index;
        std::mutex m_mutex;
        std::vector<std::vector<int>> m_space;
        CorrelationParts cp;

  

    };
    
}


#endif /* dense_motion_hpp */
