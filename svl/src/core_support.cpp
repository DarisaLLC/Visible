//
//  core_support.cpp
//  Visible
//
//  Created by Arman Garakani on 12/17/18.
//

#include <stdexcept>
#include <sys/stat.h>
#include <string>


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
