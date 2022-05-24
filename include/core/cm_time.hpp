#ifndef __CM_TIMER_CPP__
#define __CM_TIMER_CPP__

#include <CoreMedia/CMTime.h>
#include <bitset>
#include <iostream>

using namespace std;

class cm_time : public CMTime
{
public:
    cm_time () { *this = kCMTimeZero; }
    cm_time (const CMTime& other)
    {
        value = other.value;
        timescale = other.timescale;
        epoch = other.epoch;
        flags = other.flags;
    }
    
    cm_time& operator=(const CMTime& other)
    {
        value = other.value;
        timescale = other.timescale;
        epoch = other.epoch;
        flags = other.flags;
        return *this;
    }
    
    cm_time (double seconds, int32_t preferredTimeScale = 60000)
    {
        *this = CMTimeMakeWithSeconds (seconds, preferredTimeScale);
    }
    
    cm_time operator+(const CMTime& rhs)
    {
        return CMTimeAdd(*this, rhs);
    }
    
    cm_time operator-(const CMTime& rhs)
    {
        return CMTimeSubtract(*this, rhs);
    }
    
    
    int64_t getValue () const { return value; }
    int32_t getScale () const { return timescale; }
    int64_t getEpoch () const { return epoch; }
    
    explicit operator double () const { return CMTimeGetSeconds(*this); }
    explicit operator float () const { return (float) CMTimeGetSeconds(*this); }
    
    bool isValid () const
    {
        return CMTIME_IS_VALID(*this);
    }
    
    bool isRounded () const
    {
        return CMTIME_HAS_BEEN_ROUNDED(*this);
    }
    bool isPositiveInfinity () const
    {
        return CMTIME_IS_POSITIVE_INFINITY(*this);
    }
    bool isNegativeInfinity () const
    {
        return CMTIME_IS_NEGATIVE_INFINITY(*this);
    }
    bool isIndefinite () const
    {
        return CMTIME_IS_INDEFINITE(*this);
    }
    
    bool isNumeric () const
    {
        return CMTIME_IS_NUMERIC(*this);
    }
    
    bool isZero () const
    {
        static const cm_time _cm_0;
        return CMTimeCompare(*this, _cm_0) == 0;
    }
    
    
    bool operator==(const cm_time& rhs) const
    {
        return CMTimeCompare(*this, rhs) == 0;
    }
    
    bool operator<(const cm_time& rhs) const
    {
        return CMTimeCompare(*this, rhs) == -1;
    }
    
    bool operator>(const cm_time& rhs) const
    {
        return CMTimeCompare(*this, rhs) == 1;
    }
    
    void show () const
    {
        CMTimeShow(*this);
    }
    
    //-----------
    // Stream I/O
    //-----------
    
    friend std::ostream &	operator << (ostream &os,  const cm_time&  h)
    {
        os << "{" << h.value << "/" << h.timescale << " = " << (float) h << "}" << std::endl;
        return os;
    }
    
};




#endif // 
