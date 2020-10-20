//
//  oiio_utils.cpp
//  Visible
//
//  Created by Arman Garakani on 10/18/20.
//

#include "oiio_utils.hpp"


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

