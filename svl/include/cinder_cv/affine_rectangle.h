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
struct EditableRect {
    Area  area;
    vec2  position;
    vec2  scale;
    quat  rotation;
    
    EditableRect() : scale( 1 ) { rotation.x = rotation.y = rotation.z = 0; rotation.w = 1; }
    ~EditableRect() {}
    
    //! Returns the rectangle's model matrix.
    mat4  matrix()
    {
        mat4 m = glm::translate( vec3( position, 0 ) );
        m *= glm::toMat4( rotation );
        m *= glm::scale( vec3( scale, 1 ) );
        m *= glm::translate( vec3( -area.getSize() / 2, 0 ) );
        
        return m;
    }
};


class affineRectangle {
public:
    
    affineRectangle () {}
    /* Args:
     * bounds :         Initial Display Size
     * image_bounds:    Image bounds
     * initial:         initial RotatedRect in image_bounds
     * padded_bounds:   If not default constructed and empty Area, must be larger than image and is used for padding
     *                  Padding value is average gray value of image
     *
     * Returned stated: initial contains the final RotatedRect
     */
    affineRectangle (const Area& bounds, const Area& image_bounds, const cv::RotatedRect& initial,const Area& padded_bounds = Area());
    
    void draw (const Area& display_bounds);

    void mouseDown( const vec2& event_pos );
    void mouseUp( );
    void mouseDrag( const vec2& event_pos );
    void resize( const vec2 change );
    void rotate( const float change );
    void translate ( const vec2 change );
    bool contains ( const vec2 pos);
    void reset (); 
    
    Area area () const { return mRectangle.area; }
    vec2   position () const { return mRectangle.area.getCenter(); }
    bool isClicked () const { return mIsClicked; }
    vec3 mouseToWorld( const ivec2 &mouse, float z = 0 );
    float  degrees () const;
    float  radians () const;

    const cv::RotatedRect& rotatedRectInImage(const Area& image_bounds) const;
    
private:
    void cornersInImage(const Area& image_bounds) const;
    mutable EditableRect   mRectangle;
    ivec2          mMouseInitial;
    EditableRect   mRectangleInitial;
    Area           mInitialArea;
    ivec2          mInitialScreenPos;
    bool           mIsClicked;
    Rectf           mPaddedRect;
    Rectf           mImageRect;
    vec2            mPadded2Image;
    std::vector<vec2> mCornersImageVec;
    mutable std::vector<cv::Point2f> mCornersImageCV;
    cv::RotatedRect mInitialRotatedRect;
    mutable cv::RotatedRect mCvRotatedRect;
    RectMapping    mPadded2Display;
    RectMapping    mDisplay2Padded;

    void pointsToRotatedRect (std::vector<cv::Point2f>& , cv::RotatedRect&  ) const;

    /**
     * @brief Returns a glm/OpenGL compatible viewport vector that flips y and
     * has the origin on the top-left, like in OpenCV.
     *
     * Note: Move to detail namespace / not used at the moment.
     */
    static glm::vec4 get_opencv_viewport(int width, int height)
    {
        return glm::vec4(0, height, width, -height);
    }
    
};


#endif
