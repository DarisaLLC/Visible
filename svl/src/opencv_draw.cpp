

#include "vision/opencv_utils.hpp"
#include "core/core.hpp"

using namespace svl;

// Drawing routine on top of openCV / IPL Image

void svl::drawArrow(cv::Mat& dst,
               const cv::Point& p1, const cv::Point& p2,
               const cv::Scalar & color, const int thickness)
{

  // first draw a line connecting p1 to p2:
  svl::drawLine(dst, p1, p2, color, thickness);
  // second, draw an arrow:
  CvPoint pm; pm.x = (p1.x + p2.x) / 2; pm.y = (p1.y + p2.y) / 2;
  int maxd = std::max (dst.cols, dst.rows);
  float norm = float(maxd) / 30.0F; // arrow size
  float angle = atan2((float)(p2.y - p1.y), (float)(p2.x - p1.x));
  float arrow_angle = 20.0F * svl::constants::pi / 180.0F;
  CvPoint pp;
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
  CvPoint p1, p2;

  // compute new X and Y given rotation
  const float newX = siz * sin(ori);
  const float newY = siz * cos(ori);

  p1.x = clampValue(int(p.x - newY), 0, dst.cols - 1);
  p1.y = clampValue(int(p.y + newX), 0, dst.rows - 1);

  p2.x = clampValue(int(p.x + newY), 0, dst.cols - 1);
  p2.y = clampValue(int(p.y - newX), 0, dst.rows - 1);

  svl::drawLine(dst, p1, p2, color, thickness);

  p1.x = clampValue(int(p.x - newX), 0, dst.cols - 1);
  p1.y = clampValue(int(p.y - newY), 0, dst.rows - 1);

  p2.x = clampValue(int(p.x + newX), 0, dst.cols - 1);
  p2.y = clampValue(int(p.y + newY), 0, dst.rows - 1);

  svl::drawLine(dst, p1, p2, color, thickness);
}


void svl::drawCross(cv::Mat& dst, const cv::Point& p, const cv::Scalar & color,  int siz,  int thickness)
{
  CvPoint p1, p2;

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
  CvPoint p1; p1.x=boost::math::iround(Xbr);p1.y=boost::math::iround(Ybr);
  CvPoint p2; p2.x=boost::math::iround(Xtl);p2.y=boost::math::iround(Ytl);
  CvPoint p3; p3.x=boost::math::iround(Xbl);p3.y=boost::math::iround(Ybl);
  CvPoint p4; p4.x=boost::math::iround(Xtr);p4.y=boost::math::iround(Ytr);

  // draw lines
  svl::drawLine(dst, p1, p3, color, thickness);
  svl::drawLine(dst, p1, p4, color, thickness);
  svl::drawLine(dst, p2, p3, color, thickness);
  svl::drawLine(dst, p2, p4, color, thickness);
}

void svl::drawRect(cv::Mat& dst,
              const cv::Rect& r, const cv::Scalar & color, const int thickness)
{

  CvPoint p1;p1.x=r.x;p1.y= r.y;
  CvPoint p2;p2.x=r.x+r.width; p2.y=r.y;
  CvPoint p3;p3.x=p1.x;p3.y=r.y+r.height;
  CvPoint p4;p4.x=p2.x;p4.y=p3.y;

  svl::drawLine(dst, p1, p2, color, thickness);
  svl::drawLine(dst, p1, p3, color, thickness);
  svl::drawLine(dst, p3, p4, color, thickness);
  svl::drawLine(dst, p2, p4, color, thickness);
}



void svl::drawLine(cv::Mat& dst, const cv::Point& pos, float ori, float len, const cv::Scalar & color, const int thickness )
{

  int x1 = int(cos(ori)*len/2);
  int y1 = int(sin(ori)*len/2);

  CvPoint p1; p1.x=pos.x-x1; p1.y=pos.y+y1;
  CvPoint p2; p2.x=pos.x+x1; p2.y=pos.y-y1;

  svl::drawLine(dst, p1, p2, color, thickness);

}



void svl::drawLine(cv::Mat& dst, const cv::Point& p1, const cv::Point& p2,
              const cv::Scalar & color, const int thickness)
{
    cv::line ( dst, p1, p2, color, thickness);
}

