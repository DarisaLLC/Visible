#ifndef __CINDER_SUPPORT__
#define __CINDER_SUPPORT__

#include <boost/filesystem.hpp>
#include <cinder/Channel.h>
#include <cinder/Area.h>
#include <limits>
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "vision/roiWindow.h"
#include "core/vector2d.hpp"
#include <fstream>
#include "core/singleton.hpp"
#include "cinder/ImageSourcePng.h"
#include "cinder/ip/Grayscale.h"


using namespace std;
using namespace svl;

namespace fs = boost::filesystem;

namespace svl
{
    
    inline vec2 fromSvl( const fVector_2d &point )
    {
        return vec2( point.x(), point.y() );
    }
    
    inline fVector_2d toOcv( const vec2 &point )
    {
        return fVector_2d ( point.x, point.y );
    }

 
    template<typename D>
    struct fromSurface8UCloner
    {
        void operator () (const Surface8uRef&, roiWindow<D>&);
    };


    static roiWindow<P8U> NewFromChannel ( ChannelT<uint8_t>& onec)
    {
        uint8_t* pixels = onec.getData();
        roiWindow<P8U> window (onec.getWidth (),onec.getHeight ());
        window.copy_pixels_from(pixels, onec.getWidth (),onec.getHeight (), (int32_t) onec.getRowBytes ());
        return window;
    }
    
    
    static roiWindow<P8UC4> NewFromSurface ( Surface8u *ones)
    {
        uint8_t* pixels = (uint8_t*) ones->getData();
        roiWindow<P8UC4> window (ones->getWidth (),ones->getHeight ());
        window.copy_pixels_from(pixels, ones->getWidth (),ones->getHeight (), (int32_t) ones->getRowBytes ());
        return window;
    }
    
    static roiWindow<P8U> NewGrayFromSurface (const Surface8uRef& src)
    {
        Channel8uRef ci8 = ChannelT<uint8_t>::create(src->getWidth(), src->getHeight());
        ci::ip::grayscale (*src, ci8.get());
        return NewFromChannel (*ci8);
    }
    
    template<typename P, class pixel_t = typename PixelType<P>::pixel_t>
    static std::shared_ptr<ChannelT<pixel_t> >  newCiChannel (const roiWindow<P>& w)
    {
        if (!w.isBound()) return 0;
        auto chP = ChannelT<pixel_t>::create( w.width(), w.height () );
        
        const cinder::Area clippedArea = chP->getBounds();
        int32_t rowBytes = chP->getRowBytes();
        
        for( int32_t y = clippedArea.getY1(); y < clippedArea.getY2(); ++y )
        {
            pixel_t *dstPtr = reinterpret_cast<pixel_t*>( reinterpret_cast<pixel_t*>( chP->getData() + clippedArea.getX1() ) + y * rowBytes );
            const pixel_t *srcPtr = w.rowPointer(y);
            for( int32_t x = 0; x < clippedArea.getWidth(); ++x, dstPtr++, srcPtr++)
            {
                *dstPtr = *srcPtr;
            }
        }
        
        return chP;
    }
    
    std::pair<Surface8uRef, Channel8uRef> image_io_read_surface (const boost::filesystem::path & pp);
      
    
}



#endif

