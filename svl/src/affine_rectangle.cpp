//
//  affine_rectangle.cpp
//  Visible
//
//  Created by Arman Garakani on 5/13/19.
//
#include "cinder_opencv.h"
#include "cinder/Rect.h"
#include "cinder/Quaternion.h"
#include <numeric>
#include <stdio.h>
#include "cinder_cv/affine_rectangle.h"
#include "cinder/gl/gl.h"
#include "core/angle_units.h"
#include "glm/gtx/matrix_transform_2d.hpp"

#undef near
#undef far

using namespace ci;
using namespace std;
using namespace cv;

/*  @note: add position of image in padded
 */
affineRectangle::affineRectangle (const Area& bounds, const Area& image_bounds, const cv::RotatedRect& initial,const Area& padded_bounds){
    
  //  auto rect_contains = [](const Rectf& outer, const Area& bounds){
  //      return outer.contains(bounds.getUL()) && outer.contains(bounds.getLR());
   // };
    
 //   auto dist = [](const cv::Point2f&a, const cv::Point2f&b){ return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y)); };
    
    mImageRect = Rectf(image_bounds);
    mPaddedRect = (padded_bounds == Area()) ? mImageRect : Rectf(padded_bounds);
    mPadded2Display = RectMapping(mPaddedRect, Rectf(bounds));
    mDisplay2Padded = RectMapping(Rectf(bounds), mPaddedRect);
    mPadded2Image = (mPaddedRect.getSize() - mImageRect.getSize()) / 2.0f;
    mInitialRotatedRect = initial;
    mInitialRotatedRect.center += toOcv(mPadded2Image);
    
    auto initial_bound = mInitialRotatedRect.boundingRect();
    cv::Point2f offset_to_rr_bound (initial.center.x - initial_bound.width / 2.0f, initial.center.y - initial_bound.height / 2.0f);
    mInitialRotatedRect.center += (initial.center - offset_to_rr_bound);
    
    std::cout << mInitialRotatedRect.center   << std::endl;
//    assert(rect_contains(mImageRect, initial_bound));
    // The points array for storing rectangle vertices. The order is bottomLeft, topLeft, topRight, bottomRight.
    cv::Point2f corners [4];
    mInitialRotatedRect.points(corners);
    
//    auto d1 = dist(corners[0],corners[1]);
//    auto d2 = dist(corners[0],corners[2]);
//    auto d3 = dist(corners[0],corners[3]);
//    auto d4 = dist(corners[1],corners[2]);
//    auto d5 = dist(corners[1],corners[3]);
//    auto d6 = dist(corners[2],corners[3]);
//
//    std::cout << d1 << "," << d2 << "," << d3 << std::endl;
//    std::cout << d4 << "," << d5 << "," << d6 << std::endl;
    
    vec2 ul = mPadded2Display.map(vec2(corners[1].x,corners[1].y));
    vec2 lr = mPadded2Display.map(vec2(corners[3].x,corners[3].y));
    vec2 ctr = mPadded2Display.map(fromOcv(mInitialRotatedRect.center));
    
    uDegree rra(mInitialRotatedRect.angle);
    uRadian rrr(rra);
  //  rotate(-rrr.Double());
    
    mRectangle.area  = Area (ul,lr);
    std::cout << mRectangle.area.getCenter() << "  ?  " << ctr << std::endl;
//    assert(area().getWidth() == mInitialRotatedRect.size.width && area().getHeight() == mInitialRotatedRect.size.height);

    
    mRectangle.position = vec2(0,0);
    mInitialArea = mRectangle.area;
    mInitialScreenPos = vec2(0,0);

    mIsClicked = false;
}

void affineRectangle::reset (){
    mRectangle.area = mInitialArea;
    mRectangle.position = mInitialScreenPos;
}

float  affineRectangle::degrees () const
{
    return toDegrees(2*std::acos(mRectangle.rotation.w));
}

float  affineRectangle::radians () const
{
    return 2*std::acos(mRectangle.rotation.w);
}

