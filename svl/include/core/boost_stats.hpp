//
//  boost_stats.hpp
//  Visible
//
//  Created by Arman Garakani on 9/18/19.
//

#ifndef boost_stats_h
#define boost_stats_h

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>

#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>

namespace svl {
    
    class boostStatistics
    {
        using right = boost::accumulators::right;
        const size_t cs_;
        typedef boost::accumulators::accumulator_set<double,
        boost::accumulators::stats<
        boost::accumulators::tag::median(boost::accumulators::with_p_square_quantile),
        boost::accumulators::tag::count,
        boost::accumulators::tag::min,
        boost::accumulators::tag::max,
        boost::accumulators::tag::tail_quantile<right>
        >
        > Acc;
        Acc stats;
        
    public:
        boostStatistics(size_t cs = 1000000) : cs_(cs), stats( boost::accumulators::tag::tail<right>::cache_size = cs_) {}
        
        void update(const double ela) {
            stats(ela);
        }
        double get_max() const {
            return boost::accumulators::max(stats);
        }
        double get_min() const {
            return boost::accumulators::min(stats);
        }
        double get_median() const {
            return boost::accumulators::median(stats);
        }
        size_t get_n() const {
            return boost::accumulators::count(stats);
        }
        void reset() {
            stats = Acc(boost::accumulators::tag::tail<right>::cache_size = cs_);
        }
        double get_quantile(double q) const {
            return boost::accumulators::quantile(stats, boost::accumulators::quantile_probability = q);
        }
    };
    
} // namespace svl

#endif /* boost_stats_h */
