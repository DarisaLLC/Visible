//
//  labelconnect.hpp
//  patterns
//
//  Created by Arman Garakani on 10/14/15.
//  Copyright © 2015 Arman Garakani. All rights reserved.
//

#ifndef labelconnect_hpp
#define labelconnect_hpp

#include <stdio.h>
#include <map>

#include "roiWindow.h"
#include "core/rectangle.h"

using namespace svl;

template<typename P>
class labelConnect
{
public:
    typedef P32S::value_type bin_type;
    typedef std::map<bin_type,iRect> label_bbox_map_t;
    labelConnect (const roiWindow<P>& src);
    bool run ();
    const roiWindow<P32S>& label () const;
    uint32_t regions () const;
    const label_bbox_map_t& label_bbox_map () const;
private:
    
    void get_components ();
    void get_rects ();
    roiWindow<P> src_shallow_;
    roiWindow<P32S> buffer_;
    roiWindow<P32S> label_;
    std::vector<iRect> rois_;
    std::vector<bin_type> amm_;
    bin_type regions_;
    bin_type icomponent_;
    label_bbox_map_t label2roi_;
    void add_to_associative (bin_type, bin_type);
};

//
//  Run Length Encoding of Mask Areas.
//
//
//
//
//

template<typename P>
class simpleRLE
{
public:
    typedef typename PixelType<P>::pixel_ptr_t pixel_ptr_t;
    typedef typename PixelType<P>::pixel_t pixel_t;
    typedef typename PixelType<P>::run_t run_t;

    // Rle all
    simpleRLE(const roiWindow<P>& src)
    {
        rat_.resize (0);
        for (auto row = 0; row < src.height(); row++)
        {
            std::vector<run_t> rles;
            rle_row ((uint8_t*) src.rowPointer(row), src.width(), rles);
            rat_.emplace_back(rles);
        }
    }
    
    // run_vector_t& pointToRow(int32_t y) const {return rat_[y];}
    
    int rle_row (pixel_ptr_t input, int length, std::vector<std::pair<pixel_t, uint16_t> >& output)
    {
        static const uint32_t max_run = numeric_limits<uint16_t>::max();
        
        int count = 0,index;
        pixel_t pixel;
        output.resize(0);
        int out = 0;
        
        while (count < length)
        {
            index = count;
            pixel = input[index++];
            while (index < length && index - count < max_run && input[index] == pixel)
                index++;
            output.emplace_back (pixel, (uint16_t)(index - count));
            count=index;
            out++;
        } /* while */
        return(out);
    }
    
    
    
private:
    mutable std::vector<std::vector<std::pair<pixel_t, uint16_t> > > rat_;

    
    };
    

    
#endif /* labelconnect_hpp */
