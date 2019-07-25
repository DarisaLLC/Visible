//
//  convex_hull.hpp.h
//  Visible
//
//  Created by Arman Garakani on 7/17/19.
//

#ifndef convex_hull_hpp_h
#define convex_hull_hpp_h

#include <boost/geometry.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/numeric/ublas/vector.hpp>

namespace bg = boost::geometry;
namespace ublas = boost::numeric::ublas;

// Make vector from 2 points
template<typename Point>
ublas::vector<double> make_vector(const Point& a, const Point& b)
{
    ublas::vector<double> v(2);
    v[0] = bg::get<0>(b) - bg::get<0>(a);
    v[1] = bg::get<1>(b) - bg::get<1>(a);
    return v;
}

// Calculate cos from 2 vectors
double cos_from_vecs(const ublas::vector<double>& v1,
                     const ublas::vector<double>& v2) {
    return ublas::inner_prod(v1, v2)/(ublas::norm_2(v1)*ublas::norm_2(v2));
}

// Calculate cos from 3 points
template<typename Point>
double cos_from_dots(const Point& a, const Point& b, const Point& c) {
    return cos_from_vecs(make_vector(b,a), make_vector(b,c));
}

template<typename MultiPoint>
void convex_hull(const MultiPoint& in, MultiPoint& out) {
    
    typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
    point_t min_coordinate;
    
    bool flag = false;
    
    // TODO : Remove points which cannot be a part of a convex hull
    for (const auto &point : in) {
        if (!flag) {
            bg::set<0>(min_coordinate, bg::get<0>(point));
            bg::set<1>(min_coordinate, bg::get<1>(point));
            flag = true;
            continue;
        }
        if (bg::get<0>(point) < bg::get<0>(min_coordinate) ||
            (bg::get<0>(point) == bg::get<0>(min_coordinate) &&
             bg::get<1>(point) < bg::get<1>(min_coordinate))) {
                
                bg::set<0>(min_coordinate, bg::get<0>(point));
                bg::set<1>(min_coordinate, bg::get<1>(point));
            }
    }
    
    point_t B(bg::get<0>(min_coordinate), bg::get<1>(min_coordinate)-1.0);    // Before
    point_t S(bg::get<0>(min_coordinate), bg::get<1>(min_coordinate));    // Start
    point_t T;    // To
    
    bg::append(out, S);
    
    while(1) {
        double min_cos_value = 1.0;
        point_t p;
        for (const auto &point : in) {
            bg::set<0>(T, bg::get<0>(point));
            bg::set<1>(T, bg::get<1>(point));
            
            if (bg::get<0>(S)==bg::get<0>(T) && bg::get<1>(S)==bg::get<1>(T)) {
                continue;
            }
            
            double cos_value = cos_from_dots(B,S,T);
            
            if (cos_value < min_cos_value) {
                min_cos_value = cos_value;
                bg::set<0>(p, bg::get<0>(T));
                bg::set<1>(p, bg::get<1>(T));
            }
        }
        bg::append(out, p);
        if (bg::get<0>(p)==bg::get<0>(min_coordinate) &&
            bg::get<1>(p)==bg::get<1>(min_coordinate)) {
            // Break because Pi+1 == P0
            break;
        }
        
        // B = S
        bg::set<0>(B, bg::get<0>(S));
        bg::set<1>(B, bg::get<1>(S));
        
        // S = p
        bg::set<0>(S, bg::get<0>(p));
        bg::set<1>(S, bg::get<1>(p));
    }
}



#endif /* convex_hull_hpp_h */
