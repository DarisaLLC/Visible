
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
    
    const uint8_t& axis () const { return axis_; }
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
    
    class  featureSet
    {
    public:
        featureSet() { setUnbound();}
        
        featureSet(const std::vector< feature>& newedges, const iRect& span, const fVector_2d origin = fVector_2d () )
        {
            setUnbound();
            m_edges = newedges;
            set_xform_origin( origin);
            m_pel_rect = span;
        }
        
    //    static featureSet create (cv::Mat& image,  qEdgeHelper& params = qEdgeHelper ());
    //    static void draw (cv::Mat& image, featureSet& features, qEdgeHelper& params = qEdgeHelper () );
    
        
        // default copy, assign, dtor okay
        
        void setUnbound()
        {
            m_edges.clear (); m_isBound = false;m_com_cached = false; m_cop_cached = false;m_bbox_cached = false;
        }
        
        bool isBound() const { return m_isBound; }
        
        size_t nEdges() const {return m_edges.size();}
        
        double magScale() const {return m_magScale;}
        
        const std::vector< feature>& edges() const {return m_edges;}
        
        void edges(const std::vector< feature>&newedges)
        {
         //   m_edges.resize(newedges.size());
         //   std::copy (newedges.begin(), newedges.end(), m_edges.begin());
          //  m_cop_cached = m_com_cached = m_bbox_cached = false;
        }
        
        fRect boundingBox() const
        {
            if (! m_bbox_cached ) m_compute_bbox();
            return m_bbox;
        }
        
        fVector_2d centerOfMass_2d() const
        {
            if (! m_com_cached) m_compute_com();
            return m_com;
        }
        
        fVector_2d centerOfMass_1d() const
        {
            if (! m_cop_cached ) m_compute_cop();
            return m_cop;
        }
        bool centerOfMass_1d_exists () const
        {
            return m_cop_exists;
        }
        
        
        
        // required for STL
        bool operator<  (const featureSet& rhs) const;
        bool operator== (const featureSet& rhs) const;
        
    private:
        fVector_2d m_xform_origin;
        mutable bool m_com_cached, m_cop_cached, m_bbox_cached;
        double m_magScale;
        std::vector< feature> m_edges;
        bool m_isBound;
        mutable bool m_cop_exists;
        mutable fVector_2d m_com;
        mutable fVector_2d m_cop;
        mutable fRect m_bbox;
        mutable iRect m_pel_rect;
        svl::xform m_xform;
        
        void set_xform_origin (const fVector_2d& ctr)
        {
            m_xform_origin = ctr;
        }
        
        
        void m_compute_bbox () const
        {
            float minx = boost::numeric::bounds<float>::highest(), miny = boost::numeric::bounds<float>::highest(),
            maxx = boost::numeric::bounds<float>::lowest(), maxy = boost::numeric::bounds<float>::lowest();
            
            std::vector< feature>::const_iterator fItr = m_edges.begin();
            for (; fItr != m_edges.end(); fItr++)
            {
                const fVector_2d& p = fItr->position ();
                minx = std::min (p.x(), minx);
                miny = std::min (p.y(), miny);
                maxx = std::max (p.x(), maxx);
                maxy = std::max (p.y(), maxy);
            }
            
            m_bbox = fRect (minx, miny, maxx, maxy);
            m_bbox_cached = true;
        }
        
        void m_compute_com () const
        {
//            float sx = 0, sy = 0;
//            std::vector< feature>::const_iterator f = m_edges.begin();
//            for (; f != m_edges.end(); f++)
//            {
//                sx += f->directX();
//                sy += f->directY();
//            }
//            m_com = fVector_2d (sx / m_edges.size(), sy / m_edges.size());
//            m_com_cached = true;
        }
        void m_compute_cop () const
        {
//            motionCenter mc;
//            std::vector< feature>::const_iterator f = m_edges.begin();
//            for (; f != m_edges.end(); f++)
//            {
//                mc.add (f->directX(), f->directY(), f->angle());
//            }
//            m_cop_exists = mc.center(m_cop)    ;
//            m_cop_cached = true;
        }
        

        
        
    };
    

    
}

#endif // _FEHF_H
