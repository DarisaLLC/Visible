
#ifndef __GEN_TIME_MARKER__
#define __GEN_TIME_MARKER__


#include "cinder/Cinder.h"
#include "timed_types.h"


// Holds the duration and frame count
// Can be updated by either new time, new index, or normalized 

class timeIndexConverter : index_time_t
{
public:
    
    timeIndexConverter () { first = -1; second = 0.0; }
    
    // Convertor for starting beginning at zero
    timeIndexConverter (int64_t num_frames, double duration)
    {
        first = mStart.first = 0;
        second = mStart.second = time_spec_t (0.0);
        mEntire.first = num_frames;
        mEntire.second = duration;
    }
    
    timeIndexConverter (int64_t index, float time_in_seconds, int64_t num_frames, float duration)
    {
        first = mStart.first = index;
        second = mStart.second = time_spec_t (time_in_seconds);
        mEntire.first = num_frames;
        mEntire.second = duration;
    }

    inline double duration () const { return mEntire.second.secs(); }
    inline int64_t count () const { return mEntire.first; }
    
    inline int64_t start_frame () const { return mStart.first; }
    inline const time_spec_t& start_time_spec () const { return mStart.second; }
    
    inline int64_t current_frame () const { return first; }
    inline const time_spec_t& current_time_spec () const { return second; }
    
    void update (int64_t _cnt)
    {
        double normed = ((double)_cnt) / count();
        
        if (normed > 1.0) return;
        
        second = time_spec_t (normed * duration());
        first = _cnt;
        
    }
    
    double norm_current_time () const { return second.secs() / mEntire.second.secs(); }
    double norm_current_index () const { return first / (double) mEntire.first; }
    
    
    friend std::ostream& operator<< (std::ostream& std_stream, timeIndexConverter& t)
    {
        
        std_stream << t.first << "," << t.second.secs() << "[" << t.mEntire.first << "," << t.mEntire.second.secs() << "]";
        return std_stream;
    }
    
private:
    index_time_t mEntire;
    index_time_t mStart;

};



#endif

