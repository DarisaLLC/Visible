//
//  timestamp.h
//  Visible
//
//  Created by Arman Garakani on 5/12/16.
//
//

#ifndef timestamp_h
#define timestamp_h

#include <random>
#include <iterator>
#include <vector>
#include <list>
#include <queue>
#include <fstream>
#include <mutex>
#include <string>


#include <boost/operators.hpp>
#include <ctime>

#include <sys/param.h>
#include <mach-o/dyld.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "core/cm_time.hpp"

using namespace std;
using namespace boost;
using namespace boost::date_time;


#define HAVE_MICROSEC_CLOCK

class time_spec_t : boost::additive<time_spec_t>, boost::totally_ordered<time_spec_t>
{
public:
    
    static time_spec_t get_system_time(void);
    
    static int64_t ticks_per_second ()
    {
        return micro_res::to_tick_count(0,0,1,0);
    }
    
    
    
    time_spec_t(double secs = 0);
    
    time_spec_t(time_t full_secs, double frac_secs = 0);
    
    time_spec_t(time_t full_secs, long tick_count, double tick_rate);
    
    time_spec_t(const cm_time& cmt);
    
    long get_tick_count(double tick_rate) const
    {
        return this->get_frac_secs()*tick_rate; //boost::math::iround(this->get_frac_secs()*tick_rate);
    }
    
    double secs(void) const
    {
        return this->_full_secs + this->_frac_secs;
    }
    
    unsigned int milliseconds(void) const
    {
        return (this->_full_secs + this->_frac_secs) * 1000;
    }
    
    time_t get_full_secs(void) const
    {
        return this->_full_secs;
    }
    
    double get_frac_secs(void) const
    {
        return this->_frac_secs;
    }
    
    // Support totally_ordered
    friend bool operator<  (time_spec_t const& lhs, time_spec_t const& rhs)
    {
        return (
                (lhs.get_full_secs() < rhs.get_full_secs()) or ((lhs.get_full_secs() == rhs.get_full_secs()) and
                                                                (lhs.get_frac_secs() < rhs.get_frac_secs()) ));
        
    }
    
    friend bool operator== (time_spec_t const& t1, time_spec_t const& t2)
    {
        return t1.get_full_secs() == t2.get_full_secs() and t1.get_frac_secs() == t2.get_frac_secs();
    }
    
    time_spec_t &operator+=(const time_spec_t & rhs);
    
    
    time_spec_t &operator-=(const time_spec_t & rhs);
    
    
    
    
    //private time storage details
private: time_t _full_secs; double _frac_secs;
};



#endif /* timestamp_h */
