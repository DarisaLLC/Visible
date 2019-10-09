#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <vector>
#include "vision/pixel_traits.h"
#include "vision/roiRoot.h"
#include "vision/roiWindow.h"
#include "vision/roiMultiWindow.h"

#include <assert.h>
#include "core/stl_utils.hpp"
#include "core/timestamp.h"
#include "core/rand_support.h"
#include <random>
#include "vision/roiVariant.hpp"

using namespace svl;


using namespace stl_utils;

namespace
{
#if defined(USED)
    std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr)
    {
        return std::shared_ptr<std::ofstream>(ifstream_ptr, ofStreamDeleter());
    }
    std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename)
    {
        return make_shared_ofstream(new std::ofstream(filename, std::ofstream::out));
    }
#endif
    
    std::shared_ptr<std::ifstream> make_shared_ifstream(std::ifstream * ifstream_ptr)
    {
        return std::shared_ptr<std::ifstream>(ifstream_ptr, ifStreamDeleter());
    }
    
    std::shared_ptr<std::ifstream> make_shared_ifstream(std::string filename)
    {
        return make_shared_ifstream(new std::ifstream(filename, std::ifstream::in));
    }
    
}

bool svl::vroiIsBound ( roiVP8UC& vr)
{
    isBoundVisitor bv;
    return boost::apply_visitor(bv, vr);
}

int32_t svl::vroiChannels ( roiVP8UC& vr)
{
    channelVisitor iv;
    return boost::apply_visitor(iv, vr);
}

iPair svl::vroiSize (roiVP8UC& vr)
{
    sizeVisitor sv;
    return boost::apply_visitor(sv, vr);
}


template<typename P>
roiWindow<P>::roiWindow(std::vector<typename roiWindow<P>::pixel_t>& src){
    int width = static_cast<int>(src.size());
    m_frame_buf = sharedRoot_t (new root<P>(width, 1, image_memory_alignment_policy::align_first_row));
    m_bounds = iRect(width,1);
    pixel_ptr_t our_row = rowPointer(0);
    std::memcpy( our_row, src.data(), width);
}

template <typename P>
void roiWindow<P>::randomFill( uint32_t seed )
{
    typedef typename roiWindow<P>::pixel_t pixel_t;

    std::random_device rd;
    
    std::mt19937 _rng_generator(rd());
    
    for (int32_t j = 0; j < height(); j++) {
        pixel_ptr_t one = rowPointer (j);
        for (uint32_t i = 0; i < width(); ++i, ++one)
        {
            float fr = randFloat(0.0f, 1.0f);
            *one = (pixel_t)(fr * std::numeric_limits<pixel_t>::max());
        }
    }
        
}


template <typename P>
void root<P>::set(pixel_t val)
{
    std::memset((void *)this->alignedPixelData(), val, this->n());
}


template <typename P>
void roiWindow<P>::set(pixel_t val)
{
    for (int32_t j = 0; j < height(); j++)
    {
        pixel_ptr_t row = rowPointer(j);
        for(auto i = 0; i < width(); i++) *row++ = val;
    }
}

template <typename P>
std::pair<typename roiWindow<P>::pixel_t, typename roiWindow<P>::pixel_t> roiWindow<P>::range() const
{
    std::pair<pixel_t, pixel_t> extremes;
    extremes.first = std::numeric_limits<pixel_t>::max();
    extremes.second = std::numeric_limits<pixel_t>::min();
    for (int32_t j = 0; j < height(); j++)
    {
        pixel_ptr_t row = rowPointer(j);
        for (int32_t i = 0; i < width(); i++, row++)
        {
            pixel_t pval = *row;
            if (pval > extremes.second) extremes.second = pval;
            if (pval < extremes.first) extremes.first = pval;
        }
    }
    return extremes;
}

template <typename P>
std::pair<typename roiWindow<P>::pixel_t, iPair> roiWindow<P>::maxValueLocation() const
{
    iPair loc;
    pixel_t maxv = std::numeric_limits<pixel_t>::min();
    for (int32_t j = 0; j < height(); j++)
    {
        pixel_ptr_t row = rowPointer(j);
        for (int32_t i = 0; i < width(); i++, row++)
        {
            pixel_t pval = *row;
            if (pval > maxv)
            {
                maxv = pval;
                loc.first = i;
                loc.second = j;
            }
        }
    }
    return make_pair(maxv, loc);
}

