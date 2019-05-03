/** @dir cinder
 * @brief Namespace @ref svl
 */
/** @namespace svl
 * @brief Cinder Additions
 */



#ifndef __AFFINE_RECTANGLE__
#define __AFFINE_RECTANGLE__

#include "cinder_opencv.h"
#include "cinder/Quaternion.h"
#include <numeric>

#undef near
#undef far

using namespace ci;
using namespace ci::app;
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
 */

class affineRectangle : public Rectf
{
public:
    affineRectangle () : mScale ( 1 )
    {
        mCornersWorld.resize(4);
        mCornersWindow.resize(4);
        mCornersCv.resize(4);
    }
    
    affineRectangle (const Rectf& ar) : Rectf(ar)
    {
        mScale = vec2(1);
        mCornersWorld.resize(4);
        mCornersWindow.resize(4);
        mCornersCv.resize(4);
    }
    
    ~affineRectangle () {}
    
    Rectf&  area () { return *this; }
    
    void   position (const vec2& np) { mPosition = np; }
    
    const vec2&   position () { return mPosition; }
    
    const vec2&   scale () const { return mScale; }
    
    void   scale (const vec2& ns) { mScale = ns; }
    
    quat   rotation ()const { return mRotation; }
    void rotation (const quat& rq) { mRotation = rq; }
    
    vec2 scale_map (const vec2& pt)
    {
        return (vec2 (pt[0]*mScale[0],pt[1]*mScale[1]));
    }
    
    //! Returns the rectangle's model matrix.
    mat4  matrix() const
    {
        mat4 m = glm::translate( vec3( mPosition, 0 ) );
        m *= glm::toMat4( mRotation );
        m *= glm::scale( vec3( mScale, 1 ) );
        m *= glm::translate( vec3( -getSize() * 0.5f, 0 ) );
        return m;
    }
    
    float degrees () const
    {
        return 2*toDegrees(std::acos(mRotation.w));
    }
    
    float radians () const
    {
        return 2*std::acos(mRotation.w);
    }
    
    float uni_scale () const { return (mScale.x+mScale.y) / 2.0f; }
    
    float cvMinorAxis () const { return mCvMinorAxis; }
    
    //The order is bottomLeft, topLeft, topRight, bottomRight.
    // return in tl,tr,br,bl
    // @note Check if this makes sense in all cases 
    const std::vector<vec2>& worldCorners () const
    {
        mCornersWorld.resize(4);
        Rectf* mArea ((affineRectangle*)this);
        mArea->canonicalize();
        mCornersWorld[0]=mArea->getUpperLeft();
        mCornersWorld[1]=mArea->getUpperRight();
        mCornersWorld[2]=mArea->getLowerRight();
        mCornersWorld[3]=mArea->getLowerLeft();
        
        return mCornersWorld;
    }
    
    std::vector<Point2f>& windowCorners (Point2f& center)
    {
        Point2f ctr (0.0f,0.0f);
        mCornersWindow.resize(4);
        for (int i = 0; i < 4; i++)
        {
            vec4 world = matrix() * vec4( mCornersWorld[i], 0, 1 );
            vec2 window = gl::worldToWindowCoord( vec3( world ) );
            mCornersWindow[i]=toOcv(window);
            ctr.x += mCornersWorld[i].x;
            ctr.y += mCornersWorld[i].y;
        }
        center.x = ctr.x / 4.0f;
        center.y = ctr.y / 4.0f;
        
        return mCornersWindow;
    }
    
#ifdef NOT_YET
    cv::Mat rectify_image(cv::Mat& image) {
        Point2f center;
        std::vector<cv::Point2f> ipts = windowCorners(center);
        auto rr = cv::minAreaRect(ipts);
        if (rr.angle < -45.0){
            rr.angle += 90.0;
            auto sz = rr.size;
            rr.size.width = sz.height;
            rr.size.height = sz.width;
        }
        
        std::vector<cv::Point2f> pts(4);
        rr.points(pts.data());
        
        auto _scale = [](const cv::Point2f& pt, float mul){
            cv::Point2f rr (pt.x * mul, pt.y*mul);
            return rr;
        };
        
        cv::Point2f tl = _scale(pts[0], 0.5f);
        cv::Point2f tr = _scale(pts[1], 0.5f);
        cv::Point2f br = _scale(pts[2], 0.5f);
        cv::Point2f bl = _scale(pts[3], 0.5f);
        
        // compute the width of the new image, which will be the
        // maximum distance between bottom - right and bottom - left
        // x - coordiates or the top - right and top - left x - coordinates
        float widthA = cv::norm(br - bl);
        float widthB = cv::norm(tr - tl);
        int maxWidth = std::max(int(widthA), int(widthB));
        
        // compute the height of the new image, which will be the
        // maximum distance between the top - right and bottom - right
        // y - coordinates or the top - left and bottom - left y - coordinates
        float heightA = cv::norm(tr - br);
        float heightB = cv::norm(tl - bl);
        int maxHeight = std::max(int(heightA), int(heightB));
        
        // now that we have the dimensions of the new image, construct
        // the set of destination points to obtain a "birds eye view",
        // (i.e. top-down view) of the image, again specifying points
        // in the top - left, top - right, bottom - right, and bottom - left
        // order
        std::vector<cv::Point2f> dst(4);
        dst[0] = cv::Point2f(0, maxHeight - 1);
        dst[3] = cv::Point2f(maxWidth - 1, maxHeight - 1);
        dst[2] = cv::Point2f(maxWidth - 1, 0);
        dst[1] = cv::Point2f(0, 0);
        
        // compute the perspective transform matrix and then apply it
        cv::Mat M = cv::getPerspectiveTransform(pts, dst);
        cv::Mat warped;
        cv::warpPerspective(image, warped, M, cv::Size(maxWidth, maxHeight));
        
        // return the warped image
        return warped;
    }
#endif
    
