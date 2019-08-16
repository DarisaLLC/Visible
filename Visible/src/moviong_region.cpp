//
//  moving_region.cpp
//  Visible
//
//  Created by Arman Garakani on 8/10/19.
//

#include <stdio.h>
#include "core/lineseg.hpp"
#include "vision/opencv_utils.hpp"
#include "cinder_cv/cinder_opencv.h"
#include "moving_region.h"

using labelBlob=svl::labelBlob;

moving_region::moving_region (const labelBlob::blob& bb, uint32_t ind): labelBlob::blob (bb), m_id(ind){
    m_rr = bb.rotated_roi();
    std::vector<cv::Point2f> corners(4);
    //     std::string msg = svl::to_string(m_rr);
    //      msg = " < " + msg;
    //      vlogger::instance().console()->info(msg);
    m_rr.points(corners.data());
    pointsToRotatedRect(corners, m_rr);
//    m_rr.center.x += pad_trans.first;
//    m_rr.center.y += pad_trans.second;
    //   msg = svl::to_string(m_rr);
    //   msg = " > " + msg;
    //   vlogger::instance().console()->info(msg);
    std::vector<cv::Point2f> mid_points;
    svl::get_mid_points(m_rr, mid_points);
    auto surface_affine = cv::getRotationMatrix2D(m_rr.center,m_rr.angle, 1);
    surface_affine.at<double>(0,2) -= (m_rr.center.x - m_rr.size.width/2);
    surface_affine.at<double>(1,2) -= (m_rr.center.y - m_rr.size.height/2);
 
    auto two_pt_dist = [mid_points](int a, int b){
        const cv::Point2f& pt1 = mid_points[a];
        const cv::Point2f& pt2 = mid_points[b];
        return sqrt(pow(pt1.x-pt2.x,2)+pow(pt1.y-pt2.y,2));
    };
    
    if (mid_points.size() == 4){
        
        float dists[2];
        float d = two_pt_dist(0,2);
        dists[0] = d;
        d = two_pt_dist(1,3);
        dists[1] = d;
        if (dists[0] >= dists[1]){
            m_cell_ends[0].first = vec2(mid_points[0].x,mid_points[0].y);
            m_cell_ends[0].second = vec2(mid_points[2].x,mid_points[2].y);
            m_cell_ends[1].first = vec2(mid_points[1].x,mid_points[1].y);
            m_cell_ends[1].second = vec2(mid_points[3].x,mid_points[3].y);
        }
        else{
            m_cell_ends[1].first = vec2(mid_points[0].x,mid_points[0].y);
            m_cell_ends[1].second = vec2(mid_points[2].x,mid_points[2].y);
            m_cell_ends[0].first = vec2(mid_points[1].x,mid_points[1].y);
            m_cell_ends[0].second = vec2(mid_points[3].x,mid_points[3].y);
        }
    }
    m_ab = bb.moments().getEllipseAspect ();

}


void pointsToRotatedRect (std::vector<cv::Point2f>& imagePoints, cv::RotatedRect& rotated_rect ){
    // get the left most
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.x > b.x;
    });
    
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.y < b.y;
    });
    
    auto cwOrder = [&] (std::vector<cv::Point2f>& cc, std::vector<std::pair<std::pair<int,int>,float>>& results) {
        const float d01 = glm::distance( fromOcv(cc[0]), fromOcv(cc[1]) );
        const float d02 = glm::distance( fromOcv(cc[0]), fromOcv(cc[2]) );
        const float d03 = glm::distance( fromOcv(cc[0]), fromOcv(cc[3]) );
        
        std::vector<std::pair<std::pair<int,int>,float>> ds = {{{0,1},d01}, {{0,2},d02}, {{0,3},d03}};
        std::sort (ds.begin(), ds.end(),[](std::pair<std::pair<int,int>,float>&a, const std::pair<std::pair<int,int>,float>&b){
            return a.second > b.second;
        });
        results = ds;
        auto print = [](std::pair<std::pair<int,int>,float>& vv) { std::cout << "::" << vv.second; };
        std::for_each(ds.begin(), ds.end(), print);
        std::cout << '\n';
    };
    
    std::vector<std::pair<std::pair<int,int>,float>> ranked_corners;
    cwOrder(imagePoints, ranked_corners);
    assert(ranked_corners.size()==3);
    fVector_2d tl (imagePoints[ranked_corners[1].first.first].x, imagePoints[ranked_corners[1].first.first].y);
    fVector_2d tr (imagePoints[ranked_corners[1].first.second].x, imagePoints[ranked_corners[1].first.second].y);
    fVector_2d bl (imagePoints[ranked_corners[2].first.second].x, imagePoints[ranked_corners[2].first.second].y);
    fLineSegment2d sideOne (tl, tr);
    fLineSegment2d sideTwo (tl, bl);
    
    
    cv::Point2f ctr(0,0);
    for (auto const& p : imagePoints){
        ctr.x += p.x;
        ctr.y += p.y;
    }
    ctr.x /= 4;
    ctr.y /= 4;
    rotated_rect.center = ctr;
    float dLR = sideOne.length();
    float dTB = sideTwo.length();
    rotated_rect.size.width = dTB;
    rotated_rect.size.height = dLR;
    std::cout << toDegrees(sideOne.angle().Double()) << " Side One " << sideOne.length() << std::endl;
    std::cout << toDegrees(sideTwo.angle().Double()) << " Side Two " << sideTwo.length() << std::endl;
    
    uDegree angle = sideTwo.angle();
    if (angle.Double() < -45.0){
        angle = angle + uDegree::piOverTwo();
        std::swap(rotated_rect.size.width,rotated_rect.size.height);
    }
    rotated_rect.angle = angle.Double();
}
