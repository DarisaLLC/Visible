#ifndef __TimeIndex__
#define __TimeIndex__

#include "timestamp.h"

using namespace std;

typedef std::pair<int64_t, time_spec_t> index_time_t;
typedef std::pair<index_time_t, index_time_t> length_time_t;

#endif 
