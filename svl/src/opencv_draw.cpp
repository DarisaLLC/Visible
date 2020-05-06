
#include "opencv2/highgui/highgui.hpp"
#include "vision/opencv_utils.hpp"
#include "core/core.hpp"
#include "vision/ellipse.hpp"
#include "core/angle_units.h"
#include "core/lineseg.hpp"
#include <boost/math/special_functions/round.hpp>

using namespace svl;
using namespace cv;
// angle in argument is always treated as radian
// RotatedRect's angle is assumed to be in degrees and is converted to radians
// center is the intersection of the oposite corners of the bounding box.

void svl::drawEllipse(cv::Mat& dst, svl::ellipseShape& ellipse, const cv::Scalar & colourEllipse){
    
    cv::RotatedRect rr;
    ellipse.get(rr);
    cv::ellipse(dst, rr, colourEllipse, 1);
    cv::Point cver(ellipse.x,ellipse.y);
    uRadian ur(ellipse.ccwRotationXtoMajor());
    ur.norm();
    svl::drawCrossOR(dst, cver, colourEllipse, 11, 3, toDegrees(ur.Double()));
    
}

void svl::drawRotatedRect(cv::Mat& dst, const cv::RotatedRect& rRect,
                          const cv::Scalar & ctr_color, const cv::Scalar & fill_color,
                          const cv::Scalar & vertex_color, bool sort, bool fill_only ){
    std::vector<Point2f> vertices(4);
    std::vector<Point2f> vcpy;
    rRect.points(vertices.data());
    vcpy = vertices;
    std::vector<Point> points;
    for (const auto ptf : vertices)points.emplace_back(ptf.x,ptf.y);
    
    if(fill_only){
        fillConvexPoly(dst, points, fill_color);
        return;
    }
    
    if (sort){
        
        std::sort (vcpy.begin(), vcpy.end(),[](const cv::Point2f&a, const cv::Point2f&b){
            return a.x > b.x;
        });
        std::sort (vcpy.begin(), vcpy.end(),[](const cv::Point2f&a, const cv::Point2f&b){
            return a.y < b.y;
        });
        
        for (int i = 0; i < 4; i++){
            line(dst, vcpy[i], vcpy[(i+2)%4], vertex_color);
        }
    }
    
    for (int i = 0; i < 4; i++){
        line(dst, vertices[i], vertices[(i+2)%4], fill_color);
    }
    
    auto mind = std::min(rRect.size.width,rRect.size.height)/2;
    cv::Point cver(rRect.center.x,rRect.center.y);
    svl::drawCrossOR(dst, cver, ctr_color, mind, 2, toRadians(rRect.angle));
}

void svl::drawFourCorners(cv::Mat& dst, const std::vector<Point2f>& vertices, float angle,
                          const cv::Scalar & ctr_color, const cv::Scalar & fill_color,
                          const cv::Scalar & vertex_color){
    std::vector<Point> points;
    for (const auto ptf : vertices)points.emplace_back(ptf.x,ptf.y);
    fillConvexPoly(dst, points, ctr_color);
    
    auto ctr = [] (const std::vector<Point2f>& vertices){
        Point2f sum(0,0);
        for (auto const& pp : vertices) sum += pp;
        return Point2f(sum.x / vertices.size(), sum.y / vertices.size());
    };
    
    Point2f center = ctr(vertices);
    
    auto dis = [](const Point2f& a, const Point2f&b){
        return glm::distance(vec2(a.x,a.y), vec2(b.x,b.y));
    };
    
    auto dis0 = dis(vertices[0],vertices[1]);
    auto dis1 = dis(vertices[1],vertices[2]);
    
    
    for (int i = 0; i < 4; i++){
        line(dst, vertices[i], vertices[(i+2)%4], fill_color);
        cv::Point iver(vertices[i].x,vertices[i].y);
        svl::drawCrossOR(dst, iver,
                         Scalar(255 - (i * 63),vertex_color[1],vertex_color[2]),
                         11, 2, angle);
    }
    cv::Point iver(vertices[0].x,vertices[0].y);
    cv::Point cver(center.x,center.y);
    cv::circle(dst,iver, 7, vertex_color);
    svl::drawCrossOR(dst, cver, ctr_color, std::min(dis0,dis1), 3, angle);
}





void svl::drawArrow(cv::Mat& dst,
                    const cv::Point& p1, const cv::Point& p2,
                    const cv::Scalar & color, const int thickness)
{
    
    // first draw a line connecting p1 to p2:
    svl::drawLine(dst, p1, p2, color, thickness);
    // second, draw an arrow:
    cv::Point pm; pm.x = (p1.x + p2.x) / 2; pm.y = (p1.y + p2.y) / 2;
    int maxd = std::max (dst.cols, dst.rows);
    float norm = float(maxd) / 30.0F; // arrow size
    float angle = atan2((float)(p2.y - p1.y), (float)(p2.x - p1.x));
    float arrow_angle = 20.0F * svl::constants::pi / 180.0F;
    cv::Point pp;
    pp.x = pm.x - int(norm * cos(angle + arrow_angle));
    pp.y = pm.y - int(norm * sin(angle + arrow_angle));
    svl::drawLine(dst, pm, pp, color, thickness);
    pp.x = pm.x - int(norm * cos(angle - arrow_angle));
    pp.y = pm.y - int(norm * sin(angle - arrow_angle));
    svl::drawLine(dst, pm, pp, color, thickness);
}



