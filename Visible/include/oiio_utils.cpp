//
//  oiio_utils.cpp
//  Visible
//
//  Created by Arman Garakani on 10/18/20.
//

#include "oiio_utils.hpp"

std::string describe_image_spec (const ImageSpec& spec){
    std::ostringstream oss;
    oss << spec.nchannels << " Channels \n";
    for (auto cn : spec.channelnames)
        oss << cn << std::endl;
    oss << " ( width: " << spec.width << " , " << "height: " << spec.height << std::endl;
    oss << spec.format << std::endl;
    return oss.str();
}

void dump_all_extra_attributes (const ImageSpec& spec){
    for (size_t i = 0;  i < spec.extra_attribs.size();  ++i) {
        const ParamValue &p (spec.extra_attribs[int(i)]);
        printf ("    %s: ", p.name().c_str());
        if (p.type() == TypeString)
            printf ("\"%s\"", *(const char **)p.data());
        else if (p.type() == TypeFloat)
            printf ("%g", *(const float *)p.data());
        else if (p.type() == TypeInt)
            printf ("%d", *(const int *)p.data());
        else if (p.type() == TypeDesc::UINT)
            printf ("%u", *(const unsigned int *)p.data());
        else if (p.type() == TypeMatrix) {
            const float *f = (const float *)p.data();
            printf ("%f %f %f %f %f %f %f %f "
                    "%f %f %f %f %f %f %f %f",
                    f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7],
                    f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15]);
        }
        else
            printf (" <unknown data type> ");
        printf ("\n");
    }
}

cv::Mat getRootFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index){
    ib->reset(contentName, frame_index, 0);
    ROI roi = ib->roi();
    const ImageSpec& spec = ib->spec();
    if(ib->pixeltype() == TypeUInt16){
        cv::Mat cvb (spec.height, spec.width, CV_16U);
        ib->get_pixels(roi, TypeUInt16, cvb.data);
        return cvb;
    }
    else if (ib->pixeltype() == TypeUInt8){
        cv::Mat cvb (spec.height, spec.width, CV_8U);
        ib->get_pixels(roi, TypeUInt8, cvb.data);
        return cvb;
    }
    else{
        std::cout << " Unimplemented " << std::endl;
        assert(false);
    }
}

cv::Mat getRoiFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index, const ROI& roi){
    assert(ib->roi().contains(roi));
    ib->reset(contentName, frame_index, 0);
    const ImageSpec& spec = ib->spec();
    if(ib->pixeltype() == TypeUInt16){
        cv::Mat cvb (spec.width, spec.height, CV_16U);
        ib->get_pixels(roi, TypeUInt16, cvb.data);
        return cvb;
    }
    else if (ib->pixeltype() == TypeUInt8){
        cv::Mat cvb (spec.width, spec.height, CV_8U);
        ib->get_pixels(roi, TypeUInt8, cvb.data);
        return cvb;
    }
    else{
        std::cout << " Unimplemented " << std::endl;
        assert(false);
    }
}

cv::Mat getRoiFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index,
                    const iPair& tl, int width, int height){
    ROI roi (tl.first, tl.first+width, tl.second, tl.second+height,0, 0, 0, 0);
    return getRoiFrame(ib, contentName, frame_index, roi);
}

