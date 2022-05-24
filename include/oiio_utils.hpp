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
#include "boost/filesystem.hpp"
namespace bfs=boost::filesystem;

using namespace OIIO;
using namespace cv;

void dump_all_extra_attributes (const ImageSpec& spec);
std::string describe_image_spec (const ImageSpec& spec);
cv::Mat getRootFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index);
cv::Mat getRoiFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index, const ROI& roi);
cv::Mat getRoiFrame(const std::shared_ptr<ImageBuf>& ib, const ustring& contentName, int frame_index,
                    const iPair& tl, int width, int height);

bool write_tiff_stackU8 (const bfs::path& output_path, std::vector<cv::Mat>& images);

#endif /* oiio_utils_hpp */