// become an entire window to this framebuf
template <typename P>
void roiWindow<P>::entire()
{
    m_bounds = m_frame_buf->bounds();
    
}



template <typename P>
bool roiWindow<P>::step(const iPair & tp)
{
    iRect cp(m_bounds);
    cp.translate(tp);
    bool ok = m_frame_buf->bounds().contains(cp);
    if (!ok) return ok;
    m_bounds.translate(tp);
    return true;
}


// Set pixel value for (x,y). Return new value
template <typename P>
typename roiWindow<P>::pixel_t roiWindow<P>::setPixel(int32_t px, int32_t py, typename roiWindow<P>::pixel_t val)
{
    assert(!(px < 0 || py < 0 || !isBound()));
    m_frame_buf->setPixel(px + x(), py + y(), val);
    return val;
}



template <typename P>
bool roiWindow<P>::copy_pixels_from(pixel_ptr_t pixels, int columns, int rows, int stride, int increment)
{
    bool sg = isBound() && pixels != nullptr && width() == columns && height() == rows;
    if (!sg) return !sg;
    pixel_ptr_t pels = pixels;
    if (increment != 1)
    {
        for (int j = 0; j < height(); j++)
        {
            pixel_ptr_t our_row = rowPointer(j);
            pixel_ptr_t our_col = pels;
            for (int i = 0; i < width(); i++)
            {
                *our_row++ = *our_col;
                our_col += increment;
            }
            pels += stride;
        }
        return true;
    }
    
    for (int j = 0; j < height(); j++)
    {
        pixel_ptr_t our_row = rowPointer(j);
        std::memcpy (our_row, pels, stride);
        pels += stride;
    }
    return true;
}


template <typename P>
roiWindow<P> roiWindow<P>::clone() const
{
    if (! isBound())
        return roiWindow<P> ();
    
    roiWindow<P> clo (width(),height(), image_memory_alignment_policy::align_first_row);
    int stride = width() * bytes ();
    for (int j = 0; j < height(); j++)
    {
        pixel_ptr_t our_row = rowPointer(j);
        pixel_ptr_t other_row = clo.rowPointer(j);
        std::memcpy(other_row, our_row, stride);
    }
    return clo;
}


template <typename P>
bool roiWindow<P>::copy_pixels_to(roiWindow<P> & other)
{
    bool sg = isBound() && other.isBound() && width() == other.width() && height() == other.height();
    if (!sg) return !sg;
    int stride = width() * bytes ();
    for (int j = 0; j < height(); j++)
    {
        pixel_ptr_t our_row = rowPointer(j);
        pixel_ptr_t other_row = other.rowPointer(j);
        std::memcpy(other_row, our_row, stride);
    }
    return true;
}



template <typename P>
void roiWindow<P>::output(std::ostream& outstream, uint8_t delim, bool is_normalized) const
{
    // = first < second
    iPair range_j(0, height());
    iPair range_i(0, width());
    bool is_float = P::is_floating_point();

    outstream << std::endl;
    
    float mult = (is_float && is_normalized) ? 255.0 : 1.0;
    
    for (int j = range_j.first; j < range_j.second; j++)
    {
        pixel_ptr_t row_ptr = rowPointer(j);
        std::string ps;
        for (int i = range_i.first - range_i.first; i < range_i.second - range_i.first; i++)
        {
            pixel_t pel = row_ptr[i];
            outstream << (int)(pel*mult);
            outstream << delim  << setw(4);
        }
        outstream << std::endl;
    }
}


uint32_t split_count (const std::string &str, std::vector<std::string>& elems)
{
    // construct a stream from the string
    std::stringstream strstr(str);
    
    // use stream iterators to copy the stream to the vector as whitespace separated strings
    std::istream_iterator<std::string> it(strstr);
    std::istream_iterator<std::string> end;
    std::vector<std::string> results(it, end);
    elems = results;
    return (uint32_t) results.size();
}