void affineRectangle::cornersInImage (const Area& image_bounds) const {

    mCornersImageCV.clear();
    for(auto const & window : mCornersImageVec){
        mCornersImageCV.emplace_back(toOcv(window));
        std::cout << window.x << "," << window.y << std::endl;
    }

    pointsToRotatedRect(mCornersImageCV, mCvRotatedRect);
    
}

const cv::RotatedRect&  affineRectangle::rotatedRectInImage (const Area& image_bounds) const{
  
    cornersInImage (image_bounds);
    
    std::cout << degrees() << "   " << mCvRotatedRect.angle << std::endl;

    return mCvRotatedRect;
}

/*
    Notes on Coordinate Transformations
 
 */

void affineRectangle::draw(const Area& display_bounds)
{
    Rectf af(area());
    
    vec2 corners[] = { af.getUpperLeft(), af.getUpperRight(),
        af.getLowerRight(), af.getLowerLeft() };
    mCornersImageVec.clear();
    int i = 0;
    float dsize = 5.0f;
    // Use worldToWindowCoord, and viewport for OpenCv to draw
    // Store world coordinates
    gl::pushViewport(0, mPaddedRect.getHeight(),mPaddedRect.getWidth(),-mPaddedRect.getHeight());
    for( vec2 &corner : corners ) {
        vec4 world = mRectangle.matrix() * vec4( corner, 0, 1 );
        vec2 window = gl::worldToWindowCoord( vec3( world ) );
        mCornersImageVec.push_back(window);
        vec2 scale = mRectangle.scale;
        gl::ScopedColor color (ColorA (0.33, 0.67, 1.0,0.8f));
        gl::drawLine( vec2( window.x, window.y - 10 * scale.y ), vec2( window.x, window.y + 10 * scale.y ) );
        gl::drawLine( vec2( window.x - 10 * scale.x, window.y ), vec2( window.x + 10 * scale.x, window.y ) );

        {
            gl::ScopedColor color (ColorA (0.0, 1.0, 0.0,0.8f));
            gl::drawLine( vec2( window.x, window.y - 5 * scale.y ), vec2( window.x, window.y + 5 * scale.y ) );
            gl::drawLine( vec2( window.x - 5 * scale.x, window.y ), vec2( window.x + 5 * scale.x, window.y ) );
        }
        ColorA cl (1.0, 0.0, 0.0,0.8f);
        if (i==0 || i == 1)
            cl = ColorA (0.0, 0.0, 1.0,0.8f);
        {
            gl::ScopedColor color (cl);
            gl::drawStrokedCircle(window, dsize, dsize+5.0f);
        }
        dsize+=5.0f;
        i++;
    }
    {
        const std::vector<vec2>& corners = mCornersImageVec;
        gl::drawLine( corners[0], corners[1] );
        gl::drawLine( corners[1], corners[2] );
        gl::drawLine( corners[2], corners[3] );
        gl::drawLine( corners[3], corners[0] );
    }
            gl::popViewport();
    
    
#if 1
    // Can use setMatricesWindow() or setMatricesWindowPersp() to enable 2D rendering.
    gl::setMatricesWindow(display_bounds.getSize(), true );
    
    // Draw the transformed rectangle.
    gl::pushModelMatrix();
    gl::multModelMatrix( mRectangle.matrix() );
    // Draw a stroked rect in magenta (if mouse inside) or white (if mouse outside).
    {
        ColorA cl (1.0, 1.0, 1.0,0.8f);
        if (mIsClicked) cl = ColorA (1.0, 0.0, 1.0,0.8f);
        gl::ScopedColor color (cl);
        gl::drawStrokedRect( mRectangle.area );
        gl::drawStrokedEllipse(area().getCenter(), area().getWidth()/2, area().getHeight()/2);
        // Draw the same rect as line segments.
        gl::drawLine( corners[0], corners[1] );
        gl::drawLine( corners[1], corners[2] );
        gl::drawLine( corners[2], corners[3] );
        gl::drawLine( corners[3], corners[0] );
    }
    {
        vec2 scale (1.0,1.0);
        {
            vec2 window = area().getCenter();
            gl::ScopedColor color (ColorA (0.0, 1.0, 0.0,0.8f));
            gl::drawLine( vec2( window.x, window.y - 5 * scale.y ), vec2( window.x, window.y + 5 * scale.y ) );
            gl::drawLine( vec2( window.x - 5 * scale.x, window.y ), vec2( window.x + 5 * scale.x, window.y ) );
        }
        
        {
            vec2 window = area().getCenter();
            const vec2& tl = position();
            gl::ScopedColor color (ColorA (1.0, 1.0, 0.0,0.8f));
            gl::drawLine( tl, window);
        }
    }


    gl::popModelMatrix();
#endif
    
 
}

