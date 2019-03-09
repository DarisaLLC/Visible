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
     

    class dmf
    {
    public:
        struct match{
            float r;
            uiPair peak;
            fVector_2d motion;
        };
        struct buffers {
            cvMatRef m_i;
            cvMatRef m_s;
            cvMatRef m_ss;
        };
        enum direction { forward = 0, backward = 1};
        
        dmf(const uiPair& frame, const uiPair& fixed_half_size, const uiPair& moving_half_size, const fPair& = fPair(1.0f,1.0f));

        const uiPair& fixed_size () const;
        const uiPair& moving_size () const;
        
        void update(const cv::Mat& image);
        const std::vector<buffers>::const_iterator fixed_buffers () const;
        const std::vector<buffers>::const_iterator moving_buffers () const;

        // For fixed and moving sizes and centered at location, compute
        // forward and backward motions and validate them
        // forward is prev to cur
        // backward is cur to prev
       // void block_bi_motion(const uiPair& location);
        
        // Assume moving is larger or equal to fixed.
        // return position of the best match
        // Requires fixed rect at fixed location to be in the moving rect at moving location
        
        void block_match(const uiPair& fixed, const uiPair& moving, direction dir, match& result);
        
      //  void block_match(const iRect& fixed, const iRect& moving, direction dir, match& result){
            // Calculate a range of point correlations to compute
            // from dir get which image to use
            //}
        // Point match
        // Compute ncc at tl location row_0,col_0 in fixed and row_1,col_1 moving using integral images
        double normalizedCorrelation(const uiPair& fixed, const uiPair& moving, CorrelationParts& cp, direction dir = forward );
      
            
        static uint32_t ring_size () { return 2; }
            
    private:
        bool allocate_buffers ();
        uiPair m_msize, m_fsize, m_isize;
        std::vector<std::vector<cv::Mat>> m_data;
        std::vector<buffers> m_links;
        
        std::atomic<int64_t> m_count;
        uint32_t m_minus_index, m_plus_index;
        std::mutex m_mutex;


  

    };
    
}


#endif /* dense_motion_hpp */
