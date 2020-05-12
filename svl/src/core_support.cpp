//
//  core_support.cpp
//  Visible
//
//  Created by Arman Garakani on 12/17/18.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"


#include <stdexcept>
#include <sys/stat.h>
#include <string>
#include "core/stl_utils.hpp"


#include <cstddef>
#include <cstring>
#include <ctime>
#include <locale>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>


namespace svl // protection from unintended ADL
{
    namespace io{
        std::pair<bool,uint64_t> check_file (const std::string &filename){
            
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define STAT_IDENTIFIER stat //for MacOS X and other Unix-like systems
#elif defined(_WIN32)
#define STAT_IDENTIFIER _stat //for Windows systems, both 32bit and 64bit?
#endif
            auto ret = std::make_pair(true,uint64_t(0));
            
            struct STAT_IDENTIFIER st;
            ret.first = STAT_IDENTIFIER(filename.c_str(), &st) != -1;
            ret.second = st.st_size;
            return ret;
        }
        
    }
}

namespace stl_utils{
    
    
    uint64_t utc_now_in_seconds()
    {
        boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
        return ptime_in_seconds(now);
    }
    
    uint64_t ptime_in_seconds(const boost::posix_time::ptime &time)
    {
        static const boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
        boost::posix_time::time_duration seconds_since_epoch = time - epoch;
        return seconds_since_epoch.total_seconds();
    }
    
    boost::posix_time::ptime now()
    {
        return boost::posix_time::second_clock::universal_time();
    }
    
    // Convenient now time in text
    std::string now_string (){
        boost::posix_time::ptime _now = now();
        return boost::lexical_cast<std::string>(ptime_in_seconds(_now));
    }
    
}

#pragma GCC diagnostic pop
