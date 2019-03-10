

#include "vision/roiWindow.h"
#include "core/angle_units.h"
#include "vision/gradient.h"
#include "vision/edgel.h"

using namespace svl;

feature::feature (const int& x, const int& y, uint8_t magnitude, uAngle8 angle, uint8_t axis,
         uint8_t posNeighborMag, uint8_t negNeighborMag,
         bool doInterpSubPixel,
         float weight,
         bool isMod180) :
    center_(magnitude), position_(x+0.5f,y+0.5f), angle_(angle), axis_(axis),
    neg_val_(negNeighborMag), pos_val_(posNeighborMag),
    weight_(weight), doneSubPixel_(false),
    isMod180_(isMod180), subPixel_(false), isValid_(true){

    tangent_ = uAngle8((angle_.basic() + 64) % 256 );
    if (doInterpSubPixel) // interpolation requested in ctor
        doEdgeInterp(); // marks it done
}

bool feature::doSubPixelInterpolation () const{
    if (!doneSubPixel_){
        doEdgeInterp();
        doneSubPixel_ = true;
    }
    return doneSubPixel_;
}

fVector_2d feature::uv () const
{
    return fVector_2d(static_cast<float> (sin (tangent_)), static_cast<float>(cos (tangent_)));
}

void feature::doEdgeInterp() const
{
    const auto s = float(neg_val_) + float(pos_val_); //left + right;
    const auto d = float(pos_val_) - float(neg_val_); //right - left;
    const auto c = float(center_) + float(center_); // center + center;
    
    maxima_ = (c == s) ? 0 : d / (2 * (c - s));
    subPixel_ = true;
    
}

bool feature::operator== (const feature& rhs) const
{
    return isValid_ && position_ == rhs.position_ && angle_ == rhs.angle_ && center_ == rhs.center_;
}



