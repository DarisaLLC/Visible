//
//  oiio_utils.hpp
//  Visible
//
//  Created by Arman Garakani on 10/18/20.
//

#ifndef oiio_utils_hpp
#define oiio_utils_hpp

#include <stdio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/imageio.h>
#include "opencv2/opencv.hpp"
#include "core/pair.hpp"

using namespace OIIO;
using namespace cv;

cv::Mat getRootFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index);
cv::Mat getRoiFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index, const ROI& roi);
cv::Mat getRoiFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index,
                    const iPair& tl, int width, int height);

#endif /* oiio_utils_hpp */
