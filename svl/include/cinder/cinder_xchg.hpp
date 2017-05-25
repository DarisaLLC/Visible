#ifndef __CINDER_XCHG__
#define __CINDER_XCHG__

#include <boost/filesystem.hpp>
#include <cinder/Channel.h>
#include <cinder/Area.h>
#include <limits>
//#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "vision/roiWindow.h"
#include "vision/roiVariant.hpp"
#include "vision/roiMultiWindow.h"
#include "core/vector2d.hpp"
#include <fstream>
//#include "core/singleton.hpp"
#include "cinder/ImageSourcePng.h"
#include "cinder/ip/Grayscale.h"




using namespace std;
using namespace svl;
namespace ip=ci::ip;

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
    
    template<typename C, typename S>
    struct imageRep;
    
    template<typename C, typename S>
    struct imageRep
    {
        static const bool mapsTo = false;
    };
    
    template<> struct imageRep<Surface8u,roiWindow<P8UC4>>
    {
        static const bool mapsTo = true;
    };
    
    template<> struct imageRep<Channel8u,roiWindow<P8U>>
    {
        static bool const mapsTo = true;
    };
     
    ci::Surface8uRef load_from_path (const fs::path& path, bool assume_jpg_if_no_extension = true);
//    cv::Mat load_from_path_ocv (const fs::path& path,  bool assume_jpg_if_no_extension = true);
    
    static std::shared_ptr<roiWindow<P8U>> NewRefFromChannel ( ChannelT<uint8_t>& onec)
    {
        uint8_t* pixels = onec.getData();
        std::shared_ptr<root<P8U>> rootref (new root<P8U> (pixels, (int32_t) onec.getRowBytes (),
                                                           onec.getWidth(), onec.getHeight()));
        return std::shared_ptr<roiWindow<P8U>> (new roiWindow<P8U>(rootref, 0, 0, onec.getWidth(), onec.getHeight()));
    }
    
    
    
    // ci is the step number for channel.
    static roiWindow<P8U> NewFromChannel ( ChannelT<uint8_t>& onec, uint8_t ci)
    {
        assert(ci >= 0 && ci < 4);
        
        uint8_t* pixels = onec.getData();
        roiWindow<P8U> window (onec.getWidth (),onec.getHeight (), image_memory_alignment_policy::align_first_row);
        
        window.copy_pixels_from(pixels, onec.getWidth (),onec.getHeight (), (int32_t) onec.getRowBytes (), ci);
        return window;
    }
    
    static roiWindow<P8U> NewChannelFromSurfaceAtIndex (const Surface8uRef& src, uint8_t ci)
    {
        return NewFromChannel (src->getChannel(ci), ci);
    }
    
    
    static roiVP8UC  NewFromSurface (const Surface8u *ones)
    {
        roiVP8UC unknown;
        uint8_t* pixels = (uint8_t*) ones->getData();
        switch(ones->getPixelInc())
        {
            case 3:
            {
                roiWindow<P8UC3> window3 (ones->getWidth (),ones->getHeight ());
                window3.copy_pixels_from(pixels, ones->getWidth (),ones->getHeight (), (int32_t) ones->getRowBytes ());
                unknown = window3;
            }
                break;
            case 4:
            {
                roiWindow<P8UC4> window4 (ones->getWidth (),ones->getHeight ());
                window4.copy_pixels_from(pixels, ones->getWidth (),ones->getHeight (), (int32_t) ones->getRowBytes ());
                unknown = window4;
            }
                break;
            default:
                assert(false);
                break;
        }
        return unknown;
    }
    
    
    static roiWindow<P8U> NewRedFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannel(src->getRedOffset()), src->getRedOffset());
    }
    
    static roiWindow<P8U> NewBlueFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannel(src->getBlueOffset()), src->getBlueOffset());
    }
    
    static roiWindow<P8U> NewGreenFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannel(src->getGreenOffset()), src->getGreenOffset());
    }
    
    static roiWindow<P8U> NewGrayFromSurface (const Surface8uRef& src)
    {
        Channel8uRef ci8 = ChannelT<uint8_t>::create(src->getWidth(), src->getHeight());
        ip::grayscale (*src, ci8.get());
        return NewFromChannel (*ci8, 0);
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
    
    
    /*
     * Returns a channel if the color order indicates monochrome, then a channel is returned, otherwise a surface
     */
    
    svl::roiVP8UC image_io_read_varoi (const boost::filesystem::path & pp, const std::string& = "red", uint32_t dst_channels = 1);
    
    
    //! Converts Surface \a srcSurface to grayscale and stores the result in Surface \a dstSurface. Uses primary weights dictated by the Rec. 709 Video Standard
    template<typename T>
    void graygmean( const SurfaceT<T> &srcSurface, SurfaceT<T> *dstSurface );
    //! Converts Surface \a srcSurface to grayscale and stores the result in Channel \a dstChannel. Uses primary weights dictated by the Rec. 709 Video Standard
    template<typename T>
    void graygmean ( const SurfaceT<T> &srcSurface, ChannelT<T> *dstChannel );
    
    
    
    
}



#endif

