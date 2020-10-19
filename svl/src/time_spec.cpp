
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"



#include "core/timestamp.h"
#include "boost/math/special_functions/round.hpp"


/***********************************************************************
 * Time spec system time
 **********************************************************************/
//typedef boost::uint64_t imaxdiv_t;
typedef struct {
    intmax_t quot;
    intmax_t rem;
 } imaxdiv_t;

//static imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom)
//   {
//   imaxdiv_t i;
//   i.quot = numer / denom;
//   i.rem = numer % denom;
//   return i;
//   }

/*!
 * Creates a time spec from system counts:
 * TODO make part of API as a static factory function
 * The counts type is 64 bits and will overflow the ticks type of long.
 * Therefore, divmod the counts into seconds + sub-second counts first.
 */

//static time_spec_t time_spec_t_from_counts(intmax_t counts, intmax_t freq){
//    imaxdiv_t divres = imaxdiv(counts, freq);
//    return time_spec_t(time_t(divres.quot), double(divres.rem)/freq);
//}

#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
time_spec_t time_spec_t::get_system_time(void){
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return time_spec_t(ts.tv_sec, ts.tv_nsec, 1e9);
}
#endif /* HAVE_CLOCK_GETTIME */


#ifdef HAVE_MACH_ABSOLUTE_TIME
#include <mach/mach_time.h>
time_spec_t time_spec_t::get_system_time(void){
    mach_timebase_info_data_t info; mach_timebase_info(&info);
    intmax_t nanosecs = mach_absolute_time()*info.numer/info.denom;
    return time_spec_t_from_counts(nanosecs, intmax_t(1e9));
}
#endif /* HAVE_MACH_ABSOLUTE_TIME */


#ifdef HAVE_QUERY_PERFORMANCE_COUNTER
#include <Windows.h>
time_spec_t time_spec_t::get_system_time(void){
    LARGE_INTEGER counts, freq;
    QueryPerformanceCounter(&counts);
    QueryPerformanceFrequency(&freq);
    return time_spec_t_from_counts(counts.QuadPart, freq.QuadPart);
}
#endif /* HAVE_QUERY_PERFORMANCE_COUNTER */


#ifdef HAVE_MICROSEC_CLOCK
#include <boost/date_time/posix_time/posix_time.hpp>
namespace pt = boost::posix_time;
time_spec_t time_spec_t::get_system_time(void){
    pt::ptime time_now = pt::microsec_clock::universal_time();
    pt::time_duration time_dur = time_now - pt::from_time_t(0);
    return time_spec_t(
        time_t(time_dur.total_seconds()),
        long(time_dur.fractional_seconds()),
        double(pt::time_duration::ticks_per_second())
    );
}
#endif /* HAVE_MICROSEC_CLOCK */

/***********************************************************************
 * Time spec constructors
 **********************************************************************/
#define time_spec_init(full, frac) { \
    _full_secs = full + time_t(frac); \
    _frac_secs = frac - time_t(frac); \
    if (_frac_secs < 0) {\
        _full_secs -= 1; \
        _frac_secs += 1; \
    } \
}

time_spec_t::time_spec_t(double secs){
    time_spec_init(0, secs);
}

time_spec_t::time_spec_t(time_t full_secs, double frac_secs){
    time_spec_init(full_secs, frac_secs);
}

time_spec_t::time_spec_t(time_t full_secs, long tick_count, double tick_rate){
    const double frac_secs = tick_count/tick_rate;
    time_spec_init(full_secs, frac_secs);
}

//time_spec_t::time_spec_t(const cm_time& cmt)
//{
//    *this = time_spec_t_from_counts(cmt.getValue(),(intmax_t)cmt.getScale());
//}
/***********************************************************************
 * Time spec math overloads
 **********************************************************************/
time_spec_t &time_spec_t::operator+=(const time_spec_t &rhs){
    time_spec_init(
        this->_full_secs + rhs.get_full_secs(),
        this->_frac_secs + rhs.get_frac_secs()
    );
    return *this;
}

time_spec_t &time_spec_t::operator-=(const time_spec_t &rhs){
    time_spec_init(
        this->_full_secs - rhs.get_full_secs(),
        this->_frac_secs - rhs.get_frac_secs()
    );
    return *this;
}
#pragma GCC diagnostic pop

