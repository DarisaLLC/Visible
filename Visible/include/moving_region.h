//
//  moving_region.h
//  Visible
//
//  Created by Arman Garakani on 8/10/19.
//

#ifndef moving_region_h
#define moving_region_h
#include "timed_value_containers.h"
#include <vector>
#include "cinder/Rect.h"
#include "core/pair.hpp"
#include "vision/labelBlob.hpp"


typedef std::pair<vec2,vec2> sides_length_t;


class moving_region : public labelBlob::blob {
public:
    moving_region (const labelBlob::blob&, cell_id_t ind);

    const cv::RotatedRect& motion_surface () const { return m_rr; };
    const cv::Mat&  surfaceAffine() const { return m_surface_affine; };
    const fPair& ellipse_ab () const { return m_ab;};
    const fPair& length_range () const;
    const std::vector<sides_length_t>& cell_ends() const;
    const std::vector<float>& cell_lengths () const { return m_cell_lengths; }
    const cell_id_t id () const { return m_id;}
private:
    cv::Mat m_surface_affine;
    cv::RotatedRect m_rr;
    std::vector<float> m_cell_lengths;
    ci::Rectf m_measure_area;
    std::vector<sides_length_t> m_cell_ends = {sides_length_t (), sides_length_t()};
    fPair m_ab;
    cell_id_t m_id;
};


// Move me
void pointsToRotatedRect (std::vector<cv::Point2f>& imagePoints, cv::RotatedRect& rotated_rect );


#endif /* moving_region_h */
