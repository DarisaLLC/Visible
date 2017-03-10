#ifndef __CINDER_SUPPORT__
#define __CINDER_SUPPORT__

#include <boost/filesystem.hpp>
#include <cinder/Channel.h>
#include <cinder/Area.h>
#include <limits>
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "vision/roiWindow.h"
#include "vision/roiMultiWindow.h"
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

    static std::shared_ptr<roiWindow<P8U>> NewRefFromChannel ( ChannelT<uint8_t>& onec)
    {
        uint8_t* pixels = onec.getData();
        std::shared_ptr<root<P8U>> rootref (new root<P8U> (pixels, (int32_t) onec.getRowBytes (),
                                                           onec.getWidth(), onec.getHeight()));
        return std::shared_ptr<roiWindow<P8U>> (new roiWindow<P8U>(rootref, 0, 0, onec.getWidth(), onec.getHeight()));
    }
    
    static std::shared_ptr<roiMultiWindow<P8UP3>> NewRefMultiFromChannel ( ChannelT<uint8_t>& onec,
                                                                          const std::vector<std::string>& names_l2r, int64_t timestamp = 0)
    {
        uint8_t* pixels = onec.getData();
        std::shared_ptr<roiMultiWindow<P8UP3>> mwRef (new roiMultiWindow<P8UP3>(names_l2r, timestamp));
        assert (onec.getWidth() == mwRef->width() && onec.getHeight() == mwRef->height());
        mwRef->copy_pixels_from(pixels, onec.getWidth (),onec.getHeight (), (int32_t) onec.getRowBytes ());
        return mwRef;
    }
    
    static std::shared_ptr<roiMultiWindow<P8UP3>> NewRefMultiFromSurface (const Surface8uRef& src, const std::vector<std::string>& names_l2r, int64_t timestamp = 0)
    {
        return NewRefMultiFromChannel (src->getChannelRed(), names_l2r, timestamp);
    }
    

    static roiWindow<P8U> NewFromChannel ( ChannelT<uint8_t>& onec)
    {
        uint8_t* pixels = onec.getData();
        roiWindow<P8U> window (onec.getWidth (),onec.getHeight ());
        window.copy_pixels_from(pixels, onec.getWidth (),onec.getHeight (), (int32_t) onec.getRowBytes ());
        return window;
    }
    
//    static std::shared_ptr<roiWindow<P8U>> NewRefFromChannel ( ChannelT<uint8_t>& onec)
//    {
//        uint8_t* pixels = onec.getData();
//        std::shared_ptr<root<P8U>> rootref (new root<P8U> (pixels, (int32_t) onec.getRowBytes (),
//                                                           onec.getWidth(), onec.getHeight()));
//        return std::shared_ptr<roiWindow<P8U>> (new roiWindow<P8U>(rootref, 0, 0, onec.getWidth(), onec.getHeight()));
//    }
    
    static std::shared_ptr<roiWindow<P8U>> NewIndexedChannelFromSurface (const Surface8uRef& src, uint8_t channelIndex)
    {
        return NewRefFromChannel (src->getChannel(channelIndex));
    }
    
    static roiWindow<P8UC4> NewFromSurface ( Surface8u *ones)
    {
        uint8_t* pixels = (uint8_t*) ones->getData();
        roiWindow<P8UC4> window (ones->getWidth (),ones->getHeight ());
        window.copy_pixels_from(pixels, ones->getWidth (),ones->getHeight (), (int32_t) ones->getRowBytes ());
        return window;
    }
    
    static roiWindow<P8U> NewRedFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannelRed());
    }
    
    static roiWindow<P8U> NewBlueFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannelBlue());
    }
    
    static roiWindow<P8U> NewGreenFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannelGreen());
    }

    static roiWindow<P8U> NewAlphaFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannelAlpha());
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

