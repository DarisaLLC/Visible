#ifndef __TimeIndex__
#define __TimeIndex__

#include "core/timestamp.h"
#include <map>

using namespace std;

typedef std::pair<int64_t, time_spec_t> index_time_t;
typedef std::pair<index_time_t, index_time_t> duration_time_t;
typedef std::map<time_spec_t, int64_t> timeToIndex_t;
typedef std::map<int64_t, time_spec_t> indexToTime_t;
#endif 
