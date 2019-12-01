//
//  input_selector.hpp
//  Visible
//
//  Created by Arman Garakani on 9/18/19.
//

#ifndef input_selector_h
#define input_selector_h

#include <iostream>
using namespace std;

// channel_index which channel of multi-channel input. Usually visible channel is the last one
// input is -1 for the entire root or index of moving object area in results container
// TBD: Add a connector between this information and the input source data whether it is a LIF
// file or a movie file.

class region_and_source{
public:
    
    region_and_source ():m_region(ni()),m_channel(ni()) {}
    region_and_source (int r, int channel):m_region(r), m_channel(channel) {}
    inline int entire () { return -1; }
    inline int ni () { return -2; }
    bool isChannel() const {return m_channel >= 0;}
    bool isEntire() const {return m_region == -1; }
    int region () const { return m_region;}
    int channel () const { return m_channel; }
    
    friend ostream& operator<<(ostream& out,region_and_source& rs){
         out << (rs.isEntire() ? " Entire " : " ROI ") << std::endl;
        out << " Region Index: " << rs.region () << std::endl;
        out << " Channel Index: " << rs.channel () << std::endl;
         return out;
    }
    
private:
    mutable int m_region;
    mutable int m_channel;
};

typedef region_and_source input_channel_selector_t;

#endif /* input_selector_h */
