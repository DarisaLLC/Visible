
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


// Single Directed Edge Point
class feature
{
public:
    
    feature (const int& x, const int& y, const uAngle8 angle8, const roiWindow<P8U>& mag,
                      bool doInterpSubPixel = false,
                      double weight = 1,
                      bool isMod180 = false,
                      const iPair& kernel_width = iPair(2,2));
                      
    feature (const int& x, const int& y, uint16_t magnitude, uAngle8 angle, uint16_t posNeighborMag, uint16_t negNeighborMag,
                      bool doInterpSubPixel = false,
                      double weight = 1,
                      bool isMod180 = false) :
    center_(magnitude), position_(x+0.5f,y+0.5f), angle_(angle),
    neg_val_(negNeighborMag), pos_val_(posNeighborMag),
    weight_(weight), doSubPixel_(doInterpSubPixel),
    isMod180_(isMod180), subPixel_(false), isValid_(true)
    {
        if (doSubPixel_)
            doEdgeInterp();
    }
    
    // default copy constructor, assignment operator and destructor okay
    
    const fVector_2d &position() const { return position_;}
    void position(const fVector_2d &position) {position_ = position;}
    // effect Get/set the position to the given position
    
    uAngle8 angle() const { return angle_;}
    // effect Get/set the angle
    
    // effect Get/set the magnitude to the given magnitude
    uint16_t magnitude() const { return center_;}
    
    double weight() const { return weight_;}
    void weight(double weight) { weight_ = weight;}
    // effect Get/set the weight to the given weight
    
    // effect Get/set the mod180-ness of this featurelet
    bool isMod180() const { return isMod180_;}
    void isMod180(bool isMod180) { isMod180_ = isMod180;}
    
    /* returns a unit vector in positive gradient direction */
    fVector_2d unit_vector () const
    {
        return fVector_2d(static_cast<float> (cos (angle_)), static_cast<float>(sin (angle_)));
    }
    
    bool isSubPixelInterpolationDone () const { return subPixel_; }
    bool doSubPixelInterpolation () const
    {
        doEdgeInterp();
        return subPixel_;
    }
    
    bool less_x(feature lhs, feature rhs) const { return lhs.position().x() < rhs.position().x(); };
    bool less_y(feature lhs, feature rhs) const { return lhs.position().y() < rhs.position().y(); };
    
    // required for STL
    bool operator<  (const feature& rhs) const { return less_x(*this, rhs) && less_y (*this, rhs); }
    bool operator== (const feature& rhs) const;
    
    
private:
    
    fVector_2d position_;
    uint16_t neg_val_, pos_val_;
    uAngle8 angle_;
    uint16_t center_;
    double weight_;
    bool isMod180_;
    mutable bool subPixel_;
    mutable int16_t maxima_;          /* interpolated subpixel in the direction of the gradient */
    bool doSubPixel_;
    bool isValid_;
    iPair kernel_;
    
    void doEdgeInterp() const
    {
        const int16_t s = neg_val_ + pos_val_; //left + right;
        const int16_t d = pos_val_ - neg_val_; //right - left;
        const int16_t c = center_ + center_; // center + center;
        
        maxima_ = (c == s) ? 0 : d / (2 * (c - s));
        subPixel_ = true;
        
    }
    
 
};

// This class represents a chain of features -- connected and of similar direction
//



class edgels : public fRect
{
public:

    typedef std::vector<feature> feature_vector_t;
    
    typedef std::vector<edgels> directed_features_t;
    
    class null_length : public std::exception
    {
    public:
        virtual const char* what() const throw () {
            return "null length";
        }
    };
    
    class divide_zero: public std::exception
    {
    public:
        virtual const char* what() const throw () { return "divide by zero";}
    };
    
    edgels() :
    fRect (boost::numeric::bounds<float>::highest(), boost::numeric::bounds<float>::highest(),
           boost::numeric::bounds<float>::lowest(), boost::numeric::bounds<float>::lowest())
    {
        clear ();
    }
    
    edgels (const iRect& rect) : fRect(rect.cast_value_type_to<float>()), m_anchor(rect.cast_value_type_to<float>())
    {
        m_sum = fVector_2d ();
        m_unitsum = fVector_2d ();
    }
    
    
    bool check(const feature&) const;
    size_t add(const feature&);
    size_t add(std::vector<feature>::const_iterator& first, std::vector<feature>::const_iterator & last);
    
    size_t count () const { return m_probes.size(); }
    fRect anchor () const { return m_anchor; }

    fVector_2d mid () const
    {
        if (m_probes.empty()) return fVector_2d ();
        return m_sum / m_probes.size();
    }
    uint32_t manhatan_hash ()
    {
        fVector_2d midpt = this->mid ();
        assert(std::signbit(midpt.x()) == 0 && std::signbit(midpt.y()) == 0);
        return static_cast<uint32_t>(midpt.y()) * m_anchor.width() + static_cast<int32_t>(midpt.x());
    }

    
    uRadian mean_angle () const;
    float mean_angle_norm () const;
    
    const std::vector<feature>& container () const { return m_probes; }
    

    
private:
    feature_vector_t m_probes;
    fVector_2d m_sum;
    fVector_2d m_unitsum;
    fRect m_anchor;
    uRadian m_cline;
    fLineSegment2d m_seg;
    void clear ();
    void update (const feature&);
};



void extract(roiWindow<P8U> & mag, roiWindow<P8U> & angle,
             roiWindow<P8U> & peaks,edgels::directed_features_t & output, int low_threshold);



/*
static std::vector<fRect> get_minbb_per_direction (const directed_features_t& cba)
{
    std::vector<fRect> bb (256);
    
    for (int dd = 0; dd < 256; dd++)
    {
        if (cba[dd].empty()) continue;
        fRect tmp;
        for (int ff = 0; ff < cba[dd].size(); ff++)
        {
            tmp |= cba[dd][ff].enclosing();
        }
        bb[dd] = tmp;
    }
    return bb;
}
*/

// std::shared_ptr<std::vector<feature> > FeatureList(const roiWindow<P8U> & magImage, const roiWindow<P8U> & angleImage, uint8_t threshold, bool doSubPixel);

#endif // _FEHF_H
