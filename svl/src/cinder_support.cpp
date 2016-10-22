

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
#include "cinder/cinder_xchg.hpp"
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

std::pair<Surface8uRef, Channel8uRef> svl::image_io_read_surface (const boost::filesystem::path & pp)
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

