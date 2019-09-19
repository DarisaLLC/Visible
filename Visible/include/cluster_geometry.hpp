//
//  cluster_geometry.hpp
//  Visible
//
//  Created by Arman Garakani on 7/22/19.
//

#ifndef cluster_geometry_h
#define cluster_geometry_h


#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)

#include <boost/geometry/index/rtree.hpp>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/transformed.hpp>





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

    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;
    
    typedef bg::model::point<float, 2, bg::cs::cartesian> point2f_t;
    typedef bg::model::box<point2f_t> box2f_t;
    typedef std::vector<point2f_t> cluster2f_t;
    
    typedef bg::model::point<float, 3, bg::cs::cartesian> point3f_t;
    typedef bg::model::box<point3f_t> box3f_t;
    typedef std::vector<point3f_t> cluster3f_t;
    

    // find clusters of points using cluster radius r
    void find_2dclusters(std::vector<point2f_t> const& points,
                         float r,
                         std::vector<cluster2f_t> & clusters)
    {
        typedef std::pair<point2f_t, std::size_t> value_t;
        typedef pair_generator<point2f_t, std::size_t> value_generator;
        
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
            point2f_t const& p = v.first;
            float x = bg::get<0>(p);
            float y = bg::get<1>(p);
            
            // find all points in circle of radius r around current point
            std::vector<value_t> res;
            rtree.query(
                        // return points that are in a box enclosing the circle
                        bgi::intersects(box2f_t{{x-r, y-r},{x+r, y+r}})
                        // and were not used before
                        // and are indeed in the circle
                        && bgi::satisfies([&](value_t const& v){
                return points_data[v.second].used == false
                && bg::distance(p, v.first) <= r;
            }),
                        std::back_inserter(res));
            
            // create new cluster
            clusters.push_back(cluster2f_t());
            // add points to this cluster and mark them as used
            for(auto const& v : res) {
                clusters.back().push_back(v.first);
                points_data[v.second].used = true;
            }
        }
    }
    
    
    
    // find clusters of points using cluster radius r
    void find_3dclusters(std::vector<point3f_t> const& points,
                       float r, float zr,
                       std::vector<cluster3f_t> & clusters)
    {
        typedef std::pair<point3f_t, std::size_t> value3f_t;
        typedef pair_generator<point3f_t, std::size_t> value_generator;
        
        if (r < 0.0)
            return; // or return error
        
        // create rtree holding std::pair<point_t, std::size_t>
        // from container of points of type point_t
        bgi::rtree<value3f_t, bgi::rstar<4> >
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
            point3f_t const& p = v.first;
            float x = bg::get<0>(p);
            float y = bg::get<1>(p);
            float z = bg::get<2>(p);
            int iz = (int)(z*10);
            
            // find all points in circle of radius r around current point
            std::vector<value3f_t> res;
            rtree.query(
                        // return points that are in a box enclosing the circle
                        bgi::intersects(box3f_t{{x-r, y-r,-0.05},{x+r, y+r,z+0.5}})
                        // and were not used before
                        // and are indeed in the circle
                        && bgi::satisfies([&](value3f_t const& v){
                return points_data[v.second].used == false
                && bg::distance(p, v.first) <= r && iz > 0 ;
            }),
                        std::back_inserter(res));
            
            // create new cluster
            clusters.push_back(cluster3f_t());
            // add points to this cluster and mark them as used
            for(auto const& v : res) {
                clusters.back().push_back(v.first);
                points_data[v.second].used = true;
            }
        }
    }


}

#endif /* contraction_geometry_h */
