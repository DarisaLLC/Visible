//
//  contraction_geometry.hpp
//  Visible
//
//  Created by Arman Garakani on 7/22/19.
//

#ifndef contraction_geometry_h
#define contraction_geometry_h


#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

#include <boost/geometry/index/rtree.hpp>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/transformed.hpp>



namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 2, bg::cs::cartesian> point2d_t;
typedef bg::model::box<point2d_t> box2d_t;
typedef std::vector<point2d_t> cluster2d_t;

namespace cgeom{
    // used in the rtree constructor with Boost.Range adaptors
    // to generate std::pair<point_t, std::size_t> from point_t on the fly
    template <typename First, typename Second>
    struct pair_generator
    {
        typedef std::pair<First, Second> result_type;
        template<typename T>
        inline result_type operator()(T const& v) const
        {
            return result_type(v.value(), v.index());
        }
    };

    // used to hold point-related information during clustering
    struct point_data
    {
        point_data() : used(false) {}
        bool used;
    };

    typedef bg::model::point<double, 3, bg::cs::cartesian> point3d_t;
    typedef bg::model::box<point3d_t> box3d_t;
    typedef std::vector<point3d_t> cluster3d_t;
    
    // find clusters of points using cluster radius r
    void find_2dclusters(std::vector<point2d_t> const& points,
                         double r,
                         std::vector<cluster2d_t> & clusters)
    {
        typedef std::pair<point2d_t, std::size_t> value_t;
        typedef pair_generator<point2d_t, std::size_t> value_generator;
        
        if (r < 0.0)
            return; // or return error
        
        // create rtree holding std::pair<point_t, std::size_t>
        // from container of points of type point_t
        bgi::rtree<value_t, bgi::rstar<4> >
        rtree(points | boost::adaptors::indexed()
              | boost::adaptors::transformed(value_generator()));
        
        // create container holding point states
        std::vector<point_data> points_data(rtree.size());
        
        // for all pairs contained in the rtree
        for(auto const& v : rtree)
        {
            // ignore points that were used before
            if (points_data[v.second].used)
                continue;
            
            // current point
            point2d_t const& p = v.first;
            double x = bg::get<0>(p);
            double y = bg::get<1>(p);
            
            // find all points in circle of radius r around current point
            std::vector<value_t> res;
            rtree.query(
                        // return points that are in a box enclosing the circle
                        bgi::intersects(box2d_t{{x-r, y-r},{x+r, y+r}})
                        // and were not used before
                        // and are indeed in the circle
                        && bgi::satisfies([&](value_t const& v){
                return points_data[v.second].used == false
                && bg::distance(p, v.first) <= r;
            }),
                        std::back_inserter(res));
            
            // create new cluster
            clusters.push_back(cluster2d_t());
            // add points to this cluster and mark them as used
            for(auto const& v : res) {
                clusters.back().push_back(v.first);
                points_data[v.second].used = true;
            }
        }
    }
    
    
    
    // find clusters of points using cluster radius r
    void find_clusters(std::vector<point3d_t> const& points,
                       double r,
                       std::vector<cluster3d_t> & clusters)
    {
        typedef std::pair<point3d_t, std::size_t> value3d_t;
        typedef pair_generator<point3d_t, std::size_t> value_generator;
        
        if (r < 0.0)
            return; // or return error
        
        // create rtree holding std::pair<point_t, std::size_t>
        // from container of points of type point_t
        bgi::rtree<value3d_t, bgi::rstar<4> >
        rtree(points | boost::adaptors::indexed()
              | boost::adaptors::transformed(value_generator()));
        
        // create container holding point states
        std::vector<point_data> points_data(rtree.size());
        
        // for all pairs contained in the rtree
        for(auto const& v : rtree)
        {
            // ignore points that were used before
            if (points_data[v.second].used)
                continue;
            
            // current point
            point3d_t const& p = v.first;
            double x = bg::get<0>(p);
            double y = bg::get<1>(p);
            double z = bg::get<2>(p);
            int iz = (int)(z*10);
            
            // find all points in circle of radius r around current point
            std::vector<value3d_t> res;
            rtree.query(
                        // return points that are in a box enclosing the circle
                        bgi::intersects(box3d_t{{x-r, y-r,z-0.1},{x+r, y+r,z+0.1}})
                        // and were not used before
                        // and are indeed in the circle
                        && bgi::satisfies([&](value3d_t const& v){
                return points_data[v.second].used == false
                && bg::distance(p, v.first) <= r && iz > 0 ;
            }),
                        std::back_inserter(res));
            
            // create new cluster
            clusters.push_back(cluster3d_t());
            // add points to this cluster and mark them as used
            for(auto const& v : res) {
                clusters.back().push_back(v.first);
                points_data[v.second].used = true;
            }
        }
    }


}

#endif /* contraction_geometry_h */
