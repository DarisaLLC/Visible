
#ifndef __GEN_TIME_MARKER__
#define __GEN_TIME_MARKER__


#include "cinder/Cinder.h"
#include "time_index.h"




class marker_info : index_time_t
{
public:
    
    marker_info () { first = -1; second = 0.0; }
    
    marker_info (int64_t num_frames, double duration)
    {
        first = 0;
        second = time_spec_t (0.0);
        mEntire.first = num_frames;
        mEntire.second = duration;
    }
    
    marker_info (int64_t index, float time_in_seconds, int64_t num_frames, float duration)
    {
        first = index;
        second = time_spec_t (time_in_seconds);
        mEntire.first = num_frames;
        mEntire.second = duration;
    }

    inline double duration () const { return mEntire.second.secs(); }
    inline int64_t count () const { return mEntire.first; }
    
    inline int64_t current_frame () const { return first; }
    inline const time_spec_t& current_time_spec () const { return second; }
    
    
    void from_norm (double normed)
    {
        if (std::signbit(normed) || normed > 1.0 ) return;
        first = count() * normed;
        second = time_spec_t (duration() * normed);
    }
    
    void from_time (double new_secs)
    {
        double normed = new_secs / duration ();
        
        if (normed > 1.0) return;
        
        second = time_spec_t (new_secs);
        first = count() * normed;

    }
    
    void from_count (int64_t _cnt)
    {
        double normed = ((double)_cnt) / count();
        
        if (normed > 1.0) return;
        
        second = time_spec_t (normed * duration());
        first = _cnt;
        
    }
    
    double norm_time () const { return second.secs() / mEntire.second.secs(); }
    double norm_index () const { return first / (double) mEntire.first; }
    
    friend std::ostream& operator<< (std::ostream& std_stream, marker_info& t)
    {
        
        std_stream << t.first << "," << t.second.secs() << "[" << t.mEntire.first << "," << t.mEntire.second.secs() << "]";
        return std_stream;
    }
    
private:
    index_time_t mEntire;
    
};



#endif