void affineRectangle::mouseDown( const vec2& event_pos )
{
    // Check if mouse is inside rectangle, by converting the mouse coordinates
    // to world space and then to object space.
    vec3 world = mouseToWorld( event_pos );
    vec4 object = glm::inverse( mRectangle.matrix() ) * vec4( world, 1 );
    
    // Now we can simply use Area::contains() to find out if the mouse is inside.
    mIsClicked = mRectangle.area.contains( vec2( object ) );
    if( mIsClicked ) {
        mMouseInitial = event_pos;
        mRectangleInitial = mRectangle;
    }
}

void affineRectangle::mouseUp(  )
{
    mIsClicked = false;
}

bool affineRectangle::contains (const vec2 pos){
    vec3 object = mouseToWorld(pos);
    return area().contains( vec2( object ) );
}

void affineRectangle::resize( const vec2 change )
{
    mRectangle.scale += change;
}


void affineRectangle::rotate( const float change )
{
    mRectangle.rotation = mRectangle.rotation * glm::angleAxis(change, vec3( 0, 0, 1 ) );
}

void  affineRectangle::translate ( const vec2 change ){
    mRectangle.area.offset(change);
}

void affineRectangle::mouseDrag(const vec2& event_pos )
{
    // Scale and rotate the rectangle.
    if( mIsClicked ) {
        // Calculate the initial click position and the current mouse position, in world coordinates relative to the rectangle's center.
        vec3 initial = mouseToWorld( mMouseInitial ) - vec3( mRectangle.position, 0 );
        vec3 current = mouseToWorld( event_pos ) - vec3( mRectangle.position, 0 );
        
        // Calculate scale by using the distance to the center of the rectangle.
        float d0 = glm::length( initial );
        float d1 = glm::length( current );
        mRectangle.scale = vec2( mRectangleInitial.scale * ( d1 / d0 ) );
        
        // Calculate rotation by taking the angle with the X-axis for both positions and calculating the difference.
        float a0 = math<float>::atan2( initial.y, initial.x );
        float a1 = math<float>::atan2( current.y, current.x );
        mRectangle.rotation = mRectangleInitial.rotation * glm::angleAxis( a1 - a0, vec3( 0, 0, 1 ) );
    }
}

vec3 affineRectangle::mouseToWorld( const ivec2 &mouse, float z )
{
    // Build the viewport (x, y, width, height).
    vec2 offset = gl::getViewport().first;
    vec2 size = gl::getViewport().second;
    vec4 viewport = vec4( offset.x, offset.y, size.x, size.y );
    
    // Calculate the view-projection matrix.
    mat4 transform = gl::getProjectionMatrix() * gl::getViewMatrix();
    
    // Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
    vec3 near = glm::unProject( vec3( mouse.x, size.y - mouse.y, 0 ), mat4(), transform, viewport );
    vec3 far = glm::unProject( vec3( mouse.x, size.y - mouse.y, 1 ), mat4(), transform, viewport );
    
    // Calculate world position.
    return ci::lerp( near, far, ( z - near.z ) / ( far.z - near.z ) );
}