void svl::drawCrossOR(cv::Mat& dst,
                      const cv::Point& p, const cv::Scalar & color, const int siz,
                      const int thickness, const float ori)
{
  cv::Point p1, p2;

  // compute new X and Y given rotation
  const float newX = siz * sin(ori);
  const float newY = siz * cos(ori);

 
    p1.x = clampValue(int(p.x + newX), 0, dst.cols - 1);
    p1.y = clampValue(int(p.y - newY), 0, dst.rows - 1);
    
    p2.x = clampValue(int(p.x - newX), 0, dst.cols - 1);
    p2.y = clampValue(int(p.y + newY), 0, dst.rows - 1);


svl::drawLine(dst, p1, p2, color, thickness);
    
    Point2f ctr(p.x,p.y);
    Point2f pf1(p1.x,p1.y);
    Point2f pf2(p2.x,p2.y);
    auto p1t = svl::rotatePoint(pf1, ctr, svl::constants::pi_2);
    auto p2t = svl::rotatePoint(pf2, ctr, svl::constants::pi_2);
    p1.x = p1t.x;p1.y = p1t.y;
    p2.x = p2t.x;p2.y = p2t.y;

  svl::drawLine(dst, p1, p2, color, thickness);
}


void svl::drawCross(cv::Mat& dst, const cv::Point& p, const cv::Scalar & color,  int siz,  int thickness)
{
    cv::Point p1, p2;
    
    p1.x = clampValue(p.x - siz, 0, dst.cols - 1);
    p1.y = clampValue(p.y, 0, dst.rows - 1);
    p2.x = clampValue(p.x + siz, 0, dst.cols - 1);
    p2.y = clampValue(p.y, 0, dst.rows - 1);
    
    svl::drawLine(dst, p1, p2, color, thickness);
    
    p1.x = clampValue(p.x, 0, dst.cols - 1);
    p1.y = clampValue(p.y - siz, 0, dst.rows - 1);
    p2.x = clampValue(p.x, 0, dst.cols - 1);
    p2.y = clampValue(p.y + siz, 0, dst.rows - 1);
    
    svl::drawLine(dst, p1, p2, color, thickness);
}


void svl::drawRectOR(cv::Mat& dst,
                     const cv::Rect& r, const cv::Scalar & color, const int thickness,
                     const float ori)
{
    // compute the center of the rect
    const float centerX = r.width;
    const float centerY = r.height;
    
    // compute the distance from the center to the corners
    //const float distX  = r.rightI()  - centerX;
    //const float distY  = r.bottomI() - centerY;
    const float distX  = centerX/2;
    const float distY  = centerY/2;
    const float radius = sqrt(pow(distX,2) + pow(distY,2));
    
    // compute the transformed coordinates
    const float thetaO  = atan(distX/distY);
    const float thetaN1 = thetaO + ori;
    const float thetaN2 = ori - thetaO;
    const float Xnew1   = radius * sin(thetaN1);
    const float Ynew1   = radius * cos(thetaN1);
    const float Xnew2   = radius * sin(thetaN2);
    const float Ynew2   = radius * cos(thetaN2);
    
    // compute new coords based on rotation
    const float Xbr    = r.x + distX + Xnew1; // bottom-right
    const float Ybr    = r.y  + distY + Ynew1;
    const float Xtl    = r.x + distX - Xnew1; // top-left
    const float Ytl    = r.y  + distY - Ynew1;
    const float Xbl    = r.x + distX + Xnew2; // bottom-left
    const float Ybl    = r.y  + distY + Ynew2;
    const float Xtr    = r.x + distX - Xnew2; // top-right
    const float Ytr    = r.y  + distY - Ynew2;
    
    // set up points
    cv::Point p1; p1.x=boost::math::iround(Xbr);p1.y=boost::math::iround(Ybr);
    cv::Point p2; p2.x=boost::math::iround(Xtl);p2.y=boost::math::iround(Ytl);
    cv::Point p3; p3.x=boost::math::iround(Xbl);p3.y=boost::math::iround(Ybl);
    cv::Point p4; p4.x=boost::math::iround(Xtr);p4.y=boost::math::iround(Ytr);
    
    // draw lines
    svl::drawLine(dst, p1, p3, color, thickness);
    svl::drawLine(dst, p1, p4, color, thickness);
    svl::drawLine(dst, p2, p3, color, thickness);
    svl::drawLine(dst, p2, p4, color, thickness);
}

void svl::drawRect(cv::Mat& dst,
                   const cv::Rect& r, const cv::Scalar & color, const int thickness)
{
    
    cv::Point p1;p1.x=r.x;p1.y= r.y;
    cv::Point p2;p2.x=r.x+r.width; p2.y=r.y;
    cv::Point p3;p3.x=p1.x;p3.y=r.y+r.height;
    cv::Point p4;p4.x=p2.x;p4.y=p3.y;
    
    svl::drawLine(dst, p1, p2, color, thickness);
    svl::drawLine(dst, p1, p3, color, thickness);
    svl::drawLine(dst, p3, p4, color, thickness);
    svl::drawLine(dst, p2, p4, color, thickness);
}



void svl::drawLine(cv::Mat& dst, const cv::Point& pos, float ori, float len, const cv::Scalar & color, const int thickness )
{
    
    int x1 = int(cos(ori)*len/2);
    int y1 = int(sin(ori)*len/2);
    
    cv::Point p1; p1.x=pos.x-x1; p1.y=pos.y+y1;
    cv::Point p2; p2.x=pos.x+x1; p2.y=pos.y-y1;
    
    svl::drawLine(dst, p1, p2, color, thickness);
    
}



void svl::drawLine(cv::Mat& dst, const cv::Point& p1, const cv::Point& p2,
                   const cv::Scalar & color, const int thickness)
{
    cv::line ( dst, p1, p2, color, thickness);
}

