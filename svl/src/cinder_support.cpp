

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
#include "cinder_cv/cinder_xchg.hpp"
#include "cinder/ImageSourceFileQuartz.h"



namespace anonymous
{
    
    // A onetime initiation of anything in cinder that needs it
    class cinder_startup : public internal_singleton<cinder_startup>
    {
        public:
        cinder_startup ()
        {
            //  ci::ImageSourcePng::registerSelf();
            ci::ImageSourceFileQuartz::registerSelf();
        }
    };
}

namespace svl
{
  
    // ci is the step number for channel.
    roiWindow<P8U> NewFromChannel ( ChannelT<uint8_t>& onec, uint8_t ci)
    {
        assert(ci >= 0 && ci < 4);
        
        P8U::value_type* pixels = onec.getData();
        roiWindow<P8U> window (onec.getWidth (),onec.getHeight (), image_memory_alignment_policy::align_first_row);
        
        window.copy_pixels_from(pixels, onec.getWidth (),onec.getHeight (), (int32_t) onec.getRowBytes (), ci);
        return window;
    }
    
    
    roiWindow<P8U> NewRedFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannel(src->getRedOffset()), src->getRedOffset());
    }
    
    roiWindow<P8U> NewBlueFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannel(src->getBlueOffset()), src->getBlueOffset());
    }
    
    roiWindow<P8U> NewGreenFromSurface (const Surface8uRef& src)
    {
        return NewFromChannel (src->getChannel(src->getGreenOffset()), src->getGreenOffset());
    }
    
    roiWindow<P8U> NewGrayFromSurface (const Surface8uRef& src)
    {
        Channel8uRef ci8 = ChannelT<uint8_t>::create(src->getWidth(), src->getHeight());
        ip::grayscale (*src, ci8.get());
        return NewFromChannel (*ci8, 0);
    }
    
    
    
    std::shared_ptr<roiWindow<P8U>> NewRefSingleFromSurface (const Surface8uRef& src,
                                                                    const std::vector<std::string>& names_l2r,
                                                                    int64_t timestamp)
    {
        auto channel = src->getChannel(0);
        uint8_t* pixels = channel.getData();
        std::shared_ptr<roiWindow<P8U>> mwRef (new roiWindow<P8U>(src->getWidth(), src->getHeight()));
        assert (channel.getWidth() == mwRef->width() && channel.getHeight() == mwRef->height());
        mwRef->copy_pixels_from(pixels, channel.getWidth (),channel.getHeight (), (int32_t) channel.getRowBytes ());
        return mwRef;
    }
    
    
    std::shared_ptr<roiFixedMultiWindow<P8UP3>> NewRefMultiFromChannel ( ChannelT<uint8_t>& onec,
                                                                   const std::vector<std::string>& names_l2r, int64_t timestamp)
    {
        uint8_t* pixels = onec.getData();
        std::shared_ptr<roiFixedMultiWindow<P8UP3>> mwRef (new roiFixedMultiWindow<P8UP3>(names_l2r, timestamp));
        assert (onec.getWidth() == mwRef->width() && onec.getHeight() == mwRef->height());
        mwRef->copy_pixels_from(pixels, onec.getWidth (),onec.getHeight (), (int32_t) onec.getRowBytes ());
        return mwRef;
    }
    
    
    std::pair<Surface8uRef, Channel8uRef> image_io_read_surface (const boost::filesystem::path & pp)
    {
        anonymous::cinder_startup::instance();
        const std::string extension = pp.extension().string();
        ImageSource::Options opt;
        auto ir = loadImage (pp, opt, extension );
        std::pair<Surface8uRef, Channel8uRef> wp;
        
        
        if ( (ir->getDataType() != PixelType<P8U>::ct() || ir->getColorModel() != PixelType<P8U>::cm()) &&
            (ir->getDataType() != PixelType<P8UC4>::ct() || ir->getColorModel() != PixelType<P8UC4>::cm()) )
        return wp;
        
        switch (ir->getChannelOrder())
        {
            case ci::ImageIo::ChannelOrder::Y:
            {
                wp.second = ci::ChannelT<uint8_t>::create (ir);
                break;
            }
            case ci::ImageIo::ChannelOrder::RGBX:
            {
                wp.first = ci::SurfaceT<uint8_t>::create (ir);
                break;
            }
            default:
            assert(false);
        }
        
        return wp;
        
    }

  
    
}