void affineRectangle::pointsToRotatedRect (std::vector<cv::Point2f>& imagePoints, cv::RotatedRect& rotated_rect ) const {
    // get the left most
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.x <= b.x;
    });
    auto tl = imagePoints[0];
    auto bl = imagePoints[1];
    if (imagePoints[1].y < imagePoints[0].y)
        std::swap(tl,bl);
    
    auto dist = [](const cv::Point2f&a, const cv::Point2f&b){ return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y)); };
    auto tr = imagePoints[2];
    auto br = imagePoints[3];
    if(dist(tr, tl) > dist(br, tl))
        std::swap(tr,br);
    imagePoints.clear();
    imagePoints = {tl,tr,br, bl};
    cv::Point2f ctr(0,0);
    for (auto const& p : imagePoints){
        ctr.x += p.x;
        ctr.y += p.y;
    }
    ctr.x /= 4;
    ctr.y /= 4;
    rotated_rect.center = ctr;
    float dLR = dist(tl,tr);
    float dTB = dist(tl,bl);
    rotated_rect.size.width = std::max(dLR,dTB);
    rotated_rect.size.height = std::min(dLR,dTB);
    rotated_rect.angle = degrees();
}


#if 0
affineRectangle::affineRectangle () : mScale ( 1 )
{
    mCornersWorld.resize(4);
    mCornersWindow.resize(4);
    mXformMat = cv::Mat(2,3,CV_64F);
    m_dirty = false;
}

affineRectangle& affineRectangle::operator=(const affineRectangle& rhs){
    mPosition = rhs.position();
    mScale = rhs.scale();
    mRotation = rhs.rotation();
    mXformMat = rhs.mXformMat;
    mCornersWorld = rhs.mCornersWorld;
    mCornersWindow = rhs.mCornersWindow;
    mWindowCenter = rhs.mWindowCenter;
    mWorldCenter = rhs.mWorldCenter;
    mCvRotatedRect = rhs.mCvRotatedRect;
    mWorldTransform = rhs.mWorldTransform;
    mScreen2Normal = rhs.mScreen2Normal;
    m_dirty = rhs.m_dirty.load();
    return *this;
}

affineRectangle::affineRectangle(const affineRectangle& other){
    mPosition = other.position();
    mScale = other.scale();
    mRotation = other.rotation();
    mXformMat = other.mXformMat;
    mCornersWorld = other.mCornersWorld;
    mCornersWindow = other.mCornersWindow;
    mWindowCenter = other.mWindowCenter;
    mWorldCenter = other.mWorldCenter;
    mCvRotatedRect = other.mCvRotatedRect;
    mWorldTransform = other.mWorldTransform;
    mScreen2Normal = other.mScreen2Normal;
    m_dirty = other.m_dirty.load();
}
affineRectangle::affineRectangle (const Rectf& ar) : Rectf(ar)
{
    mScale = vec2(1);
    mCornersWorld.resize(4);
    mCornersWindow.resize(4);
    mXformMat = cv::Mat(2,3,CV_64F);
    mScreen2Normal = RectMapping(ar, Rectf( 0.0f, 0.0f, 1.0f, 1.0f ) );

    m_dirty = false;
}

void affineRectangle::update () const {

        // Translate to the position
        mat4 m = glm::translate( vec3( mPosition, 0 ) );
        // Rotate
        m *= glm::toMat4( mRotation );
        // Scale
        m *= glm::scale( vec3( mScale, 1 ) );
        // Get the center point
        m *= glm::translate( vec3( -getSize() * 0.5f, 0 ) );
        
        mWorldTransform = m;
        
        // Normalize us + setup the display locations
        mCornersWorld.resize(4);
        Rectf* mArea ((affineRectangle*)this);
//        mArea->canonicalize();
        mCornersWorld[0]=mScreen2Normal.map(mArea->getUpperLeft());
        mCornersWorld[1]=mScreen2Normal.map(mArea->getUpperRight());
        mCornersWorld[2]=mScreen2Normal.map(mArea->getLowerRight());
        mCornersWorld[3]=mScreen2Normal.map(mArea->getLowerLeft());
        mWorldCenter = mScreen2Normal.map(mArea->getCenter());
        
        // Get the image locations in cv::Point
        mCornersWindow.resize(4);
        for (int i = 0; i < 4; i++)
        {
            vec4 world = getWorldTransform() * vec4( mCornersWorld[i], 0, 1 );
            vec2 window = gl::worldToWindowCoord( vec3( world ) );
            mCornersWindow[i]=toOcv(window);
        }
        {
            vec4 world = getWorldTransform() * vec4( mWorldCenter, 0, 1 );
            vec2 window = gl::worldToWindowCoord( vec3( world ) );
            mWindowCenter=toOcv(window);
        }
        
        //
        mCvRotatedRect = cv::minAreaRect(mCornersWindow);
        update_rect_centered_xform();

}

