
#ifndef __GEN_TIME_MARKER__
#define __GEN_TIME_MARKER__


#include "cinder/Cinder.h"
#include "time_index.h"




class marker_info : public index_time_t
{
public:
    enum event_type { move = 0, down = move+1, num_types = down+1 };
    
    marker_info () { first = -1; second = 0.0; }
    
    marker_info (int64_t index, float time_in_seconds, int64_t num_frames)
    {
        first = index;
        second = time_spec_t (time_in_seconds);
        count = num_frames;
    }
    
    cinder::vec2 norm_pos;
    int64_t count;
    event_type et;
    
    friend std::ostream& operator<< (std::ostream& std_stream, marker_info& t)
    {
        
        std_stream << "Normalized Position:" << t.norm_pos.x << " x " << t.norm_pos.y << std::endl;
        std_stream << "Event:    " << ((t.et == event_type::move) ? "move" : "down") << std::endl;
        std_stream << "Index:    " << t.first << std::endl;
        return std_stream;
    }
    
};



#endif

