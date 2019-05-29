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
        params ():m_voxel_sample(3,3), m_pad(5,5), m_min_segmentation_area(10), m_norm_sym_pad(0.0f,1.0f) {}
        const std::pair<uint32_t,uint32_t>& voxel_sample ()const {return m_voxel_sample; }
        const std::pair<uint32_t,uint32_t>& voxel_pad ()const {return m_pad; }
        const iPair& expected_segmented_size () const{return m_expected_segmented_size; }
        const uint32_t& min_segmentation_area ()const {return m_min_segmentation_area; }
        
        const fPair& normalized_symmetric_padding ()const { return m_norm_sym_pad; }
    private:
        std::pair<uint32_t,uint32_t> m_voxel_sample;
        std::pair<uint32_t,uint32_t> m_pad;
        iPair m_expected_segmented_size;
        uint32_t m_min_segmentation_area;
        fPair m_norm_sym_pad;
    };
    
private:


};

#endif /* voxel_segmentation_hpp */
