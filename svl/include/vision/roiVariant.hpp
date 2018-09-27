
#ifndef _VAROI_H_
#define _VAROI_H_


#include <iostream>
#include <cstring>
#include <iomanip>
#include <fstream>
#include "roiWindow.h"
#include "boost/variant.hpp"
#include <vector>

using namespace std;
using boost::variant;
using boost::apply_visitor;

namespace svl
{
    
    typedef variant<roiWindow_P8U_t,roiWindow_P8UC3_t,roiWindow_P8UC4_t> roiVP8UC;
    
    template<typename P> static uint32_t vroIndex ();
#if defined(USED)
    template <> uint32_t vroIndex<roiWindow_P8U_t> () { return 0; }
    template <> uint32_t vroIndex<roiWindow_P8UC3_t> () { return 1; }
    template <> uint32_t vroIndex<roiWindow_P8UC4_t> () { return 2; }
#endif
    
    
    class roiVP8UC_Visitor: public boost::static_visitor<void>
    { public: };
 
    class isBoundVisitor : public boost::static_visitor<bool>
    { // A visitor with function object 'look-alike'
    public:
        bool operator () (roiWindow<P8U>& r) const { return r.isBound() ; }
        bool operator () (roiWindow<P8UC3>& r) const { return r.isBound() ; }
        bool operator () (roiWindow<P8UC4>& r) const { return r.isBound() ; }
    };
    
    bool vroiIsBound ( roiVP8UC& vr);
    
    class channelVisitor : public boost::static_visitor<int32_t>
    { // A visitor with function object 'look-alike'
    public:
        int32_t operator () (roiWindow<P8U>& r) const { return r.components() ; }
        int32_t operator () (roiWindow<P8UC3>& r) const { return r.components() ; }
        int32_t operator () (roiWindow<P8UC4>& r) const { return r.components() ; }
    };
    
    int32_t vroiChannels ( roiVP8UC& vr);
    
    class sizeVisitor : public boost::static_visitor<iPair>
    { // A visitor with function object 'look-alike'
    public:
        iPair operator () (roiWindow<P8U>& r) const { return r.size() ; }
        iPair operator () (roiWindow<P8UC3>& r) const { return r.size() ; }
        iPair operator () (roiWindow<P8UC4>& r) const { return r.size() ; }
    };
    
    iPair vroiSize (roiVP8UC& vr);
    
    
}

#endif //
