#include "core/lineseg.hpp"
#include "core/vector2d.hpp"
#include <cmath>

fLineSegment2d::fLineSegment2d()
  : mValid(false)
{}

// ######################################################################
fLineSegment2d::fLineSegment2d(const fVector_2d& p1,
                             const fVector_2d& p2)
  : mPoint1(p1), mPoint2(p2), mValid(p1 != p2)
{}


// ######################################################################
const fVector_2d& fLineSegment2d::point1() const
{
 if(!isValid ()) throw invalid_segment(); 
  return (mPoint1);
}

// ######################################################################
const fVector_2d& fLineSegment2d::point2() const
{
 if(!isValid ()) throw invalid_segment();
  return (mPoint2);
}

// ######################################################################
uRadian fLineSegment2d::angle() const
{
 if(!isValid ()) throw invalid_segment();
    return (mPoint1-mPoint2).angle();
}

// ######################################################################
uRadian fLineSegment2d::angleBetween(fLineSegment2d &line) const
{
    if (! isValid() || ! line.isValid())
        throw invalid_segment();
    
	fVector_2d d = point1() - point2();
  return uRadian (atan(d.y()/d.x()));
}

// ######################################################################
float fLineSegment2d::length() const
{
  return (float)(point1().distance(point2()));
}
// ######################################################################
bool fLineSegment2d::isValid() const
{ return mValid; }


// ######################################################################
bool fLineSegment2d::intersects(fLineSegment2d &line, double &xcoord, double &ycoord) const
{

  double Ax = point1().x();
  double Ay = point1().y();
  double Bx = point2().x();
  double By = point2().y();
  double Cx = line.point1().x();
  double Cy = line.point1().y();
  double Dx = line.point2().x();
  double Dy = line.point2().y();


  double  x, y, distAB, theCos, theSin, newX, ABpos ;

  //  Fail if either line is undefined.
  if ((Ax==Bx && Ay==By) || (Cx==Dx && Cy==Dy)) { return false; }

  //  (1) Translate the system so that point A is on the origin.
  Bx-=Ax; By-=Ay;
  Cx-=Ax; Cy-=Ay;
  Dx-=Ax; Dy-=Ay;

  //  Discover the length of segment A-B.
  distAB=sqrt(Bx*Bx+By*By);

  //  (2) Rotate the system so that point B is on the positive X axis.
  theCos=Bx/distAB;
  theSin=By/distAB;
  newX=Cx*theCos+Cy*theSin;
  Cy  =Cy*theCos-Cx*theSin; Cx=newX;
  newX=Dx*theCos+Dy*theSin;
  Dy  =Dy*theCos-Dx*theSin; Dx=newX;

  //  Fail if the lines are parallel.
  if (Cy==Dy) {
    if (distance(line.point1()) && distance(line.point2()) && line.distance(point1()) && line.distance(point2())) {
      return false;
    }
  }

  //  (3) Discover the position of the intersection point along line
  //  A-B.
  ABpos=Dx+(Cx-Dx)*Dy/(Dy-Cy);

  //  (4) Apply the discovered position to line A-B in the original
  //  coordinate system.
  x=Ax+ABpos*theCos;
  y=Ay+ABpos*theSin;

  //  Success.

  //in case the two lines share only an endpoint
  if (point1() == line.point1() || point1() == line.point2()) {
    x = point1().x();
    y = point1().y();
  }
  if (point2() == line.point1() || point2() == line.point2()) {
    x = point2().x();
    y = point2().y();
  }

  xcoord = x;
  ycoord = y;

	if (!std::isfinite (x) || !std::isfinite (y) ||
      distance(fVector_2d(x, y)) > 0.000010 ||
      line.distance(fVector_2d(x, y)) > 0.000010  ) {

    return false;
  }

  return true;


}

double fLineSegment2d::distance(const fVector_2d& point) const
{
  const fVector_2d& A = mPoint1;
  const fVector_2d& B = mPoint2;

  const fVector_2d& C(point);

  double dist = ((B-A).cross(C-A)) / sqrt((B-A)*(B-A));
  double dot1 = (C-B)*(B-A);
  if(dot1 > 0)return sqrt((B-C)*(B-C));
  double dot2 = (C-A)*(A-B);
  if(dot2 > 0)return sqrt((A-C)*(A-C));
  return fabs(dist);
}


fLineSegment2d fLineSegment2d::average (const fLineSegment2d& line) const
{
  const fVector_2d A = (mPoint1 + line.point1()) / 2.0;
  const fVector_2d B = (mPoint2 + line.point2()) / 2.0;
  return fLineSegment2d (A, B);
  }


bool fLineSegment2d::operator==(const fLineSegment2d& ls) const
      { return mId == ls.mId && mPoint1 == ls.mPoint1 && mPoint2 == ls.mPoint2 &&
			mValid == ls.mValid ; }

bool fLineSegment2d::operator!=(const fLineSegment2d& ls) const
     { return !(*this == ls); }

