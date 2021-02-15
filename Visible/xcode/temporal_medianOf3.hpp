//
//  temporal_medianOf3.hpp
//  Visible
//
//  Created by Arman Garakani on 2/13/21.
//

#ifndef temporal_medianOf3_h
#define temporal_medianOf3_h

#include "opencv2/opencv.hpp"

/*
 Based on this implementation of median of 3
 #define median_of_3(a, b, c)                                       \
 ((a) > (b) ? ((a) < (c) ? (a) : ((b) < (c) ? (c) : (b))) :    \
 ((a) > (c) ? (a) : ((b) < (c) ? (b) : (c))))
*/

inline bool temporal_medianOf3 (const cv::Mat& _a, const cv::Mat& _b, const cv::Mat& _c, cv::Mat& dst){

    
    bool dcheck = _a.rows != _b.rows && _a.cols != _b.cols && _b.rows != _c.rows && _b.cols != _c.cols;
    if (dcheck) return dcheck;
    
    if (! dst.empty() && dst.rows != _a.rows && dst.cols != _a.cols) return false;
    if (dst.empty())
        dst = Mat::zeros(_a.rows,_a.cols, CV_8U);
    
    auto A = _a > _b;
    auto B = _a < _c;
    auto C = _b < _c;


    _a.copyTo(dst, A & B);
    _c.copyTo(dst, A & ~B & C);
    _b.copyTo(dst,  A & ~B & ~C);
    _a.copyTo(dst, ~A & ~B);
    _b.copyTo(dst, ~A & B & C);
    _c.copyTo(dst,  ~A & B & ~C);
    
    return true;
}



#endif /* temporal_medianOf3_h */
