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
#include "core/lineseg.hpp"

#undef near
#undef far

using namespace ci;
using namespace std;
using namespace cv;

namespace {
    
    
}
/*  @note: add position of image in padded
 */
affineRectangle::affineRectangle (const Area& bounds, const Area& image_bounds, const cv::RotatedRect& initial,const Area& padded_bounds){
    
    // Padded Rect is image bounds in either padded or not padded case
    // Get the offsets
    mImageRect = Rectf(image_bounds);
    mDisplayRect = Rectf(bounds);
    mPaddedRect = (padded_bounds == Area()) ? mImageRect : Rectf(padded_bounds);
    mPadded2Display = RectMapping(mPaddedRect, Rectf(bounds));
    mDisplay2Padded = RectMapping(Rectf(bounds), mPaddedRect);
    mPadded2Image = (mPaddedRect.getSize() - mImageRect.getSize()) / 2.0f;
    
    mInitialRotatedRect = initial;
    std::vector<cv::Point2f> corners(4);
    mInitialRotatedRect.center += toOcv(mPadded2Image);
    mInitialRotatedRect.points(corners.data());
    pointsToRotatedRect(corners, mInitialRotatedRect);
    mInitialRotatedRect.points(corners.data());

    
    vec2 ul = mPadded2Display.map(vec2(corners[1].x,corners[1].y));
    vec2 lr = mPadded2Display.map(vec2(corners[3].x,corners[3].y));
  //  vec2 ctr = mPadded2Display.map(fromOcv(mInitialRotatedRect.center));
    
    uDegree rra(mInitialRotatedRect.angle);
    uRadian rrr(rra);
    rotate(rrr.Double());
    
    mRectangle.area  = Area (ul,lr);
    auto initial_aspect = (float)initial.size.width / (float) initial.size.height;
//    auto guess_aspect = (float)mRectangle.area.getWidth() / (float) mRectangle.area.getHeight();
    mWoHaspect = initial_aspect;


    
    mRectangle.position = vec2(0,0);
    mInitialArea = mRectangle.area;
    mInitialScreenPos = vec2(0,0);
    mIsClicked = false;
    mIsOver = false;
    mSelected = -1;
    
}

int affineRectangle::getNearestIndex( const ivec2 &pt ) const
{
    uint8_t index = 0;
    float   distance = 10.0e6f;
    
    for( uint8_t i = 0; i < 4; ++i ) {
        const float d = glm::distance( vec2( mCornersImageVec[i].x, mCornersImageVec[i].y ), vec2( pt ) );
        if( d < distance ) {
            distance = d;
            index = i;
        }
    }
    
    return index;
}

void affineRectangle::mouseMove(  const vec2& event_pos )
{
    mIsOver = contains( event_pos );
}

void affineRectangle::resize(const Area &display_bounds){
    
    mDisplayRect.getCenteredFit(Rectf(display_bounds), true);
    mPadded2Display = RectMapping(mPaddedRect, mDisplayRect);
    mDisplay2Padded = RectMapping(mDisplayRect, mPaddedRect);
}

bool affineRectangle::contains (const vec2 pos){
    vec3 object = mouseToWorld(pos);
    return area().contains( vec2( object ) );
}

void affineRectangle::scale( const vec2 change )
{
    mRectangle.scale += change;
}


void affineRectangle::rotate( const float change )
{
    mRectangle.rotation = mRectangle.rotation * glm::angleAxis(change, vec3( 0, 0, 1 ) );
}

