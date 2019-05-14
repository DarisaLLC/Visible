/** @dir cinder
 * @brief Namespace @ref svl
 */
/** @namespace svl
 * @brief Cinder Additions
 */



#ifndef __AFFINE_RECTANGLE__
#define __AFFINE_RECTANGLE__

#include "cinder/Cinder.h"
#include "cinder_opencv.h"
#include "cinder/Quaternion.h"
#include <numeric>
#include <atomic>

#undef near
#undef far

using namespace ci;
using namespace std;
using namespace cv;

//! A rectangle that can be transformed.
/*
 struct EditableRect
 {
 Rectf  area;
 vec2   position;
 vec2   scale;
 quat   rotation;
 
 EditableRect() : scale( 1 ) {}
 ~EditableRect() {}
 
 //! Returns the rectangle's model matrix.
 mat4  matrix()
 {
 mat4 m = glm::translate( vec3( position, 0 ) );
 m *= glm::toMat4( rotation );
 m *= glm::scale( vec3( scale, 1 ) );
 m *= glm::translate( vec3( -area.getSize() * 0.5f, 0 ) );
 
 return m;
 }
 
 };
 
 
 OpenCv RotatedRect:
 The rotation angle in a clockwise direction. When the angle is 0, 90, 180, 270 etc., the rectangle becomes an up-right rectangle.
 
 
 */

class affineRectangle : private Rectf
{
public:
    affineRectangle ();
    affineRectangle (const Rectf& ar);
    ~affineRectangle () {}
    
    affineRectangle(const affineRectangle& other);
    affineRectangle& operator=(const affineRectangle& rhs);
    
    void update () const;
    
    // Getters
    const Rectf&  area () const;
    const vec2&   position () const;
    Rectf  norm_area () const { return screenToNormal().map(area()); }
    vec2   norm_position () const { return screenToNormal().map(position()); }
    
    const vec2&   scale () const;
    const quat   rotation ()const;
    //! Returns the rectangle's model matrix.
    const mat4  &getWorldTransform() const;
    float degrees () const;
    float radians () const;
    const cv::RotatedRect& rotated_rect () const;
    const RectMapping&    screenToNormal () const;
    
    
    // Modifiers Call update
    void   scale (const vec2& ns);
    void   position (const vec2& np);
    void rotation (const quat& rq);
    void resize (const int width, const int height);
    void inflate (const vec2 trim);
    
    //The order is bottomLeft, topLeft, topRight, bottomRight.
    // return in tl,tr,br,bl
    // @note Check if this makes sense in all cases 
    const std::vector<vec2>& worldCorners () const;
    const std::vector<Point2f>& windowCorners () const;
    
    vec2 scale_map (const vec2& pt)
    {
        return (vec2 (pt[0]*mScale[0],pt[1]*mScale[1]));
    }
    
    
    static void compute2DRectCorners(const cv::RotatedRect rect, std::vector<cv::Point2f> & corners)
    {
        corners.resize(4);
        return rect.points(corners.data());
    }
    
#if 0
    static void draw_oriented_rect (const Rectf& arect)
    {
        vec2 corners[] = { arect.getUpperLeft(), arect.getUpperRight(),
            arect.getLowerRight(), arect.getLowerLeft() };
        
        for(unsigned index = 0; index < 4; index++)
        {
            unsigned index_to = (index+1)%4;
            gl::drawLine( corners[index], corners[index_to]);
        }
    }
    
    static void draw_oriented_rect (const std::vector<Point2f>& corners)
    {
        for(unsigned index = 0; index < corners.size(); index++)
        {
            unsigned index_to = (index+1)%(corners.size());
            gl::drawLine( fromOcv(corners[index]), fromOcv(corners[index_to]));
        }
    }
#endif
    

private:
    Point2f map_point(Point2f pt) const;
    void update_rect_centered_xform () const;
    
    vec2   mPosition;
    vec2   mScale;
    quat   mRotation;
    mutable cv::Mat mXformMat;
    mutable std::vector<vec2> mCornersWorld;
    mutable std::vector<Point2f> mCornersWindow;
    mutable Point2f mWindowCenter;
    mutable vec2 mWorldCenter;
    mutable RotatedRect mCvRotatedRect;
    mutable glm::mat4 mWorldTransform;
    
    mutable RectMapping mScreen2Normal;
    mutable std::atomic<bool> m_dirty;
    
};


#endif