const Rectf&   affineRectangle::area () const { return *this; }
const vec2&    affineRectangle::position () const { return mPosition; }
const vec2&    affineRectangle::scale () const { return mScale; }
const quat    affineRectangle::rotation ()const { return mRotation; }
const RectMapping&    affineRectangle::screenToNormal ()const { return mScreen2Normal; }

void    affineRectangle::scale (const vec2& ns) { mScale = ns; m_dirty = true; }
void  affineRectangle::rotation (const quat& rq) { mRotation = rq; m_dirty = true; }
void    affineRectangle::position (const vec2& np) { mPosition = np; m_dirty = true; }

void  affineRectangle::resize (const int width, const int height){
    ivec2 dd (width - getWidth(), height - getHeight());
    inflate(dd);
    m_dirty = true;
}

void affineRectangle::inflate (const vec2 trim){
    static_cast<Rectf*>(this)->inflate(trim);
    m_dirty = true;
}
//vec2 scale_map (const vec2& pt)
//{
//    return (vec2 (pt[0]*mScale[0],pt[1]*mScale[1]));
//}

//! Returns the rectangle's model matrix.
const mat4  & affineRectangle::getWorldTransform() const
{
    return mWorldTransform;
}


float  affineRectangle::degrees () const
{
    return toDegrees(2*std::acos(mRotation.w));
}

float  affineRectangle::radians () const
{
    return 2*std::acos(mRotation.w);
}


//The order is bottomLeft, topLeft, topRight, bottomRight.
// return in tl,tr,br,bl
// @note Check if this makes sense in all cases
const std::vector<vec2>&  affineRectangle::worldCorners () const
{
    return mCornersWorld;
}

const std::vector<Point2f>&  affineRectangle::windowCorners () const
{
    return mCornersWindow;
}

const cv::RotatedRect&  affineRectangle::rotated_rect () const
{
    return mCvRotatedRect;
}

Point2f  affineRectangle::map_point(Point2f pt) const
{
    const cv::Mat& transMat = mXformMat;
    pt.x = transMat.at<float>(0, 0) * pt.x + transMat.at<float>(0, 1) * pt.y + transMat.at<float>(0, 2);
    pt.y = transMat.at<float>(1, 0) * pt.y + transMat.at<float>(1, 1) * pt.y + transMat.at<float>(1, 2);
    return pt;
}

void  affineRectangle::update_rect_centered_xform () const
{
    const RotatedRect& rr = mCvRotatedRect;
    float w = rr.size.width / 2;
    float h = rr.size.height / 2;
    Mat m(2, 3, CV_64FC1);
    float ang = rr.angle * CV_PI / 180.0;
    mXformMat.at<double>(0, 0) = cos(ang);
    mXformMat.at<double>(1, 0) = sin(ang);
    mXformMat.at<double>(0, 1) = -sin(ang);
    mXformMat.at<double>(1, 1) = cos(ang);
    mXformMat.at<double>(0, 2) = rr.center.x - w*cos(ang) + h*sin(ang);
    mXformMat.at<double>(1, 2) = rr.center.y - w*sin(ang) - h*cos(ang);
    
}


#endif

