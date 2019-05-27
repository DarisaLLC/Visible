//
//  voxel_segmentation.hpp
//  Visible
//
//  Created by Arman Garakani on 5/26/19.
//

#ifndef voxel_segmentation_hpp
#define voxel_segmentation_hpp

#include "core/pair.hpp"

class voxelSegmenter{
public:
    class params{
    public:
        params ():m_voxel_sample(3,3), m_pad(5,5), m_min_segmentation_area(10) {}
        const std::pair<uint32_t,uint32_t>& voxel_sample () {return m_voxel_sample; }
        const std::pair<uint32_t,uint32_t>& voxel_pad () {return m_pad; }
        const iPair& expected_segmented_size () {return m_expected_segmented_size; }
        const uint32_t& min_segmentation_area () {return m_min_segmentation_area; }
    private:
        std::pair<uint32_t,uint32_t> m_voxel_sample;
        std::pair<uint32_t,uint32_t> m_pad;
        iPair m_expected_segmented_size;
        uint32_t m_min_segmentation_area;
    };
    
private:


};

#endif /* voxel_segmentation_hpp */