iPair get_size_from_file (const std::string& fqfn)
{
    std::shared_ptr<std::ifstream> myfile = make_shared_ifstream(fqfn);
    std::vector<std::string> elems;
    
    uint32_t line_count = 0;
    uint32_t diff_count = 0;
    uint32_t col_count = 0;
    for(std::string line; std::getline(*myfile, line); )
    {
        uint32_t this_count = split_count (line, elems);
        if (col_count == 0)
        {
            col_count = this_count;
        }
        if (col_count != this_count)
        {
            diff_count++;
        }
        line_count++;
    }
    
  //  std::cout << line_count << std::endl;
  //  std::cout << col_count << std::endl;
  //  std::cout << diff_count << std::endl;
    
    // = first < second
    if (diff_count) return iPair(-1,-1);
    iPair range (col_count, line_count);
    return range;
}

/*
 * for small uint8 or uint16 images.
 *
 */


template <typename P>
roiWindow<P>::roiWindow(const std::string& fqfn, image_memory_alignment_policy im)
{
    auto sizeit = get_size_from_file(fqfn);
    
    if (sizeit.first > 0 && sizeit.second > 0)
    {
        int w = sizeit.first;
        int h = sizeit.second;
        
        m_frame_buf = sharedRoot_t (new root<P>(w, h, im));
        m_bounds = iRect(w, h);
        std::shared_ptr<std::ifstream> myfile = make_shared_ifstream(fqfn);
        
        unsigned row = 0;
        unsigned col_count = 0;
        for(std::string line; std::getline(*myfile, line); )
        {
            std::vector<std::string> elems;
            uint32_t this_count = split_count (line, elems);
            if (col_count == 0)
            {
                col_count = this_count;
            }
            assert (col_count == this_count);

            
            int col = 0;
            for (auto elem : elems)
            {
                setPixel(col++,row, (pixel_t) atoi(elem.c_str()));
            }
            row++;
        }
    }
}



template <typename P>
void roiWindow<P>::setBorder(int pad, pixel_t val)
{
    const int width(this->width());
    const int height(this->height());
    {
        roiWindow<P> win(*this, 0, 0, width, pad);
        win.set(val);
    }
    {
        roiWindow<P> win(*this, 0, height - pad, width, pad);
        win.set(val);
    }
    {
        roiWindow<P> win(*this, 0, 0, pad, height);
        win.set(val);
    }
    {
        roiWindow<P> win(*this, width - pad, 0, pad, height);
        win.set(val);
    }
}


template <typename P>
roiWindow<P>::roiWindow(const roiWindow<P> & parentWindow, int32_t tl_x, int32_t tl_y, int32_t width, int32_t height)
{
    assert(!(tl_x < 0 || tl_y < 0 || !parentWindow.isBound()));

    int32_t x0 = tl_x + parentWindow.x();
    int32_t y0 = tl_y + parentWindow.y();
    // rect in root buffer of parent
    iRect geom = iRect(x0, y0, width, height);

    // Clip or no clip, our bounds will be this root or a throw
    m_bounds = parentWindow.m_bounds;
    if (parentWindow.bound().contains(geom))
    {
        *this = roiWindow(parentWindow.frameBuf(), x0, y0, width, height);
    }
}


template <typename P>
roiWindow<P> roiWindow<P>::window(int32_t tl_x, int32_t tl_y, int32_t width, int32_t height)
{
    if (!(tl_x < 0 || tl_y < 0 || !isBound()))
    {

        int32_t x0 = tl_x + x();
        int32_t y0 = tl_y + y();
        // rect in root buffer of parent
        iRect geom = iRect(x0, y0, width, height);


        if (frame().contains(geom))
        {
            return roiWindow<P>(frameBuf(), x0, y0, width, height);
        }
    }

    return roiWindow<P>();
}

/***
 
    roiFixedMultiWindow Implementation

 ****/

 template <typename T,  int W, int H, int CHANNELS>
roiFixedMultiWindow<T,W,H,CHANNELS>::roiFixedMultiWindow ()
{
    
}


