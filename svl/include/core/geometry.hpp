//
//  geometry.hpp
//  Visible
//
//  Created by Arman Garakani on 7/15/19.
//

#ifndef geometry_h
#define geometry_h
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <cmath>
#include <vector>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<float, 2, bg::cs::cartesian> point_2d;
typedef bg::model::box<point_2d> box_2d;
/* Defaults
 bool ClockWise = true,
 bool Closed = true,
 template<typename, typename> class PointList = std::vector,
 template<typename, typename> class RingList = std::vector,
 template<typename> class PointAlloc = std::allocator,
 template<typename> class RingAlloc = std::allocator

 */
typedef bg::model::polygon<point_2d> polygon_2d;
typedef boost::shared_ptr<polygon> shp;
typedef std::pair<box, shp> value;

class labeledField_2d : 
class Polygon : polygon_2d
{
    
    /* ctor -
     Create a polygon for a circle of this radius or
     an ellipse with a, b
     * For our purposes, these are
     * considered to be both valid and convex.
     * Use translate to move it where you want it
     * @note should use rcCircle or rcEllipse
     */
    Polygon ();
  //  Polygon (const float , const int32);
 //   Polygon (const float , const float, const int32);
    Polygon (std::vector<point_2d>& new_vertices);
    
    // void clear() same as base class
    
    /**
     * @brief Returns all vertices of this polygon
     *
     * @return all vertices of this polygon
     */
    std::vector<point_2d> getVertices();
    
    /**
     * @brief Sets all the vertices of this polygon
     */
    void setVertices(std::vector<point_2d>);
    
    /**
     * @brief Returns the number of vertices in this polygon
     *
     * @return the number of vertices in this polygon
     */
    size_t getSize();
    
    /* centerOf - Returns the area circumscribed by this polygon. The
     * polygon must be valid. This is an O(N) operation.
     * centerOf is cached. All vertex operations as well as translate and transform
     * invalidate the center.
     */
    bool centerOf(point_2d& cent) const { centroid (*this, cent); }
    
    /* Area - Returns area this polygon. The
     * polygon must be valid. This is an O(N) operation.
     * Negative area indicates abnormal polygon.
     * @note bring back the const
     */
    double area() const { return area(*this); }
    
    /* perimeter - Returns the length of the polygon's perimeter. This
     * is an O(N) operation.
     * Note: The polygon need not be valid.
     */
    double perimeter() const  { return perimeter (*this); }
    
    /* Circularity - Returns "projection sphericity". It equals to 1 for
     * any disc and less than 1 for all other shapes. This is an O(N) operation.
     * Note: The polygon need not be valid.
     * Reference:Volodymyr Kindratenko (Simple Geometric Shapes) (Arman)
     */
    double circularity () const;
    
    /* elipseRatio - Returns ratio of long and short semi axis of an ellipse
     * with area and perimeter equal to this polygon.
     * This is an O(N) operation.
     * Note: The polygon need not be valid.
     * Reference:Volodymyr Kindratenko (Simple Geometric Shapes) (Arman)
     */
    
    
    /* contains - Check that the point is contained within this
     * polygon. The polygon must be valid. This is in O(N) operation.
     */
    bool contains(const point_2d& point) const { return within (point, *this); }
    
    /* contains - Check that the specified polygon is contained
     * within this polygon. Both polygons must be valid. If either
     * polygon is not convex then the algorithm is O(N**2). Otherwise, a
     * special convex intersection function is called that is O(N).
     */
    bool contains(const Polygon& polygon) const;
    
    /* intersects - Check that the specified polygons intersects. Both
     * polygons must be valid. If either polygon is not convex then the
     * algorithm is O(N**2). Otherwise, a special convex intersection
     * function is called that is O(N).
     *
     * The group overload return a vector of Polygon (copies) that intersect from a vector of polygons.
     * @note use a different design or make polygons a smart pointer class
     */
    bool intersects(const Polygon& polygon) const;
    
    /* operator== - Checks that the two polygons have exactly the same
     * points stored in the same relative order. So, { (0,0) (1,1) (2,2) }
     * matches { (2,2) (0,0) (1,1) }, but not { (0,0) (2,2) (1,1) }
     */
    friend bool operator==(const Polygon& a, const Polygon& b);
    friend bool operator!=(const Polygon& a, const Polygon& b);
    
private:
    
};

#endif /* geometry_h */