    cv::RotatedRect get_rotated_rect ()
    {
        Point2f center;
        std::vector<Point2f>& corners = windowCorners (center);
        auto rr = cv::minAreaRect(corners);
        if (rr.angle < -45.0){
            rr.angle += 90.0;
            auto sz = rr.size;
            rr.size.width = sz.height;
            rr.size.height = sz.width;
        }
        rr.points(corners.data());
        
        float lr_dist = glm::distance(fromOcv(corners[2]), fromOcv(corners[1]));
        float td_dist = glm::distance(fromOcv(corners[1]), fromOcv(corners[0]));
        Point2f minor_axis = (lr_dist < td_dist) ? corners[2] - corners[1] : corners[0] - corners[1];
        mCvMinorAxis = toDegrees (atan2(-minor_axis.y , minor_axis.x));

        cv::Size rrsize (std::min(lr_dist,td_dist), std::max(lr_dist, td_dist));
        float dirty_angle = mCvMinorAxis;
        return cv::RotatedRect(toOcv(position()), rrsize, dirty_angle);

    }
    
    
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
    
    
private:
    
    void compute2DRectCorners(const cv::RotatedRect rect, std::vector<cv::Point2f> & corners) const
    {
        std::cout << "cv: " << rect.angle << std::endl;
        
        float a = (float) sin(toRadians(rect.angle)) * 0.5f;
        float b = (float) cos(toRadians(rect.angle)) * 0.5f;
        corners[0].x = rect.center.x - a * rect.size.height - b * rect.size.width;
        corners[0].y = rect.center.y + b * rect.size.height - a * rect.size.width;
        corners[1].x = rect.center.x + a * rect.size.height - b * rect.size.width;
        corners[1].y = rect.center.y - b * rect.size.height - a * rect.size.width;
        corners[2].x = 2 * rect.center.x - corners[0].x;
        corners[2].y = 2 * rect.center.y - corners[0].y;
        corners[3].x = 2 * rect.center.x - corners[1].x;
        corners[3].y = 2 * rect.center.y - corners[1].y;
    }
    
    Point2f map_point(Point2f pt) const
    {
        const cv::Mat& transMat = mXformMat;
        pt.x = transMat.at<float>(0, 0) * pt.x + transMat.at<float>(0, 1) * pt.y + transMat.at<float>(0, 2);
        pt.y = transMat.at<float>(1, 0) * pt.y + transMat.at<float>(1, 1) * pt.y + transMat.at<float>(1, 2);
        return pt;
    }
    
    void update_rect_centered_xform () const
    {
        mXformMat = getRotationMatrix2D(toOcv(mPosition),degrees(),uni_scale());
    }
    vec2   mPosition;
    vec2   mScale;
    quat   mRotation;
    mutable cv::Mat mXformMat;
    mutable std::vector<Point2f> mCornersCv;
    mutable std::vector<vec2> mCornersWorld;
    mutable std::vector<Point2f> mCornersWindow;
    mutable float mCvMinorAxis;
};


#endif