template <typename T,  int W, int H, int CHANNELS>
roiFixedMultiWindow<T,W,H,CHANNELS>::roiFixedMultiWindow (const window_t& root,const std::vector<std::string>& names_l2r, int64_t timestamp,
                                               image_memory_alignment_policy im ) : window_t(root)
{
    bool size_ok = root.width () == W && root.height() == (H*CHANNELS);
    assert(size_ok);
    
     init(names_l2r, timestamp, im);
}

 template <typename T,  int W, int H, int CHANNELS>
roiFixedMultiWindow<T,W,H,CHANNELS>::roiFixedMultiWindow(const std::vector<std::string>& names_l2r, int64_t timestamp,
               image_memory_alignment_policy im )
: roiWindow<traits_t> (W, CHANNELS * H, im)
{
    init(names_l2r, timestamp, im);
}


template <typename T,  int W, int H, int CHANNELS>
void roiFixedMultiWindow<T,W,H,CHANNELS>::init (const std::vector<std::string>& names_l2r, int64_t timestamp,
                                          image_memory_alignment_policy im )
{
    this->frameBuf ()->setTimestamp (timestamp);
    auto names = names_l2r;
    if(names.empty())
        for (auto ww = 0; ww < CHANNELS; ww++) names.push_back(tostr(ww));
    
     for (unsigned ww = 0; ww < CHANNELS; ww++)
     {
         m_names[ww] = names[ww];
         m_indexes[ww] = ww;
     }
    
    iPair spacing (0, H);
    iPair msize (W, H);
    m_bounds[0].size (msize);
    auto allr = m_bounds[0];
    for (auto ww = 1; ww < CHANNELS; ww++){
        m_bounds[ww] = m_bounds[ww-1];
        m_bounds[ww].translate (spacing);
        allr |= m_bounds[ww];
    }
    
    assert (allr == this->bound ());
    
    for (unsigned ww = 0; ww < CHANNELS; ww++)
    {
        m_planes[ww] = roiWindow<traits_t>(this->frameBuf(),m_bounds[ww]);
        assert(m_planes[ww].width() == W);
        assert(m_planes[ww].height() == H);
    }
    
}

 template <typename T, int W, int H, int CHANNELS>
roiFixedMultiWindow<T,W,H,CHANNELS>::roiFixedMultiWindow(const roiFixedMultiWindow<T,W,H,CHANNELS> & other)
{
    for (unsigned ww = 0; ww < CHANNELS; ww++)
    {
        m_planes[ww] = other.m_planes[ww];
        m_bounds[ww] = other.m_bounds [ww];
        m_names[ww] = other.m_names [ww];
        m_indexes[ww] = other.m_indexes [ww];
    }
}

template <typename T,  int W, int H, int CHANNELS>
bool roiFixedMultiWindow<T,W,H,CHANNELS>::operator==(const roiFixedMultiWindow<T,W,H,CHANNELS> & other) const
{
    for (unsigned ww = 0; ww < CHANNELS; ww++)
    {
        if (m_planes[ww] != other.m_planes[ww]) return false;
        if (m_bounds[ww] != other.m_bounds [ww]) return false;
        if (m_names[ww] != other.m_names [ww]) return false;
        if (m_indexes[ww] != other.m_indexes [ww]) return false;
    }
    return true;
}



template <typename T,  int W, int H, int CHANNELS>
bool roiFixedMultiWindow<T,W,H,CHANNELS>::operator!=(const roiFixedMultiWindow<T,W,H,CHANNELS> & other) const
{
    return !operator==(other);
}


template <typename T,  int W, int H, int CHANNELS>
const roiFixedMultiWindow<T,W,H,CHANNELS> & roiFixedMultiWindow<T,W,H,CHANNELS>::operator=(const roiFixedMultiWindow<T,W,H,CHANNELS> & rhs)
{
    if (*this == rhs) return *this;
    for (unsigned ww = 0; ww < CHANNELS; ww++)
    {
        m_planes[ww] = rhs.m_planes[ww];
        m_bounds[ww] = rhs.m_bounds [ww];
        m_names[ww] = rhs.m_names [ww];
        m_indexes[ww] = rhs.m_indexes [ww];
    }
    return *this;

}


/***
 
 roiMultiWindow Implementation
 
 ****/


