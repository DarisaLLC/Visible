
#ifndef _isocline_H
#define _isocline_H

#include <vector>
#include <iostream>
#include "pair.hpp"
#include "vision/roiWindow.h"
#include <tuple>
#include <map>
#include "lineseg.hpp"

// Plain
class feature
{
public:
    
    explicit feature (const fVector_2d &position = fVector_2d(),
                          const uRadian &angle = uRadian(0),
                          double Magnitude = 0,
                          bool subPixel = false,
                          double weight = 1,
                          bool isMod180 = false) :
    position_(position), angle_(angle), weight_(weight), subPixel_(subPixel),
    isMod180_(isMod180)
    { magnitude(Magnitude); }
    
    // default copy constructor, assignment operator and destructor okay
    
    const fVector_2d &position() const { return position_;}
    void position(const fVector_2d &position) {position_ = position;}
    // effect Get/set the position to the given position
    
    uRadian angle() const { return angle_;}
    void angle(uRadian angle) { angle_ = angle;}
    // effect Get/set the angle
    
    double magnitude() const { return magnitude_;}
    void magnitude(double magnitude)
    {
        assert( std::signbit(magnitude_) != 0);
        magnitude_ = magnitude;
    }
    // effect Get/set the magnitude to the given magnitude
    //
    
    double weight() const { return weight_;}
    void weight(double weight) { weight_ = weight;}
    // effect Get/set the weight to the given weight
    
    bool isMod180() const { return isMod180_;}
    void isMod180(bool isMod180) { isMod180_ = isMod180;}
    // effect Get/set the mod180-ness of this featurelet
    
    
    bool operator==(const feature& rhs) const;
    // effect True if *this equals rhs, false otherwise
    // note   Two pof objects are considered equal
    //        if and only if all their members are exactly equal.
    

    
   // friend class pofChainSet;
    
private:
    
    fVector_2d position_;
    uRadian angle_;
    double magnitude_;
    double weight_;
    bool isMod180_;
    bool subPixel_;
};


typedef std::tuple<iPair, fVector_2d, uint8_t, uint8_t, int> protuple;

static void make_probe(int x, int y, uint8_t ang, protuple & fillit)
{
    std::get<0>(fillit) = iPair(x, y);
    std::get<1>(fillit) = fVector_2d(x,y);
    std::get<2>(fillit) = ang;
    std::get<3>(fillit) = 0;
    std::get<4>(fillit) = 0;
}

class isocline;
typedef std::vector<isocline> isocline_vector_t;

std::map<uint32_t, isocline> directed_isoclines_t;



// This class represents a chain of features -- connected and of similar direction

class isocline : public fRect
{
public:

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
    
    isocline()
    {
        clear ();
    }
    
    isocline (const iRect& rect) : fRect(rect.cast_value_type_to<float>()), m_enclosing(rect.cast_value_type_to<float>())
    {
    }
    
    bool check(const feature&) const;
    size_t add(const feature&);
    size_t count () const { return m_probes.size(); }
    fRect enclosing () const { return m_enclosing; }
    
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

    const fVector_2d& first () const { return m_ends[0]; }
    const fVector_2d& last () const { return m_ends[1]; }
    
    uRadian mean_angle () const;
    float mean_angle_norm () const;
    
    const std::vector<feature>& container () const { return m_probes; }
    
   
    // Run length: add probes as long as they overlap
//    static isocline combine (const isocline_vector_t& apart)
 //   {
        
//    }
    
private:
    std::vector<feature> m_probes;
    fVector_2d m_sum;
    fRect m_anchor;
    fRect m_enclosing;
    uRadian m_cline;
    fVector_2d m_ends[2];
    fLineSegment2d m_seg;
    
    void clear ();
    void update ();
};




static std::vector<fRect> get_minbb_per_direction (const chains_directed_t& cba)
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


void extract(roiWindow<P8U> & mag, roiWindow<P8U> & angle, roiWindow<P8U> & peaks, directed_isoclines_t & output, int low_threshold);


#endif // _FEHF_H
