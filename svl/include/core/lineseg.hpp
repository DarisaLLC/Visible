#ifndef _LINE_H_
#define _LINE_H_

#include <vector>
#include "angle_units.h"
#include "vector2d.hpp"
#include <exception>


class fLineSegment2d
{
public:
    
    class invalid_segment : public std::exception {
    public:
        virtual const char* what() const throw () {
            return "Invalid Segment";
        }
    };
    
    fLineSegment2d();
		
    //! construct from two bf2dVector<int>
    /*!@param p1 an endpoint on the line
      @param p2 the second endpoint of the line line*/
    fLineSegment2d(const fVector_2d & p1, const fVector_2d & p2);

    //! get a point on the line
    /*!@param n get point point + n * direction; for n = 0 (default),
      this is the point entered in the constructor or in reset*/
    const fVector_2d& point1() const;
		
    //! returns the direction vector for this straight line
    const fVector_2d& point2() const;
		
    //!returns the angle of the line
    uRadian angle() const;
		
    uRadian angleBetween(fLineSegment2d &line) const;
		
    bool intersects(fLineSegment2d &line, double &xcoord, double &ycoord) const;
		
    double distance(const fVector_2d& point) const ;
	
    fLineSegment2d average (const fLineSegment2d&) const;
		
    float length() const;
		
    //! whether this object is valid
    bool isValid() const;
	
		void id (int mid) { mId = mid; }
		int id () const { return mId; }
    bool operator==(const fLineSegment2d& ls) const;

    bool operator!=(const fLineSegment2d& ls) const;
		
private:
    fVector_2d mPoint1;
    fVector_2d mPoint2;
    bool mValid;
    int mId;
};



#endif /* _bfLINE_H_ */
