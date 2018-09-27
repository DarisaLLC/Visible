//
//  clipManager.hpp
//  Visible
//
//  Created by Arman Garakani on 9/9/18.
//

#ifndef clipManager_h
#define clipManager_h

#include <stdio.h>
#include <algorithm>
#include <deque>
#include <vector>
#include <memory>
#include <typeinfo>
#include <string>
#include <tuple>
#include <chrono>
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "core/signaler.h"

using namespace std;
using namespace boost;
using namespace svl;
using namespace stl_utils;

class clip : public std::array<uint32_t,3>
{
public:
    typedef uint32_t element_t;
    clip (uint32_t count) : std::array<uint32_t,3> () {
        init (0,count-1,0);
    }
    
    clip (const size_t s, const size_t e, const size_t a){
        init (s, e, a);
    }
    
    const element_t& first () const { return at(0); }
    const element_t& last () const { return at(1); }
    const element_t& anchor () const { return at(2); }
    
    bool contains (const element_t pt) const{
        return pt >= first() && pt <= last();
    }
    bool isAfter (const element_t pt) const{
        return pt > last();
    }
    bool isBefore (const element_t pt) const{
        return pt < first ();
    }
    
    friend std::ostream& operator<< (std::ostream& out, const clip& se)
    {
        out << se.to_string() << std::endl;
        return out;
    }
    
    std::string to_string () const {
        auto msg =  "[ " + tostr(begin()) + " : " + tostr(end()) + " ] ( " + tostr(anchor());
        return msg;
    }
    
private:
    void init(const size_t s, const size_t e, const size_t a){
        at(0) = uint32_t(s), at(1) = uint32_t(e), at(2) = uint32_t(a);
    }
};

#if 0
typedef std::shared_ptr<class clipManger> clipManagerRef;

// Default logger factory-  creates synchronous loggers
struct clipManager_factory
{
    static clipManagerRef create()
    {
        return std::make_shared<class clipManager>();
    }
};

using default_factory = synchronous_factory;


class clip_signaler : public base_signaler
{
    virtual std::string
    getName () const { return "clipSignaler"; }
};


class clip_manager : public clip_signaler, std::enable_shared_from_this<clip_manager>
{
public:
    clip_manager ( ci::Timeline &timeline ) : mTimeline(timeline)
    
    static std::shared_ptr<contraction_analyzer> create();
    
    
private:
    ci::Timeline    &mTimeline;
};
#endif


    
#endif /* clipManager_h */