template <typename T, typename trait_t>
roiMultiWindow<T,trait_t>::roiMultiWindow (const iPair& size, int64_t timestamp,image_memory_alignment_policy im ) : m_size(size),
 roiWindow<trait_t> (size.first, T::planes_c * size.second, im)
{
    m_spacing.first = 0;
    m_spacing.second = m_size.second;
    
    init(timestamp, im);
}



template <typename T, typename trait_t>
roiMultiWindow<T,trait_t>::roiMultiWindow (const window_t& root, int64_t timestamp,image_memory_alignment_policy im ) : roiWindow<trait_t>(root)
{
    bool size_ok = (root.height() % T::planes_c) == 0;
    assert(size_ok);
    m_size.first = root.width ();
    m_size.second = root.height() / T::planes_c;
    m_spacing.first = 0;
    m_spacing.second = m_size.second;
    
    init(timestamp, im);
}


template <typename T, typename trait_t>
void roiMultiWindow<T,trait_t>::init ( int64_t timestamp, image_memory_alignment_policy im )
{
    
    this->frameBuf ()->setTimestamp (timestamp);

    m_bounds[0].size (m_size);
    m_bounds[1] = m_bounds[0];
    m_bounds[1].translate (m_spacing);
    m_bounds[2] = m_bounds[1];
    m_bounds[2].translate (m_spacing);
    
    auto allr = m_bounds[0] | m_bounds[1] | m_bounds[2];
    assert (allr == this->bound ());
    
    for (unsigned ww = 0; ww < T::planes_c; ww++)
    {
        m_planes[ww] = roiWindow<trait_t>(this->frameBuf(),m_bounds[ww]);
        assert(m_planes[ww].width() == m_size.first);
        assert(m_planes[ww].height() == m_size.second);
    }
    
}

template <typename T, typename trait_t>
roiMultiWindow<T,trait_t>::roiMultiWindow(const roiMultiWindow<T,trait_t> & other)
{
    for (unsigned ww = 0; ww < T::planes_c; ww++)
    {
        m_planes[ww] = other.m_planes[ww];
        m_bounds[ww] = other.m_bounds [ww];
    }
}

template <typename T, typename trait_t>
bool roiMultiWindow<T,trait_t>::operator==(const roiMultiWindow<T,trait_t> & other) const
{
    for (unsigned ww = 0; ww < T::planes_c; ww++)
    {
        if (m_planes[ww] != other.m_planes[ww]) return false;
        if (m_bounds[ww] != other.m_bounds [ww]) return false;
    }
    return true;
}



template <typename T, typename trait_t>
bool roiMultiWindow<T,trait_t>::operator!=(const roiMultiWindow<T,trait_t> & other) const
{
    return !operator==(other);
}


template <typename T, typename trait_t>
const roiMultiWindow<T,trait_t> & roiMultiWindow<T,trait_t>::operator=(const roiMultiWindow<T,trait_t> & rhs)
{
    if (*this == rhs) return *this;
    for (unsigned ww = 0; ww < T::planes_c; ww++)
    {
        m_planes[ww] = rhs.m_planes[ww];
        m_bounds[ww] = rhs.m_bounds [ww];
    }
    return *this;
    
}


namespace svl
{
    template class root<P8U>;
    template class root<P16U>;
    template class root<P32F>;
    template class root<P32S>;
    template class root<P8UC3>;
    template class root<P8UC4>;
    
    
    template class roiWindow<P8U>;
    template class roiWindow<P16U>;
    template class roiWindow<P32F>;
    template class roiWindow<P32S>;
    template class roiWindow<P8UC3>;
    template class roiWindow<P8UC4>;
    
    template class roiFixedMultiWindow<P8UP2,512,128,2>;
    template class roiFixedMultiWindow<P8UP2,512,256,2>;
    template class roiFixedMultiWindow<P8UP3>;
     template class roiMultiWindow<P8UP2>;
     template class roiMultiWindow<P8UP3>;
     template class roiMultiWindow<P8UP4>;
}

#pragma GCC diagnostic pop
//
//   |                      /----------------
//  O|                     / |
//  u|                    /  |
//  t|                   /   |
//  p|------------------/    |
//  u|                  |    |
//  t|<---------------->|<-->|
//       |  "offset"         "spread"
//   |
// --+---Input--------------------------------
//   |