void  affineRectangle::translate ( const vec2 change ){
    mRectangle.position += change;
//    mRectangle.area.offset(change);
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
    auto printCorners = [] (const std::vector<vec2>& cc) {
        const float d01 = glm::distance( cc[0], cc[1] );
        const float d02 = glm::distance( cc[0], cc[2] );
        const float d03 = glm::distance( cc[0], cc[3] );
        std::vector<std::pair<int,float>> ds = {{0,d01}, {1,d02}, {2,d03}};
        std::sort (ds.begin(), ds.end(),[](const std::pair<int,float>&a, const std::pair<int,float>&b){
            return a.second > b.second;
        });
        auto print = [](std::pair<int,float>& vv) { std::cout << ":::" << vv.second; };
        std::for_each(ds.begin(), ds.end(), print);
        std::cout << '\n';
    };
    printCorners(mCornersImageVec);
    
    
    auto  toString = [] (const cv::RotatedRect& rr){
        std::ostringstream ss;
        ss << "[ + " << rr.center << " / " << rr.angle << " | " << rr.size.height << " - " << rr.size.width << "]";
        return ss.str();
    };

    mCornersImageCV.clear();
    for(auto const & window : mCornersImageVec){
        mCornersImageCV.emplace_back(toOcv(window));
        std::cout << window.x << "," << window.y << std::endl;
    }

    pointsToRotatedRect(mCornersImageCV, mCvRotatedRect);
    std::cout << toString(mCvRotatedRect);
    
    
    
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
    static Rectf persist;
    Rectf af(area());
    auto diff = af - persist;
    if (diff.x1 != 0 || diff.y1 != 0 || diff.x2 != 0 || diff.y2 != 0){
        std::cout << diff << std::endl;
        persist = af;
    }
    // These corners never change as they are transformed later to instant xform
    std::vector<vec2> corners = { af.getUpperLeft(), af.getUpperRight(),
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
        if (mSelected < 0 || i != mSelected)
            cl = ColorA (0.0, 0.0, 1.0,0.8f);
        {
            gl::ScopedColor color (cl);
            gl::drawStrokedCircle(window, dsize, dsize+1.0f);
        }
        dsize+=1.0f;
        i++;
    }
    {
        const std::vector<vec2>& corners = mCornersImageVec;
        gl::drawLine( corners[0], corners[1] );
        gl::drawLine( corners[1], corners[2] );
        gl::drawLine( corners[2], corners[3] );
        gl::drawLine( corners[3], corners[0] );
    }
    auto printCorners = [&] (std::vector<vec2>& cc) {
        const float d01 = glm::distance( cc[0], cc[1] );
        const float d02 = glm::distance( cc[0], cc[2] );
        const float d03 = glm::distance( cc[0], cc[3] );
        std::vector<std::pair<int,float>> ds = {{0,d01}, {1,d02}, {2,d03}};
        std::sort (ds.begin(), ds.end(),[](const std::pair<int,float>&a, const std::pair<int,float>&b){
            return a.second > b.second;
        });
        auto print = [](std::pair<int,float>& vv) { std::cout << ":" << vv.second; };
        std::for_each(ds.begin(), ds.end(), print);
        std::cout << '\n';
    };
        printCorners(mCornersImageVec);
            gl::popViewport();
    
    
    // Can use setMatricesWindow() or setMatricesWindowPersp() to enable 2D rendering.
    gl::setMatricesWindow(display_bounds.getSize(), true );
    
    // Draw the transformed rectangle.
    gl::pushModelMatrix();
    gl::multModelMatrix( mRectangle.matrix() );

    auto printCornersPlus = [&] (std::vector<vec2>& cc) {
        const float d01 = glm::distance( cc[0], cc[1] );
        const float d02 = glm::distance( cc[0], cc[2] );
        const float d03 = glm::distance( cc[0], cc[3] );
        std::vector<std::pair<int,float>> ds = {{0,d01}, {1,d02}, {2,d03}};
        std::sort (ds.begin(), ds.end(),[](const std::pair<int,float>&a, const std::pair<int,float>&b){
            return a.second > b.second;
        });
        auto print = [](std::pair<int,float>& vv) { std::cout << "::" << vv.second; };
        std::for_each(ds.begin(), ds.end(), print);
        std::cout << '\n';
    };
    printCornersPlus(corners);
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
            ColorA cl (0.0, 1.0, 0.0,0.8f);
            if(mSelected >= 0)
                cl = ColorA (1.0, 0.0, 0.0,0.8f);
            gl::ScopedColor color (cl);
            gl::drawLine( vec2( window.x, window.y - 5 * scale.y ), vec2( window.x, window.y + 5 * scale.y ) );
            gl::drawLine( vec2( window.x - 5 * scale.x, window.y ), vec2( window.x + 5 * scale.x, window.y ) );
        }
        
        {
            vec2 window = area().getCenter();
            const vec2& tl = area().getUL();
            gl::ScopedColor color (ColorA (1.0, 1.0, 0.0,0.8f));
            gl::drawLine( tl, window);
        }
    }


    gl::popModelMatrix();
 
}

