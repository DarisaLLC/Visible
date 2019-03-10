
#ifndef _isocline_H
#define _isocline_H

#include <vector>
#include <iostream>
#include "core/pair.hpp"
#include "vision/roiWindow.h"
#include <tuple>
#include <map>
#include "core/lineseg.hpp"
#include <boost/numeric/conversion/bounds.hpp>

namespace svl {
// Single Directed Edge Point
class feature
{
public:
    feature (const int& x, const int& y, uint8_t magnitude, uAngle8 angle, uint8_t axis,
             uint8_t posNeighborMag, uint8_t negNeighborMag,
                      bool doInterpSubPixel = false,
                      float weight = 1.0f,
             bool isMod180 = false);
    
    // default copy constructor, assignment operator and destructor okay
    
    const fVector_2d &position() const { return position_;}
    void position(const fVector_2d &position) {position_ = position;}
    // effect Get/set the position to the given position
    
    const uAngle8& angle() const { return angle_;}
    const uAngle8& tangent () const { return tangent_;}
    // effect Get/set the angle
    
    // effect Get/set the magnitude to the given magnitude
    uint16_t magnitude() const { return center_;}
    
    float weight() const { return weight_;}
    void weight(float weight) { weight_ = weight;}
    // effect Get/set the weight to the given weight
    
    // effect Get/set the mod180-ness of this featurelet
    bool isMod180() const { return isMod180_;}
    void isMod180(bool isMod180) { isMod180_ = isMod180;}
    
    /* returns a unit vector in positive gradient direction */
    fVector_2d uv () const;
    
    bool doSubPixelInterpolation () const;
    
    bool less_x(feature lhs, feature rhs) const { return lhs.position().x() < rhs.position().x(); };
    bool less_y(feature lhs, feature rhs) const { return lhs.position().y() < rhs.position().y(); };
    
    // required for STL
    bool operator<  (const feature& rhs) const { return less_x(*this, rhs) && less_y (*this, rhs); }
    bool operator== (const feature& rhs) const;
    
    
private:
    uint8_t axis_;
    fVector_2d position_;
    uint8_t neg_val_, pos_val_;
    uAngle8 angle_;
    uAngle8 tangent_;
    uint8_t center_;
    float weight_;
    bool isMod180_;
    mutable bool subPixel_;
    mutable float maxima_;          /* interpolated subpixel in the direction of the gradient */
    mutable bool doneSubPixel_;
    bool isValid_;
    
    void doEdgeInterp() const;
    
 
};
}

#endif // _FEHF_H
