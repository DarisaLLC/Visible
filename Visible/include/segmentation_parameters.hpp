//
//  segmentation_parameters.hpp
//  Visible
//
//  Created by Arman Garakani on 5/26/19.
//

#ifndef segmentation_parameters_hpp
#define segmentation_parameters_hpp

#include "core/pair.hpp"

class fynSegmenter{
public:
    class params{
    public:
        params ():m_voxel_sample(3,3), m_min_segmentation_area(1000),
            m_half(m_voxel_sample.first / 2,m_voxel_sample.second / 2) {}
        
        const std::pair<uint32_t,uint32_t>& voxel_sample ()const {return m_voxel_sample; }
        const std::pair<uint32_t,uint32_t>& voxel_sample_half ()const { return m_half; }
        const iPair& expected_segmented_size () const{return m_expected_segmented_size; }
        const uint32_t& min_segmentation_area ()const {return m_min_segmentation_area; }
        

    private:
        std::pair<uint32_t,uint32_t> m_voxel_sample;
        std::pair<uint32_t,uint32_t> m_half;
        iPair m_expected_segmented_size;
        uint32_t m_min_segmentation_area;
    };
    
private:


};

#endif /* voxel_segmentation_hpp */