void affineRectangle::mouseDown( const vec2& event_pos )
{
    mMouseIsDown = true;
    // Check if mouse is inside rectangle, by converting the mouse coordinates
    // to world space and then to object space.
    vec3 world = mouseToWorld( event_pos );
    vec4 object = glm::inverse( mRectangle.matrix() ) * vec4( world, 1 );
    
    // Now we can simply use Area::contains() to find out if the mouse is inside.
    mIsClicked = mRectangle.area.contains( vec2( object ) );
    if( mIsClicked ) {
        mMouseInitial = event_pos;
        mRectangleInitial = mRectangle;
        auto selected = getNearestIndex( mMouseInitial );
        mSelected = (mSelected < 0) ? selected : -1;
        mInitialPosition = mCornersImageVec[mSelected];
    }
}

void affineRectangle::mouseUp(  )
{
    mIsClicked = false;
    mMouseIsDown = false;
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
        float a0 = atan2( initial.y, initial.x );
        float a1 = atan2( current.y, current.x );
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
        return a.x > b.x;
    });
    
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.y < b.y;
    });
    
    auto cwOrder = [&] (std::vector<cv::Point2f>& cc, std::vector<std::pair<int,float>>& results) {
        const float d01 = glm::distance( fromOcv(cc[0]), fromOcv(cc[1]) );
        const float d02 = glm::distance( fromOcv(cc[0]), fromOcv(cc[2]) );
        const float d03 = glm::distance( fromOcv(cc[0]), fromOcv(cc[3]) );
        std::vector<std::pair<int,float>> ds = {{0,d01}, {1,d02}, {2,d03}};
        std::sort (ds.begin(), ds.end(),[](const std::pair<int,float>&a, const std::pair<int,float>&b){
            return a.second > b.second;
        });
        results = ds;
        auto print = [](std::pair<int,float>& vv) { std::cout << "::" << vv.second; };
        std::for_each(ds.begin(), ds.end(), print);
        std::cout << '\n';
    };

    std::vector<std::pair<int,float>> ranked_corners;
    cwOrder(imagePoints, ranked_corners);
    assert(ranked_corners.size()==3);
    fVector_2d tl (imagePoints[1].x, imagePoints[1].y);
    fVector_2d tr (imagePoints[ranked_corners[0].first].x, imagePoints[ranked_corners[0].first].y);
    fVector_2d bl (imagePoints[ranked_corners[2].first].x, imagePoints[ranked_corners[2].first].y);
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
    rotated_rect.size.width = dLR;
    rotated_rect.size.height = dTB;
    std::cout << toDegrees(sideOne.angle().Double()) << " Side One " << std::endl;
    std::cout << toDegrees(sideTwo.angle().Double()) << " Side Two " << std::endl;
    
    uDegree angle = sideTwo.angle();
    if (angle.Double() < -45.0){
        angle = angle + uDegree::piOverTwo();
        std::swap(rotated_rect.size.width,rotated_rect.size.height);
    }
    rotated_rect.angle = angle.Double();
//    mWoHaspect = (float)rotated_rect.size.width / (float)rotated_rect.size.height;
}

#if 0
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

