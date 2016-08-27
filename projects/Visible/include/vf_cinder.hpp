#ifndef __QTIME_VC_IMPL__
#define __QTIME_VC_IMPL__

#include <cinder/Channel.h>
#include <cinder/Rect.h>
#include "vf_types.h"
#include "simple_pair.h"
#include <fstream>
#include <mutex>
#include <memory>
#include <functional>

using namespace ci;
using namespace std;



class rcRect : public ci::RectT<float>
{
public:
    
    rcRect () : ci::RectT<float> () {}
    
    rcRect( int32 x, int32 y, int32 width, int32 height )
    : ci::RectT<float> ((float) x, (float)y, (float) width, (float) height) {}
    
    rcRect( const ci::RectT<float>& other)
    : ci::RectT<float> (other) {}
    
    // Accessors
    int32 x()  const { return getX1(); }
    int32 y()  const { return getY1(); }
    
    int32 width () const { return getWidth(); }
    int32 height () const { return getHeight(); }
    
    
    bool contains( const rcRect &rect ) const
    {
        return (rect.x1 >= x1) &&
        (rect.x2 <= x2) &&
        (rect.y1 >= y1) &&
        (rect.y2 <= y2);
    }
    
    
    bool contains( const std::pair<int32,int32>& pt ) const
    {
        return (pt.first >= x1) &&
        (pt.first <= x2) &&
        (pt.second >= y1) &&
        (pt.second <= y2);
    }
    
    rcRect intersect ( const rcRect &rect ) const
    {
        ci::RectT<float> result;
        result.x1 = std::max(x1, rect.x1);
        result.y1 = std::max(y1, rect.y1);
        result.x2 = std::min (x2, rect.x2);
        result.y2 = std::min(y2, rect.y2);
        result.canonicalize();
        return result;
    }
    
    rcRect operator &= ( const rcRect &rect )
    {
        x1 = std::max(x1, rect.x1);
        y1 = std::max(y1, rect.y1);
        x2 = std::min (x2, rect.x2);
        y2 = std::min(y2, rect.y2);
        return *this;
    }

    inline rcRect operator& (const rcRect& other) const
    {
        return intersect(other);
    }
    
    inline rcRect operator| (const rcRect& other) const
    {
        rcRect result( *this );
        result.include (other);
        return result;
    }

    bool operator==(const rcRect & rhs) const
    {
        return x1 == rhs.x1 and x2 == rhs.x2 and y1 == rhs.y1 and y2 == rhs.y2;
    }

    bool operator!=(const rcRect & rhs) const
    {
        return ! (rhs == *this);
    }


};


#endif // __QTIME_VC_IMPL__
