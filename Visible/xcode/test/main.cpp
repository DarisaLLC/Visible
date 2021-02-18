//
//  main.cpp
//  VisibleGtest
//
//  Created by Arman Garakani on 5/16/16.
//
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"

#include <OpenImageIO/imagebuf.h>

#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/imageio.h>


#include <iostream>
#include <fstream>
#include <chrono>
#include "gtest/gtest.h"
#include <memory>
#include <thread>
#include <list>


#include <boost/foreach.hpp>
#include "boost/algorithm/string.hpp"
#include <boost/range/irange.hpp>
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"

#include "vision/voxel_frequency.h"

#include "opencv2/highgui.hpp"
#include "vision/roiWindow.h"
#include "vision/self_similarity.h"
#include "cinder/ImageIo.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "cm_time.hpp"
#include "core/gtest_env_utils.hpp"
#include "vision/colorhistogram.hpp"
#include "cinder_opencv.h"
#include "ut_util.hpp"
#include "core/csv.hpp"
#include "core/kmeans1d.hpp"
#include "core/stl_utils.hpp"
#include "core/core.hpp"
#include "contraction.hpp"
#include "sm_producer.h"
#include "sg_filter.h"
#include "vision/drawUtils.hpp"
#include "ut_localvar.hpp"
#include "vision/labelBlob.hpp"
#include "result_serialization.h"
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include "permutation_entropy.h"
#include "result_serialization.h"
#include "cvplot/cvplot.h"
#include "time_series/persistence1d.hpp"
#include "timed_types.h"
#include "CinderImGui.h"
#include "logger/logger.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/gauss.hpp"
#include "vision/dense_motion.hpp"
#include "vision/gradient.h"
#include "vision/ipUtils.h"
#include "lif_content.hpp"
#include "cinder/PolyLine.h"
#include "etw_utils.hpp"
#include "core/lineseg.hpp"
#include "vision/ellipse.hpp"
#include "moving_region.h"
#include "algo_runners.hpp"
#include <stdio.h>
#include <gsl/gsl_sf_bessel.h>
#include "core/moreMath.h"
#include "eigen_utils.hpp"
#include "cluster_geometry.hpp"
#include "core/fit.hpp"
#include <OpenImageIO/imagebufalgo.h>
#include "temporal_medianOf3.hpp"
#include "vision/ellipse.hpp"



using namespace OIIO;

using namespace etw_utils;
using namespace cgeom;

// @FIXME Logger has to come before these
#include "ut_units.hpp"
#include "ut_cardio.hpp"
//#include "ut_sm.hpp" @todo refactor
#define cimg_plugin1 "plugins/cvMat.h"
#define cimg_display 0
#include "CImg.h"
using namespace cimg_library;


//#define INTERACTIVE

using namespace boost;

using namespace ci;
using namespace ci::ip;
using namespace spiritcsv;
using namespace kmeans1D;
using namespace stl_utils;
using namespace std;
using namespace svl;

namespace fs = boost::filesystem;

static const auto CVCOLOR_RED = cv::Vec3b(0, 0, 255);
static const auto CVCOLOR_YELLOW = cv::Vec3b(0, 255, 255);
static const auto CVCOLOR_GREEN = cv::Vec3b(0, 255, 0);
static const auto CVCOLOR_BLUE = cv::Vec3b(255, 0, 0);
static const auto CVCOLOR_GRAY = cv::Vec3b(127, 127, 127);
static const auto CVCOLOR_BLACK = cv::Vec3b(0, 0, 0);
static const auto CVCOLOR_WHITE = cv::Vec3b(255, 255, 255);
static const auto CVCOLOR_VIOLET = cv::Vec3b(127, 0, 255);


std::vector<double> oneD_example = {39.1747, 39.2197, 39.126, 39.0549, 39.0818, 39.0655, 39.0342,
    38.8791, 38.8527, 39.0099, 38.8608, 38.9188, 38.8499, 38.6693,
    38.2513, 37.9095, 37.3313, 36.765, 36.3621, 35.7261, 35.0656,
    34.2602, 33.2523, 32.3183, 31.6464, 31.0073, 29.8166, 29.3423,
    28.5223, 27.5152, 26.8191, 26.3114, 25.8164, 25.0818, 24.7631,
    24.6277, 24.8184, 25.443, 26.2479, 27.8759, 29.2094, 30.7956,
    32.3586, 33.6268, 35.1586, 35.9315, 36.808, 37.3002, 37.67, 37.9986,
    38.2788, 38.465, 38.5513, 38.6818, 38.8076, 38.9388, 38.9592,
    39.058, 39.1322, 39.0803, 39.1779, 39.1531, 39.1375, 39.1978,
    39.0379, 39.1231, 39.202, 39.1581, 39.1777, 39.2971, 39.2366,
    39.1555, 39.2822, 39.243, 39.1807, 39.1488, 39.2491, 39.265, 39.198,
    39.2855, 39.2595, 39.4274, 39.3258, 39.3162, 39.4143, 39.3034,
    39.2099, 39.2775, 39.5042, 39.1446, 39.188, 39.2006, 39.1799,
    39.4077, 39.2694, 39.1967, 39.2828, 39.2438, 39.2093, 39.2167,
    39.2749, 39.4703, 39.2846};


std::vector<float> oneF_example = {39.1747, 39.2197, 39.126, 39.0549, 39.0818, 39.0655, 39.0342,
    38.8791, 38.8527, 39.0099, 38.8608, 38.9188, 38.8499, 38.6693,
    38.2513, 37.9095, 37.3313, 36.765, 36.3621, 35.7261, 35.0656,
    34.2602, 33.2523, 32.3183, 31.6464, 31.0073, 29.8166, 29.3423,
    28.5223, 27.5152, 26.8191, 26.3114, 25.8164, 25.0818, 24.7631,
    24.6277, 24.8184, 25.443, 26.2479, 27.8759, 29.2094, 30.7956,
    32.3586, 33.6268, 35.1586, 35.9315, 36.808, 37.3002, 37.67, 37.9986,
    38.2788, 38.465, 38.5513, 38.6818, 38.8076, 38.9388, 38.9592,
    39.058, 39.1322, 39.0803, 39.1779, 39.1531, 39.1375, 39.1978,
    39.0379, 39.1231, 39.202, 39.1581, 39.1777, 39.2971, 39.2366,
    39.1555, 39.2822, 39.243, 39.1807, 39.1488, 39.2491, 39.265, 39.198,
    39.2855, 39.2595, 39.4274, 39.3258, 39.3162, 39.4143, 39.3034,
    39.2099, 39.2775, 39.5042, 39.1446, 39.188, 39.2006, 39.1799,
    39.4077, 39.2694, 39.1967, 39.2828, 39.2438, 39.2093, 39.2167,
    39.2749, 39.4703, 39.2846};

std::vector<float> ransac_pick = {               39.009900000000002
    ,       37.909500000000001
    ,       36.765000000000001
    ,       36.362099999999998
    ,       35.726100000000002
    ,       35.065600000000003
    ,       33.252299999999998
    ,       32.318300000000001
    ,       31.007300000000001
    ,       28.522300000000001
    ,       27.5152
    ,       26.819099999999999
    ,       26.311399999999999
    ,       25.816400000000002
    ,       24.763100000000001
    ,       24.8184
    ,       35.9315
    ,       36.808
    ,       37.300199999999997
    ,       37.670000000000002
    ,       38.465000000000003
    ,       38.807600000000001
};


// Support Functions Implementation at the bottom of the file
bool setup_loggers (const std::string& log_path,  std::string id_name);
std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr);
std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename);
void output_array (const std::vector<std::vector<float>>& data, const std::string& output_file);

cv::Mat generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>& voxels,
                                       std::vector<std::vector<float>>& ss);
bool setup_text_loggers (const fs::path app_support_dir, std::string id_name);
SurfaceRef get_surface(const std::string & path);
void norm_scale (const std::vector<double>& src, std::vector<double>& dst);

std::shared_ptr<test_utils::genv> dgenv_ptr;

typedef std::weak_ptr<Surface8u>	Surface8uWeakRef;
typedef std::weak_ptr<Surface8u>	SurfaceWeakRef;
typedef std::weak_ptr<Surface16u>	Surface16uWeakRef;
typedef std::weak_ptr<Surface32f>	Surface32fWeakRef;
typedef boost::filesystem::path path_t;

path_t create_if (const path_t path){
    boost::system::error_code ec;
    if(bfs::exists(path))
        bfs::remove_all(path);
    bfs::create_directory(path, ec);
    return path;
}


#define SHOW(a,b,c) \
{\
std::string _msg (a);\
if (c == 0){_msg += " Space to Cont ";std::cout << _msg << std::endl;}\
cv::namedWindow(_msg.c_str(), cv::WINDOW_NORMAL);\
cv::imshow(_msg.c_str(),(b));\
cv::waitKey((c));\
}




#if 1


    // First check oiio:BitsPerSample int attribute.  If not set,
    // fall back on the TypeDesc. return 0 for float types
    // or those that exceed the int range (long long, etc)
static unsigned long long
get_intsample_maxval(const ImageSpec& spec)
{
    TypeDesc type = spec.format;
    int bits      = spec.get_int_attribute("oiio:BitsPerSample");
    if (bits > 0) {
        if (type.basetype == TypeDesc::UINT8
            || type.basetype == TypeDesc::UINT16
            || type.basetype == TypeDesc::UINT32)
            return ((1LL) << bits) - 1;
        if (type.basetype == TypeDesc::INT8 || type.basetype == TypeDesc::INT16
            || type.basetype == TypeDesc::INT32)
            return ((1LL) << (bits - 1)) - 1;
    }
    
        // These correspond to all the int enums in typedesc.h <= int
    if (type.basetype == TypeDesc::UCHAR)
        return 0xff;
    if (type.basetype == TypeDesc::CHAR)
        return 0x7f;
    if (type.basetype == TypeDesc::USHORT)
        return 0xffff;
    if (type.basetype == TypeDesc::SHORT)
        return 0x7fff;
    if (type.basetype == TypeDesc::UINT)
        return 0xffffffff;
    if (type.basetype == TypeDesc::INT)
        return 0x7fffffff;
    
    return 0;
}



TEST (ut_dm, block){
    
    iPair dims (9,9);
    float sigma = 4.5f;
    cv::Mat fgm = getGaussianKernel(dims.first, dims.second, sigma, sigma);
    cv::normalize(fgm, fgm, 0, 1, cv::NORM_MINMAX);
    fgm *= 255;
    cv::Mat gm(fgm.rows, fgm.cols, CV_8U);
    fgm.convertTo(gm, CV_8U);
    
    iPair fsize(32,64);
    
    cv::Mat fimage (fsize.second, fsize.first,  CV_8UC(1));
    fimage = 0;
    iPair fixed (11,21);
    cv::Rect2i froi(fixed.first,fixed.second,dims.first,dims.second);
    cv::Mat fwindow = fimage(froi);
    cv::add (gm, fwindow, fwindow);
    
    cv::Mat mimage (fsize.second, fsize.first,  CV_8UC(1));
    mimage = 0;
    iPair gold (12,22);
    cv::Rect2i mroi(gold.first,gold.second, dims.first,dims.second);
    cv::Mat mwindow = mimage(mroi);
    cv::add (gm, mwindow, mwindow);
    
    iPair scan (6,6);
    denseMotion ff(fsize, dims / 2, (dims + scan) / 2);
    auto moving = fixed - scan / 2;
    denseMotion::match res;
    ff.update(fimage);
    ff.update(mimage);
    auto check = ff.block_match(fixed, moving, res);
    EXPECT_TRUE(check);
    std::cout << std::boolalpha << check << std::endl;
    
    
//    SHOW("gtmpl", gm, 0);
//    SHOW("fixed", fimage, 0);
//    SHOW("moving", mimage, 0);
    
    
}


TEST(temporal_median, basic){
    
    Mat a(128,512, CV_8U);
    randu(a, Scalar(0), Scalar(255));
    Mat b(128,512, CV_8U);
    randu(b, Scalar(0), Scalar(255));
    Mat c(128,512, CV_8U);
    randu(c, Scalar(0), Scalar(255));
    
    cv::Mat gold = Mat::zeros(128,512, CV_8U);
    for (auto row = 0; row < gold.rows; row++){
        for (auto col = 0; col < gold.cols; col++)
            {
                gold.at<uint8_t>(row,col) = median_of_3(a.at<uint8_t>(row,col), b.at<uint8_t>(row,col), c.at<uint8_t>(row,col));
            }
    }
    
    
    auto ac = a.clone();
    
    EXPECT_TRUE(temporal_medianOf3(a, b, c, ac));
    
    bool check = matIsEqual(gold, ac);
    EXPECT_TRUE(check);
    
    int count = 100;
    auto npixels = count * a.rows * a.cols;
    
    {
        double endtime;
        std::clock_t start;
        start = std::clock();

        for (auto i = 0; i < count; i++)
            EXPECT_TRUE(temporal_medianOf3(a, b, c, ac));

        endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
        auto million_rate = npixels / endtime / 1000000;
        std::cout << " Array Logic Implementation " << endtime  << " Seconds " << million_rate << " Million Pixels Per second " << std::endl;
    }
    
    {
        double endtime;
        std::clock_t start;
        
        start = std::clock();
        for (auto i = 0; i < count; i++)
            {
                cv::Mat gold = Mat::zeros(a.rows, a.cols, CV_8U);
                for (auto row = 0; row < gold.rows; row++){
                    for (auto col = 0; col < gold.cols; col++)
                    {
                    gold.at<uint8_t>(row,col) = median_of_3(a.at<uint8_t>(row,col), b.at<uint8_t>(row,col), c.at<uint8_t>(row,col));
                    }
                }
            }
        
        endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
        auto million_rate = npixels / endtime / 1000000;
        std::cout << " Per Pixel at<> Implementation " << endtime  << " Seconds " << million_rate << " Million Pixels Per second " << std::endl;
    }
    
    
    
}


cv::Mat create_cone_model (int width, int height){
	
	cv::Mat model (width, height, CV_8U);
	float diag = std::sqrt(squareOf(width)+squareOf(height)) / 2.0;
	float w2 = width / 2;
	float h2 = height / 2;
	for (auto y = 0; y < height; y++){
		for (auto x = 0; x < width; x++){
			float d2 = std::sqrt(squareOf(x - w2)+squareOf(y-h2));
			d2 = sigmoid_clip(d2/diag, 0.0f, 1.0f);
			d2 *= 64;
			model.at<uint8_t>(y,x) = uint8_t(d2);
		}
	}
	return model;
}

double find_local_minima(const cv::Mat &input_image, cv::Mat &local_minima, const iPair& e_size = iPair(3,3)){
	cv::Mat eroded_input_image, thresholded_input_image, thresholded_input_8bit;
	
	cv::Mat element = cv::getStructuringElement(
		cv::MORPH_ELLIPSE, cv::Size(2 * e_size.first + 1, e_size.second * 3 + 1),
		cv::Point(e_size.first, e_size.second));
	
	cv::erode(input_image, eroded_input_image, element);
	cv::compare(input_image, eroded_input_image, local_minima, cv::CMP_EQ);

	auto thr = cv::threshold(input_image, thresholded_input_image, 0, 255,
							 cv::THRESH_BINARY_INV | cv::THRESH_OTSU );

	thresholded_input_image.convertTo(thresholded_input_8bit, CV_8U);
	cv::bitwise_and(local_minima, thresholded_input_8bit, local_minima);
	
	return thr;
}


void drawEllipseWithBox(cv::Mat img, cv::RotatedRect box, cv::Scalar color, int lineThickness)
   {
	   assert(!img.empty());

	   box.center = cv::Point2f(box.center.x, box.center.y);
	   box.size.width  = (float)(box.size.width);
	   box.size.height = (float)(box.size.height);

	   ellipse(img, box, color, lineThickness, LINE_AA);

	   Point2f vtx[4];
	   box.points(vtx);
	   for( int j = 0; j < 4; j++ ){
		   cv::line(img, vtx[j], vtx[(j+1)%4], color, lineThickness, LINE_AA);
	   }
   }


TEST(syn, basic){
	
	auto res = dgenv_ptr->asset_path("motion_field_2.png");
	EXPECT_TRUE(res.second);
	EXPECT_TRUE(boost::filesystem::exists(res.first));
	
	cv::Mat img2 = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
	EXPECT_EQ(img2.channels() , 1);
	
	auto res2 = dgenv_ptr->asset_path("motion_field_1.png");
	EXPECT_TRUE(res2.second);
	EXPECT_TRUE(boost::filesystem::exists(res2.first));
	
	cv::Mat img1 = cv::imread(res2.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
	EXPECT_EQ(img1.channels() , 1);
	

	auto fit_ellipse = [](const cv::Mat& img){
		cv::Mat lm = img.clone();
		find_local_minima(img, lm);

		iPair margin (10,10);
		std::vector<Point2f> minimas;
		for (int j = margin.second; j < (img.rows - margin.second); j++)
			for (int i = margin.first; i < (img.cols - margin.first); i++){
			if (img.at<uint8>(j,i) > 0)
				minimas.emplace_back(i,j);
			}
	
		cv::Mat planes[] = {img,img,img};
		cv::Mat display;
		cv::merge(planes, 3, display);

		RotatedRect boxDirect;
	
		boxDirect = fitEllipseDirect(minimas);
		drawEllipseWithBox(display, boxDirect, Scalar(0,0,255), 2);
		std::cout << " Width " << boxDirect.size.width << " Height " << boxDirect.size.height << " Angle " << boxDirect.angle << std::endl;
		
		ellipseShape es(boxDirect);
		std::cout << " a " << es.a << " b " << es.b << " Angle " << es.phi << std::endl;
		
		std::vector<cv::Point2f> ends(2);
		
		ends[0].x = es.x - es.a * std::cos(es.phi);
		ends[0].y = es.y + es.a * std::sin(es.phi);
		ends[1].x = es.x + es.a * std::cos(es.phi);
		ends[1].y = es.y - es.a * std::sin(es.phi);

		std::sort(ends.begin(), ends.end(), [](Point2f& a, Point2f&b){ return a.x < b.x; });
		
		std::vector<cv::Rect> m_rects;
		m_rects.resize(2);
		iPair halfwidths;
		halfwidths.first = ends[0].x;
		halfwidths.second = display.cols - ends[1].x;
		m_rects[0] = cv::Rect(0,0, 2*halfwidths.first, display.rows);
		m_rects[1] = cv::Rect(display.cols-2*halfwidths.second,0, 2*halfwidths.second, display.rows);
		
		
		for (auto pt : ends){
			auto color = pt.x < boxDirect.center.x ? Scalar(0,255,0) : Scalar(255,0,0);
			cv::drawMarker(display, cv::Point(pt.x,pt.y), color);
		}
		
		for (auto rr : m_rects){
			auto color = rr.x < boxDirect.center.x ? Scalar(0,255,0) : Scalar(255,0,0);
			cv::rectangle(display, rr, color);
		}
		
		
		
		
		SHOW("Peaks", lm, 0);
		SHOW("Ellipse", display, 0);
	};
	
	fit_ellipse(img2);
	fit_ellipse(img1);
	
	
}



bool
image_progress_callback(void* opaque, float done)
{
    std::cout << " Progress " << (int)(done * 100) << std::endl;
    return false;
}

//TEST(temporal_median, tiffstack){
//    auto res = dgenv_ptr->asset_path("C2-nd004_all.tif");
//    EXPECT_TRUE(res.second);
//    EXPECT_TRUE(boost::filesystem::exists(res.first));
//    ustring filename (res.first.c_str());
//    std::vector<cv::Mat> src_images;
//
//
//
//    auto build_vector = [&](const ustring& filename, std::vector<cv::Mat>& images){
//        images.resize(0);
//        ImageBuf buf(filename);
//        int nsubimages = buf.nsubimages();
//        const ImageSpec& bspec = buf.spec();
//        int xres = bspec.width;
//        int yres = bspec.height;
//        buf.threads(1);
//
//            // Now read them from cache and display them
//        for (int ss = 0; ss < nsubimages; ss++){
//            buf.reset(filename, ss, 0);
//            std::string datetime = buf.spec().get_string_attribute("DateTime");
//            ROI roi = buf.roi();
//            cv::Mat cvb (yres,xres, CV_16U);
//            cv::Mat cvb8 (yres,xres, CV_8U);
//
//            buf.get_pixels(roi, TypeUInt16, cvb.data);
//            cv::normalize(cvb, cvb8, 0, 255, NORM_MINMAX, CV_8UC1);
//            images.push_back(cvb8);
//        }
//    };
//
//    build_vector(filename, src_images);
//
//    auto output_path = dgenv_ptr->output_path();
//	auto frames_dir = output_path / "frames";
//    ImageBuf buf(filename);
//    int nsubimages = buf.nsubimages();
//
//#if 0
//        // Create the ImageOutput
//    std::unique_ptr<ImageOutput> out = ImageOutput::create (output_path.string());
//
//        // Be sure we can support subimages
//    if (nsubimages > 1 && ! out->supports ("multiimage")) {
//        std::cerr << "Cannot write multiple subimages\n";
//        return;
//    }
//
//    ImageSpec U8spec(src_images[0].cols, src_images[0].rows, 1, TypeUInt8);
//
//        // Open and declare all subimages
//    out->open(output_path.string(), U8spec, ImageOutput::OpenMode::Create);
//
//        // Be sure we can support subimages
//    if (nsubimages > 1 &&  (! out->supports("multiimage") ||
//                           ! out->supports("appendsubimage"))) {
//        std::cerr << "Does not support appending of subimages\n";
//        return;
//    }
//
//    EXPECT_TRUE(out->supports ("multiimage"));
//    EXPECT_TRUE(out->supports ("appendsubimage"));
//
//
//        // Use Create mode for the first level.
//    ImageOutput::OpenMode appendmode = ImageOutput::Create;
//#endif
//
//    static std::mutex _mu;
//        // Write the individual subimages
//    for (int s = 0;  s < nsubimages-2;  ++s) {
//        std::lock_guard<std::mutex> lock(_mu);
//
//        // Produce temporal median of 3 frames
//        cv::Mat dst;
//        if (temporal_medianOf3(src_images[s],src_images[s+1], src_images[s+2], dst)){
//            SHOW(" Median ", dst, 10);
//			std::string fname = "C2_median"+ to_string(s) + ".tif";
//			output_path = frames_dir / fname;
//			cv::imwrite(output_path.c_str(), dst);
//        }
//    }
//
//}


TEST(scale_space, basic){
    
    auto res = dgenv_ptr->asset_path("nd122_u8.tif");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    ustring filename (res.first.c_str());
    std::vector<cv::Mat> src_images;
    
    auto build_vector = [&](const ustring& filename, std::vector<cv::Mat>& images){
        images.resize(0);
        ImageBuf buf(filename);
        int nsubimages = buf.nsubimages();
        const ImageSpec& bspec = buf.spec();
        int xres = bspec.width;
        int yres = bspec.height;
        buf.threads(1);
        
            // Now read them from cache and display them
        for (int ss = 0; ss < nsubimages; ss++){
            buf.reset(filename, ss, 0);
            std::string datetime = buf.spec().get_string_attribute("DateTime");
            ROI roi = buf.roi();
			if (bspec.format ==  TypeUInt16){
				cv::Mat cvb (yres,xres, CV_16U);
				cv::Mat cvb8 (yres,xres, CV_8U);
				
				buf.get_pixels(roi, TypeUInt16, cvb.data);
				cv::normalize(cvb, cvb8, 0, 255, NORM_MINMAX, CV_8UC1);
				images.push_back(cvb8);
			}else if (bspec.format ==  TypeUInt8){
				cv::Mat cvb8 (yres,xres, CV_8U);
				buf.get_pixels(roi, TypeUInt8, cvb8.data);
				images.push_back(cvb8);
			}
			else{
				assert(false);
			}
        }
    };

    build_vector(filename, src_images);

    auto output_path = dgenv_ptr->output_path();
    auto scales_dir = output_path / "scales";
    scales_dir = create_if(scales_dir);
    auto dogs_dir = output_path / "dogs";
    dogs_dir = create_if(dogs_dir);
    
    
    scaleSpace ss;
    ss.generate(src_images, 2, 15, 2);
    
    std::string msg = " ScaleSpace " ;
    int index = 0;
    auto sum = cv::Mat(src_images[0].rows, src_images[0].cols, CV_64F);
    sum = 0;
    for(const auto& rw : ss.space()){
        sum += rw;
        auto grw = cv::Mat(rw.rows,rw.cols,CV_8U);
        cv::normalize(rw,grw,0,255, CV_MINMAX, CV_8U);
        std::string filename = "scale_var_"+ to_string(index) + ".png";
        auto filepath = scales_dir / filename;
        cv::imwrite(filepath.c_str(), grw);
        index++;
        SHOW(filename.c_str(),grw, 100);
    }

    index = 0;
    for(const auto& rw : ss.dog()){
        auto filename = "dog_var_" + to_string(index) + ".png";
        auto filepath = dogs_dir / filename;
        cv::imwrite(filepath.c_str(), rw);
        index++;
        SHOW(filename.c_str(), rw, 100);
        
    }
 //   bool ok = ss.process_motion_peaks();
 //   ASSERT_TRUE(ok);
    
    auto mm = ss.motion_field();
    
    std::string file_name = "motion_field.png";
    auto filepath = output_path / file_name;
    cv::imwrite(filepath.c_str(), mm);
    
    std::vector<cv::Rect> rects;
	std::vector<cv::Point2f> ends;
	bool ok = ss.process_motion_peaks(16);
	
  
	if (ok)
    {
    auto name = " Length ";
    cvplot::setWindowTitle(name, " Over Time");
    cvplot::moveWindow(name, 300, 100);
    cvplot::resizeWindow(name, 1024, 512);
    
    cvplot::figure(name).series(" L(t) ").addValue(ss.lengths());
    cvplot::figure(name).show();
    cv::waitKey();
    }
}


TEST(oiio, basic){
    
    auto test_file = [](const ustring& filename){
        
        ImageBuf buf(filename);
        int nsubimages = buf.nsubimages();
        const ImageSpec& bspec = buf.spec();
        int xres = bspec.width;
        int yres = bspec.height;
        int channels = bspec.nchannels;
        int bps = bspec.get_int_attribute("oiio:BitsPerSample", -1);
        std::cout << xres << "," << yres << "," << channels << ":" << bps << std::endl;
        
            // Now read them from cache and display them
        namedWindow("oiio", WINDOW_AUTOSIZE | WINDOW_OPENGL);
        for (int ss = 0; ss < nsubimages; ss++){
            buf.reset(filename, ss, 0);
            std::string datetime = buf.spec().get_string_attribute("DateTime");
            std::cout << buf.subimage() << " @ " << datetime << std::endl;
            ROI roi = buf.roi();
            cv::Mat cvb (yres,xres, CV_16U);
            cv::Mat cvb8 (yres,xres, CV_8U);
            
            buf.get_pixels(roi, TypeUInt16, cvb.data);
            cv::normalize(cvb, cvb8, 0, 255, NORM_MINMAX, CV_8UC1);
            SHOW("oiio", cvb8, 100);
        }
    };
    
    
   auto res = dgenv_ptr->asset_path("zser16.tif");
   EXPECT_TRUE(res.second);
   EXPECT_TRUE(boost::filesystem::exists(res.first));
   ustring filename (res.first.c_str());
  //  ustring filename( "/Volumes/medvedev/Users/arman/Downloads/nd122.tif");
    test_file(filename);
        
}


    // Wimple wrapper to return a raw "null" ImageInput*.
static ImageInput*
NullInputCreator()
{
        // Note: we can't create it directly, but we can ask for a managed
        // pointer and then release the raw pointer from it.
    return ImageInput::create("0.null").release();
}


TEST(oiio, inmemory){
    // Create 3 synthetic images
    ImageCache* imagecache = ImageCache::create(false /*not shared*/);
    
        // Add a file entry with a "null" ImageInput proxy configured to look
        // like a 2x2 RGB float image.
    ustring fooname("foo");
    const int xres = 2, yres = 2, chans = 3;
    TypeDesc imgtype = TypeDesc::FLOAT;
    ImageSpec config(xres, yres, chans, imgtype);
    config.tile_width  = xres;
    config.tile_height = yres;
    config.attribute("null:force", 1);  // necessary because no .null extension
    bool fadded = imagecache->add_file(fooname, NullInputCreator, &config);
    EXPECT_TRUE(fadded);
    
        // Make sure it got added correctly
    ImageSpec retrieved_spec;
    imagecache->get_imagespec(fooname, retrieved_spec);
    EXPECT_EQ(retrieved_spec.width, xres);
    EXPECT_EQ(retrieved_spec.height, yres);
    EXPECT_EQ(retrieved_spec.format, imgtype);
    
    float pixels[yres][xres][chans] = { { { 0, 0, 0 }, { 0, 1, 0 } },
        { { 1, 0, 0 }, { 1, 1, 0 } } };
        // Add a tile that copies the image.
    bool ok = imagecache->add_tile(fooname, 0 /* subimage */, 0 /* miplevel */,
                                   0, 0, 0,         /* origin */
                                   0, chans,        /* channel range */
                                   imgtype, pixels, /* the buffer */
                                   AutoStride, AutoStride,
                                   AutoStride, /* strides */
                                   true /* COPY THE PIXELS! */);
    EXPECT_TRUE(ok);
    
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(20,10);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(20,10);
    cv::Mat mat1(10, 20, CV_8UC(1), img1.get(), 20);
    cv::Mat mat2(10, 20, CV_8UC(1), img2.get(), 20);
        
    ImageBuf dst = ImageBufAlgo::from_OpenCV(mat1);
    EXPECT_TRUE(!dst.has_error());
    
}



//#include "ut_lif.hpp"

TEST(cluster_2d, basic){
    
    std::vector<point2f_t> points;
    
    for (double x = 0.0 ; x < 10.0 ; x += 1.0)
        for (double y = 0.0 ; y < 10.0 ; y += 1.0)
            points.push_back(point2f_t{x, y});
    
    std::vector<cluster2f_t> clusters;
    
    find_2dclusters(points, 3.0, clusters);
  
#if PRINT_OUT
    for(size_t i = 0 ; i < clusters.size() ; ++i) {
        std::cout << "Cluster " << i << std::endl;
        for (auto const& p : clusters[i]) {
            std::cout << bg::wkt(p) << "==";
        }
        std::cout << std::endl;
    }
#endif
}



TEST(cluster, basic){
    
    std::vector<point3f_t> points;
    cv::Mat emat (10,10,CV_8U);
    
    ellipseShape one;
    one.setEllipse(5.0, 5.0, 3.0, 2.0, svl::constants::pi_2 / 4);
    for (double y = 0.0 ; y < 10.0 ; y += 1.0){
        for (double x = 0.0 ; x < 10.0 ; x += 1.0){
            double rz = one.insideEllipse(x,y) ? one.normalizedDistanceFromOrigin(x, y) : 1.0;
            emat.at<uint8_t>(y,x) = (uint8_t)(100*rz);
            std::cout << (int)(10*rz) << '\t';
            if(x == 9.0) std::cout << std::endl;
        }
    }
    svl::momento bm(emat, false);
    svl::momento gm(emat, true);
    std::cout << bm.com() << '\t' << bm.getEllipseAspect() << '\t' << bm.getOrientation().Double() << std::endl;
    std::cout << gm.com() << '\t' << gm.getEllipseAspect() << '\t' << gm.getOrientation().Double() << std::endl;


    
    std::vector<cluster3f_t> clusters;
    
    find_3dclusters(points, 3.0f, 0.3f, clusters);
    
#if PRINT_OUT
    for(size_t i = 0 ; i < clusters.size() ; ++i) {
        if (clusters[i].empty()) continue;
        std::cout << "Cluster " << i << std::endl;
        for (auto const& p : clusters[i]) {
            std::cout << bg::wkt(p) << std::endl;
        }
    }
#endif
    
}

TEST(chull, basic){
    typedef bg::model::point<float, 2, bg::cs::cartesian> point_2d;
    typedef bg::model::box<point_2d> box_2d;
    typedef bg::model::polygon<point_2d> polygon_2d;
    typedef boost::shared_ptr<polygon_2d> shp_2d;
    typedef std::pair<box_2d, shp_2d> value_2d;
    
    // create the rtree using default constructor
    bgi::rtree< value_2d, bgi::linear<16, 4> > rtree;
    
    // create some polygons and fill the spatial index
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // create a polygon
        shp_2d p_ref (new polygon_2d ());
        for ( float a = 0 ; a < 6.28316f ; a += 1.04720f )
        {
            float x = i + int(10*::cos(a))*0.1f;
            float y = i + int(10*::sin(a))*0.1f;
            p_ref->outer().push_back(point_2d(x, y));
        }

        // display new polygon
#if PRINT_OUT
        std::cout << bg::wkt<polygon_2d>(*p_ref) << std::endl;
#endif
        // calculate polygon bounding box
        box_2d b = bg::return_envelope<box_2d>(*p_ref);
        // insert new value
        rtree.insert(std::make_pair(b, p_ref));
        
        polygon_2d hull;
        boost::geometry::convex_hull(*p_ref, hull);
    
        using boost::geometry::dsv;
#if PRINT_OUT
        std::cout
        << "polygon: " << dsv(*p_ref) << std::endl
        << "hull: " << dsv(hull) << std::endl
        ;
#endif
    }
    // find values intersecting some area defined by a box
    box_2d query_box(point_2d(0, 0), point_2d(5, 5));
    std::vector<value_2d> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));
    
    // find 5 nearest values to a point
    std::vector<value_2d> result_n;
    rtree.query(bgi::nearest(point_2d(0, 0), 5), std::back_inserter(result_n));
    
    // note: in Boost.Geometry the WKT representation of a box is polygon
    
    // note: the values store the bounding boxes of polygons
    // the polygons aren't used for querying but are printed
    
    // display results
    std::cout << "spatial query box:" << std::endl;
    std::cout << bg::wkt<box_2d>(query_box) << std::endl;
    std::cout << "spatial query result:" << std::endl;
    BOOST_FOREACH(value_2d const& v, result_s)
    std::cout << bg::wkt<polygon_2d>(*v.second) << std::endl;
    
    std::cout << "knn query point:" << std::endl;
    std::cout << bg::wkt<point_2d>(point_2d(0, 0)) << std::endl;
    std::cout << "knn query result:" << std::endl;
    BOOST_FOREACH(value_2d const& v, result_n)
    std::cout << bg::wkt<polygon_2d>(*v.second) << std::endl;


}


TEST(zscore, basic){
    auto res = dgenv_ptr->asset_path("voxel_ss_.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat src = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);

    roiWindow<P8U> r8(src.cols, src.rows);
    cpCvMatToRoiWindow8U (src, r8);
    auto tres = zscore (r8);
#if PRINT_OUT
    for (const auto& kv : tres) {
        std::cout << unsigned(kv.first) << " has value " << kv.second << std::endl;
    }
#endif
    std::vector<sliceSimilarity::slice_result_t> ires;
    auto sres = sliceSimilarity::threshold(r8, ires);
    std::cout << unsigned(sres.val) << " @ " <<  sres.corr << " w  " << sres.weight << std::endl;

    
}


TEST(ut_labelBlob, mult_level)
{
    using blob = svl::labelBlob::blob;
    
    static bool s_results_ready = false;
    static bool s_graphics_ready = false;
    static int64_t cid = 0;
    
    auto res = dgenv_ptr->asset_path("voxel_ss_.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat src = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    EXPECT_EQ(src.channels() , 1);
    EXPECT_EQ(src.cols , 510);
    EXPECT_EQ(src.rows , 126);

    roiWindow<P8U> r8(src.cols, src.rows);
    cpCvMatToRoiWindow8U (src, r8);
    
    histoStats hh;
    hh.from_image<P8U>(r8);
#if PRINT_OUT
    std::cout << hh;
#endif
    std::vector<uint8_t> valid_bins;
    valid_bins.resize(0);
    for (unsigned binh = 0; binh < hh.histogram().size(); binh++)
    {
        if (hh.histogram()[binh] > 0 ) valid_bins.push_back((uint8_t) binh);
    }
    //auto width = r8.width();
    //auto height = r8.height();
    
    // Create lut mapps
    std::vector<uint8_t> backtoback(512, 0);
    for (int ii = 256; ii < 512; ii++)
        backtoback[ii] = 1;
    
    // Start at mid point between 0000 and 1111
    std::vector<uint8_t>::const_iterator pmMid = backtoback.begin();
    std::advance(pmMid, 256);
    namedWindow( " Multi Label ", WINDOW_AUTOSIZE | WINDOW_OPENGL);
    
    for (unsigned tt = 0; tt < valid_bins.size(); tt++)
    {
        s_results_ready = false;
        s_graphics_ready = false;
        cid = 0;
        
        auto tmp = extractAtLevel(r8, tt);
        cv::Mat mask (tmp.height(), tmp.width(), CV_8UC(1), tmp.pelPointer(0,0), size_t(tmp.rowUpdate()));
        
        labelBlob::ref lbr = labelBlob::create(src, mask, 6, 666);
        EXPECT_EQ(lbr == nullptr , false);
        std::function<labelBlob::results_ready_cb> res_ready_lambda = [](int64_t& cbi){ s_results_ready = ! s_results_ready; cid = cbi;};
        std::function<labelBlob::graphics_ready_cb> graphics_ready_lambda = [](){ s_graphics_ready = ! s_graphics_ready;};
        boost::signals2::connection results_ready_ = lbr->registerCallback(res_ready_lambda);
        boost::signals2::connection graphics_ready_ = lbr->registerCallback(graphics_ready_lambda);
        EXPECT_EQ(false, s_results_ready);
        EXPECT_EQ(true, cid == 0);
        EXPECT_EQ(true, lbr->client_id() == 666);
        lbr->run();
        while(!s_results_ready){std::this_thread::yield(); }
        
        EXPECT_EQ(true, s_results_ready);
        EXPECT_EQ(true, lbr->client_id() == 666);
        EXPECT_EQ(true, cid == 666);
        EXPECT_EQ(true, lbr->hasResults());
        const std::vector<blob> blobs = lbr->results();
#if PRINT_OUT
        std::cout << "["<< tt << "]: " << blobs.size() << std::endl;
#endif
        
        try{
            lbr->drawOutput();
            while(! s_graphics_ready){ std::this_thread::yield(); }
            EXPECT_EQ(true, s_graphics_ready);
            /// Show in a window
            std::string msg = " LabelBlob " + toString(tt) + " ";
            SHOW(msg, lbr->graphicOutput(), 100);
        }
        catch(const std::exception & ex)
        {
            std::cout <<  ex.what() << std::endl;
        }
    }
}


TEST(samples_1d, basic){
    
    auto res = dgenv_ptr->asset_path("signal.csv");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    record_t array;
    load_record(res.first.string(), array, false);
    EXPECT_EQ(size_t(500), array.size());
    

    
    std::vector<double> dst_1;
    dst_1.insert(dst_1.end(),array.begin(), array.end());

#if NOTYET
    auto build_index_data = [=] (std::vector<double>& signal){
        std::list<svl::vec_2<double>> out;
        int index = 0;
        for(const auto &val : signal){
            svl::vec_2<double> iv (index++,val);
            out.push_back(iv);
        }
        return out;
    };
        
    auto convert_to = [=] (std::vector<double>& signal){
        auto out = build_index_data(signal);
        svl::samples_1D<double> ss (out);
        return ss;
    };
    
    auto ss = convert_to(dst_1);
    auto fd = ss.Derivative_Forward_Finite_Differences();
    auto bd = ss.Derivative_Forward_Finite_Differences();
    auto peaks = ss.Peaks();
    auto to_vector = [] (const svl::samples_1D<double>& src){
        std::vector<double> out(src.samples.size());
        for (auto& s : src.samples){
            out[s[0]] = s[2];
        }
        return out;
    };
    
    auto fp = to_vector(peaks);

    
    bool ok = false;
    auto lm = ss.Linear_Least_Squares_Regression(&ok, true);
    std::cout << lm.slope << "  offset  " << lm.intercept << std::endl;
#endif
    
    lsFit<float> lsf;
    float index = 0.0f;
    for (auto pt : dst_1){
        float fpt = pt;
        lsf.update(index,fpt);
        index += 1.0f;
    }
    auto line = lsf.fit();
    uRadian ang = line.first;
    std::cout << ang.Double() << " dist " << line.second << std::endl;
    
    // Poly fitting
    std::vector<double> mathematica_coeffs = {0.742154, 0.00125496, -3.79331e-6};
    std::vector<double> coeffs;
    auto iB = dst_1.begin();
    std::advance(iB, 261);
    polyfit(iB, dst_1.end(), 1.0, coeffs, 2);
    EXPECT_TRUE(coeffs.size() == mathematica_coeffs.size());
    
    for (auto ii = 0; ii < coeffs.size(); ii++)
        EXPECT_TRUE(svl::equal(mathematica_coeffs[ii], coeffs[ii], 1.e-05));
    std::cout << coeffs << std::endl;
    
    polyfit (dst_1, 1.0, coeffs, 2);
    auto pfit = [coeffs](double x){
        return coeffs[0] - coeffs[1]*x + coeffs[2]*x*x;
    };
    
}





TEST(cardiac_ut, load_sm)
{
    auto res = dgenv_ptr->asset_path("sm.csv");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    vector<vector<double>> array;
    load_sm(res.first.string(), array, false);
    EXPECT_EQ(size_t(500), array.size());
    for (auto row = 0; row<500; row++){
        EXPECT_EQ(size_t(500), array[row].size());
        EXPECT_EQ(1.0,array[row][row]);
    }
    
    
}




TEST(cardiac_ut, locate_contractions)
{
    auto res = dgenv_ptr->asset_path("avg_baseline25_28_length_length_pct_short_pct.csv");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    vector<vector<double>> array;
    load_sm(res.first.string(), array, false);
    EXPECT_EQ(size_t(500), array.size());
    for (auto row = 0; row<500; row++){
        EXPECT_EQ(size_t(5), array[row].size());
    }
    
    data_t dst;
    flip(array,dst);
    
    // Persistence of Extremas
    persistence1d<double> p;
    vector<double> dst_1;
    dst_1.insert(dst_1.end(),dst[1].begin(), dst[1].end());
    
    // Exponential Smoothing
    eMAvg<double> emm(0.333,0.0);
    vector<double> filtered;
    for (double d : dst_1){
        filtered.push_back(emm.update(d));
    }
    
    auto median_filtered_val = Median(filtered);
    p.RunPersistence(filtered);
    std::vector<int> tmins, lmins, tmaxs, lmaxs;
    p.GetExtremaIndices(tmins,tmaxs);
    std::cout << " Median val" << median_filtered_val << std::endl;

    for (auto tmi : tmins){
        if (dst_1[tmi] > median_filtered_val) continue;
        lmins.push_back(tmi);
    }
    
    for (auto tma : tmaxs){
        if (dst_1[tma] < median_filtered_val) continue;
        lmaxs.push_back(tma);
    }
    
    std::sort(lmins.begin(),lmins.end());
    std::sort(lmaxs.begin(),lmaxs.end());

    std::vector<std::pair<float, float>> contractions;
    for (auto lmi : lmins){
        contractions.emplace_back(lmi,dst_1[lmi]);
    }
    {
        auto name = "Contraction Localization";
        cvplot::setWindowTitle(name, "Contraction");
        cvplot::moveWindow(name, 300, 100);
        cvplot::resizeWindow(name, 1024, 512);
        cvplot::figure(name).series("Raw").addValue(dst_1);
        cvplot::figure(name).series("filtered").addValue(filtered);
        cvplot::figure(name).series("contractions").set(contractions).type(cvplot::Dots).color(cvplot::Red);
        cvplot::figure(name).show();
    }
    
}



TEST(ut_gsl, basic){
    double x = 5.0;
    double y = gsl_sf_bessel_J0 (x);
    printf ("J0(%g) = %.18e\n", x, y);
}


static double ransacZ(const std::vector<double> &Z) {
    return etw_utils::ransac(
                         Z, []() -> double { return 0.0; },
                         [](double z, double zest) { return std::abs(z - zest) < 0.03; });
}

static Eigen::VectorXd getZPlanes(const std::vector<double> &z) {
    std::vector<double> zCoords(z.begin(), z.end());
    Eigen::VectorXd domZs = Eigen::VectorXd::Zero(30);
    int count = 0;
    do {
        double z0 = ransacZ(zCoords);
        
        zCoords.erase(
                      std::remove_if(zCoords,
                                     [&z0](auto &z) { return std::abs(z - z0) < 0.03; }),
                      zCoords.end());
        
        domZs[count++] = z0;
#if PRINT_OUT
        std::cout << domZs.minCoeff() << std::endl;
#endif
        
    } while (domZs.minCoeff() <= 0 || count < 2);
    
    std::vector<float> fcp;
    for (auto & dd : zCoords)fcp.push_back((float)dd);
    
        auto cvDrawPlot = [] (std::vector<float>& tmp){
    
            std::string name = svl::toString(std::clock());
            cvplot::setWindowTitle(name, svl::toString(tmp.size()));
            cvplot::moveWindow(name, 0, 256);
            cvplot::resizeWindow(name, 512, 256);
            cvplot::figure(name).series(name).addValue(tmp).type(cvplot::Line).color(cvplot::Red);
            cvplot::figure(name).show();
        };
    
    cvDrawPlot(oneF_example);
    cvDrawPlot(fcp);
    
    
    return domZs;
}

TEST(ut_ransac, basic){
    auto domz = getZPlanes(oneD_example);
#if PRINT_OUT
    std::cout << domz << std::endl;
#endif
    
}


/**
 Gets the first dominate direction.  Dominate direction extraction
 is done using RANSAC.  N is all normals and M is the ouput
 */
void ransacManhattan1(const std::vector<Eigen::Vector3d> &N,
                      Eigen::Vector3d &M) {
    M = etw_utils::ransac(
                      N, []() -> Eigen::Vector3d { return Eigen::Vector3d::Zero(); },
                      [](auto &n, auto &nest) {
                          return std::fabs(nest.z() - n.z()) > 1;
                      },
                      [](const auto &ave, const auto &n, const auto &est) -> Eigen::Vector3d {
                          if (std::fabs(est.z() - n.z()) < 1)
                              return ave - n;
                          else
                              return ave + n;
                      });
}

TEST(ut_ransac, plane){
    auto res = dgenv_ptr->asset_path("voxel_ss_.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat image = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    cv::Size iSize(image.cols,image.rows);
    
    std::vector<Eigen::Vector3d> normals;
    normals.reserve(image.rows*image.cols);
    for(auto row = 0; row < image.rows; row++){
        for (auto col = 0; col < image.cols; col++){
            normals.emplace_back(col, row, (double) image.at<uint8_t>(row,col));
        }
    }
    
    std::vector<Eigen::Vector3d> M(3);
    ransacManhattan1(normals, M[0]);
    
    std::cout << "D1: " << M[0][2] << std::endl << std::endl;
}

TEST (ut_affine_translation, basic){
    
    auto res = dgenv_ptr->asset_path("affine_translation.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat image = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    cv::Size iSize(image.cols,image.rows);
    
    auto trans = [] (const cv::Mat& minus, const cv::Mat& plus){
        auto tw = minus.cols / 3;
        auto th = minus.rows / 3;
        auto width = minus.cols;
        auto height = minus.rows;
        auto cw = (width - tw)/2;
        auto ch = (height-th)/2;
        auto left = cv::Mat(minus, cv::Rect(tw,ch,tw,th));
        auto right = cv::Mat(minus, cv::Rect(cw,ch,tw,th));
        auto plus_left = cv::Mat(plus, cv::Rect(0,0,plus.cols/2, plus.rows));
        auto plus_right = cv::Mat(plus, cv::Rect(plus.cols/2,0,plus.cols/2, plus.rows));
        
        cv::Mat norm_xcorr_img;
        std::pair<cv::Point,cv::Point> max_loc;
        dPair max_val;
        cv::matchTemplate(plus_left,left, norm_xcorr_img, cv::TM_CCORR_NORMED, left);
        cv::minMaxLoc(norm_xcorr_img, NULL, &max_val.first, NULL, &max_loc.first);
        cv::matchTemplate(plus_right,right, norm_xcorr_img, cv::TM_CCORR_NORMED, right);
        cv::minMaxLoc(norm_xcorr_img, NULL, &max_val.second, NULL, &max_loc.second);
        fVector_2d lv(max_loc.first.x,max_loc.first.y);
        fVector_2d rv(max_loc.second.x+plus.cols/2.0f,max_loc.second.y);
        return lv.distance(rv);
    };
    
    auto d0 = trans(image,image);
    std::cout << d0 << std::endl;
    res = dgenv_ptr->asset_path("affine_translation_close.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat close_image = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    cv::Size icSize(close_image.cols,close_image.rows);
    auto d1 = trans(image, close_image);
    std::cout << d0 - d1 << std::endl;
    // @todo add check for 108.046 for first and change to -8.06
    
    
}
TEST (ut_algo_lif, segment){
    
    auto res = dgenv_ptr->asset_path("voxel_ss_.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat image = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    cv::Size iSize(image.cols,image.rows);
    cv::Size plusOne(iSize.width+1,iSize.height+1);
    
    cv::Mat  sMat (plusOne, CV_32SC1);
    cv::Mat  ssMat (plusOne, CV_64FC1);
    cv::integral (image, sMat, ssMat);
    
    auto indexMat = [](cv::Size& dims){
        cv::Mat indexmat (dims, CV_32FC1);
        auto generate = [](int start, std::vector<float>& vf){
            static float step = 1.0f;
            step = 1.0f;
            auto init = [&](float& elem){ elem = start + step++; };
            std::for_each(vf.begin(), vf.end(), init );
        };

        std::vector<float> fv(indexmat.cols);
        for (auto row = 0; row < indexmat.rows; row++){
            int st = row * indexmat.cols;

            generate(st, fv);
            std::copy(fv.begin(), fv.end(),indexmat.ptr<float>(row));
        }
        return indexmat;
    };

    cv::Size ts(plusOne);
    auto test = indexMat(ts);
    double minv = 0, maxv = 0;
    cv::Point minl, maxl;
  

    cv::Mat snorm, slog, tlog;
    sMat.convertTo(snorm, CV_32F);
    cv::log(snorm, slog);
    cv::log(test, tlog);
    cv::Mat dlog(plusOne, CV_8U);
    cv::subtract(slog, tlog, dlog, noArray(), CV_8U);
    cv::normalize(dlog, dlog,0.0,255.0, cv::NORM_MINMAX);
    cv::divide(snorm,test,test);
    cv::minMaxLoc(test, &minv, &maxv, &minl, &maxl);
    cv::normalize(test, test,0.0,255.0, cv::NORM_MINMAX);

    cv::Mat ut8;
    test.convertTo(ut8, CV_8U);
    
    SHOW("divide", ut8, 30);
    SHOW( "log", dlog, 30);
    SHOW( "image", image, 30);

    auto filename = "iratio.png";
    auto file_path = dgenv_ptr->output_path() / filename;
    cv::imwrite(file_path.c_str(), ut8);
    filename = "firatio.png";
    file_path = dgenv_ptr->output_path() / filename;
    cv::imwrite(file_path.c_str(), test);
    filename = "dlog.png";
    file_path = dgenv_ptr->output_path() / filename;
    cv::imwrite(file_path.c_str(), dlog);

    
#if PRINT_OUT
    auto printMat = [](const cv::Mat& m){
        for (auto row = 0; row < m.rows; row++){
            for_each(m.ptr<float>(row), m.ptr<float>(row)+m.cols, [](float elem) { std::cout << " | " << elem; });
            std::cout << std::endl;
        }
    };
    printMat(test);
#endif

}

std::vector<Point2f> ellipse_test = {
    {666.895422,372.895287},
    {683.128254,374.338401},
    {698.197455,379.324060},
    {735.650898,381.410000},
    {726.754550,377.262089},
    {778.094505,381.039470},
    {778.129881,381.776216},
    {839.415543,384.804510}};


TEST (ut_rotated_rect, basic){
    using vec2=glm::vec2;
    
    std::vector<vec2> points = {{5.5f,3.5f},{7.5f,2.5f},{9.5f,3.5f},{5.5f,6.5f}};
    Point2f center_gold (11.5,2.5);
    std::vector<Point2f> Points = {{5.5f,2.5f},{7.5f,2.5f},{9.5f,2.5f},{5.5f,6.5f}};
    std::vector<Point2f> Points2 = {{5.5f,6.5f},{7.5f,6.5f},{9.5f,6.5f},{5.5f,3.5f}};
    std::vector<Point2f> Points3 = {{5.5f,6.5f},{7.5f,6.5f},{7.5f,8.5f},{7.5f,12.5f}};
    Point2f cvcenter_gold (11.5,2.5);
    
#if cinder_poly_line_not_solved
    ci::PolyLine2 pl(points);
    bool is_colinear = true;
    bool isCCW = pl.isCounterclockwise(&is_colinear);
    EXPECT_TRUE(!is_colinear);
    EXPECT_TRUE(isCCW);
    auto com = pl.calcCentroid();
    auto area = pl.calcArea();
    std::cout << com << "  ,   " << area << std::endl;
#endif
    
    {
        cv::RotatedRect rr;
        pointsToRotatedRect(Points, rr);
        std::cout << rr.angle << std::endl;
        std::cout << rr.center << std::endl;
        std::cout << rr.size << std::endl;
    }
    
    {
        cv::RotatedRect rr;
        pointsToRotatedRect(Points2, rr);
        std::cout << rr.angle << std::endl;
        std::cout << rr.center << std::endl;
        std::cout << rr.size << std::endl;
    }
    
    
    {
        cv::RotatedRect rr;
        pointsToRotatedRect(Points3, rr);
        std::cout << rr.angle << std::endl;
        std::cout << rr.center << std::endl;
        std::cout << rr.size << std::endl;
    }
    
}


TEST(serialization, eigen){
    Eigen::MatrixXd test = Eigen::MatrixXd::Random(10, 3);
    
    {
        std::ofstream out("eigen.cereal", std::ios::binary);
        cereal::BinaryOutputArchive archive_o(out);
        archive_o(test);
    }
    
    std::cout << "test:" << std::endl << test << std::endl;
    
    Eigen::MatrixXd test_loaded;
    
    {
        std::ifstream in("eigen.cereal", std::ios::binary);
        cereal::BinaryInputArchive archive_i(in);
        archive_i(test_loaded);
    }
    
    std::cout << "test loaded:" << std::endl << test_loaded << std::endl;
}

TEST(ut_voxel_freq, basic){
    EXPECT_TRUE(fft1D::test());
}
TEST(ut_permutation_entropy, n_2){
    std::vector<double> times_series = {4/12.0,7/12.0,9/12.0,10/12.0,6/12.0,11/12.0,3/12.0};
    {
        auto pe_array_stats = permutation_entropy::permutation_entropy_array_stats(times_series, 2);
        auto pe_dict_stats = permutation_entropy::permutation_entropy_dictionary_stats(times_series, 2);
        std::cout << pe_array_stats << std::endl;
        std::cout << pe_dict_stats << std::endl;
    }
    {
        auto pe_array_stats = permutation_entropy::permutation_entropy_array_stats(times_series, 3);
        auto pe_dict_stats = permutation_entropy::permutation_entropy_dictionary_stats(times_series, 3);
        std::cout << pe_array_stats << std::endl;
        std::cout << pe_dict_stats << std::endl;
    }
}

TEST(ut_serialization, ssResultContainer){
    uint32_t cols = 21;
    uint32_t rows = 21;
    deque<deque<double>> sm;
    for (auto j = 0; j < rows; j++){
        deque<double> row;
        for (auto i = 0; i < cols; i++)
            row.push_back(1.0 / (i + j + 1));
        sm.push_back(row);
    }
    
    // Create sin signals
    auto sinvecD = [](float step, uint32_t size, std::deque<double>& out){
        out.clear ();
        for (auto i : irange(0u, size)) {
            out.push_back(sin(i * 3.14159 *  step));
        }
    };
    
    deque<double> entropy;
    sinvecD(1.0, rows, entropy);
    ssResultContainer ssr;
    ssr.load(entropy, sm);
    auto tempFilePath = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    bool ok = ssResultContainer::store(tempFilePath, entropy, sm);
    EXPECT_TRUE(ok);
    
    auto ssr_new_ref = ssResultContainer::create(tempFilePath.string());
    EXPECT_TRUE(ssr.is_same(*ssr_new_ref));
    
}

TEST (ut_dm, basic){
    roiWindow<P8U> p0 (320, 240);
    roiWindow<P8U> pw(p0, 80, 60, 160, 120);
    p0.set(123.0);
    pw.set(233.0);
    auto p1 = p0.clone();
    p1.set(123.0);
    roiWindow<P8U> pw1(p1, 81, 61, 160, 120);
    pw1.set(233.0);
    auto p2 = p1.clone();
    Gauss3by3 (p2, p1);
    
    cv::Mat im0 (p0.height(), p0.width(), CV_8UC(1), p0.pelPointer(0,0), size_t(p0.rowUpdate()));
    cv::Mat im1 (p1.height(), p1.width(), CV_8UC(1), p1.pelPointer(0,0), size_t(p1.rowUpdate()));
    
    iPair frame_size (p0.width(),p0.height());
    iPair fixed_half_size(16,16);
    iPair moving_half_size(24,24);
    denseMotion ff(frame_size, fixed_half_size, moving_half_size);
    iPair fixed = ff.fixed_size();
    iPair moving = ff.moving_size();
    
    EXPECT_TRUE(fixed == iPair(2*fixed_half_size.first+1,2*fixed_half_size.second+1));
    EXPECT_TRUE(moving == iPair(2*moving_half_size.first+1,2*moving_half_size.second+1));
    
    CorrelationParts cp;
    ff.update(im0);
    ff.update(im0);
    iPair fixed_tl (65, 45);
    auto r = ff.point_ncc_match(fixed_tl, fixed_tl, cp);
    EXPECT_TRUE(svl::equal(r,1.0));
    ff.update(im1);
    std::vector<std::vector<int>> space;
    for (int j = -1; j < 4; j++){
        std::vector<int> rs;
        for (int i = -1; i < 4; i++){
            iPair tmp(fixed_tl.first+i, fixed_tl.second+j);
            auto r = ff.point_ncc_match(fixed_tl, tmp,cp);
            int score = int(r*1000);
            rs.push_back(score);
        }
        space.push_back(rs);
    }
    
    EXPECT_TRUE(space[1][1] == 908);
    EXPECT_TRUE(space[1][3] == 905);
    EXPECT_TRUE(space[3][1] == 905);
    EXPECT_TRUE(space[3][3] == 905);
    
    EXPECT_TRUE(space[2][1] == 942);
    EXPECT_TRUE(space[2][2] == 980);
    EXPECT_TRUE(space[2][3] == 942);
    EXPECT_TRUE(space[1][2] == 942);
    EXPECT_TRUE(space[3][2] == 942);
    
    // Cheap Parabolic Interpolation
    auto x = ((space[2][3]-space[2][1])/2.0) / (space[2][3]+space[2][1]-space[2][2]);
    auto y = ((space[1][2]-space[3][2])/2.0) / (space[1][2]+space[3][2]-space[2][2]);
    cv::Point loc(x,y);

#if PRINT_OUT
    std::cout << std::endl;
    for(std::vector<int>& row : space){
        for (const int& score : row){
            std::cout << setw(4) << score << '\t';
        }
        std::cout << std::endl;
    }
#endif
    
    //    {
    //        auto name = "Motion";
    //        namedWindow(name, CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    //        cv::Point p0 (fixed_tl.first,fixed_tl.second);
    //        cv::Point p1 (p0.x+fixed.first, p0.y+fixed.second);
    //        rectangle(im1,p0,p1, CV_RGB(20,150,20));
    //        circle(im1,loc+p0, 5, CV_RGB(255,255,255));
    //        imshow( "im0", im0);
    //        cv::waitKey(-1);
    //        imshow( "im1", im1);
    //        cv::waitKey(-1);
    //    }
    
    
    
}


TEST (ut_fit_ellipse, local_maxima){
    
    auto same_point = [] (const Point2f& a, const Point2f& b, float eps){return svl::equal(a.x, b.x, eps) && svl::equal(a.y, b.y, eps);};
    Point2f ellipse_gold (841.376, 386.055);
    RotatedRect box0 = fitEllipse(ellipse_test);
    EXPECT_TRUE(same_point(box0.center, ellipse_gold, 0.05));
    
    std::vector<Point2f> gold = {{5.5f,2.5f},{7.5f,2.5f},{9.5f,2.5f},{12.5f,4.5f},{13.5f,6.5f}};
    Point2f center_gold (11.5,2.5);
    
    const char * frame[] =
    {
        "00000000000000000000",
        "00000000000000000000",
        "00001020300000000000",
        "00000000000000000000",
        "00000000012300000000",
        "00000000000000000000",
        "00000000000054100000",
        "00000000000000000000",
        "00000000000000000000",
        0};
    
    roiWindow<P8U> pels(20, 5);
    DrawShape(pels, frame);
    cv::Mat im (pels.height(), pels.width(), CV_8UC(1), pels.pelPointer(0,0), size_t(pels.rowUpdate()));
    
    std::vector<cv::Point> peaks;
    PeakDetect(im, peaks);
    EXPECT_EQ(peaks.size(),gold.size());
    for (auto cc = 0; cc < peaks.size(); cc++)
        EXPECT_TRUE(same_point(peaks[cc], gold[cc],0.05));
    
    std::vector<cv::Point2f> fpeaks;
    for (auto& peak : peaks){
        fpeaks.emplace_back(peak.x+0.5,peak.y+0.5);
    }
    
    RotatedRect box = fitEllipse(fpeaks);
    EXPECT_TRUE(same_point(box.center, center_gold, 0.05));
    EXPECT_TRUE(svl::equal(box.angle, 60.85f, 1.0f));
    
}



TEST(ut_median, basic){
    std::vector<double> dst;
    bool ok = rolling_median_3(oneD_example.begin(), oneD_example.end(), dst);
    EXPECT_TRUE(ok);
    
}

void done_callback (void)
{
    std::cout << "Done"  << std::endl;
}



cv::Mat show_cv_angle (const cv::Mat& src, const std::string& name){
    cv::Mat mag, ang;
    svl::sobel_opencv(src, mag, ang, 7);
    
    SHOW(name.c_str(), ang, 0);
    cv::waitKey();

    return ang;
    
}


void show_gradient (const cv::Mat& src, const std::string& name){
    cv::Mat disp3c(src.size(), CV_8UC3, Scalar(255,255,255));
    
    roiWindow<P8U> r8(src.cols, src.rows);
    cpCvMatToRoiWindow8U (src, r8);
    roiWindow<P8U> mag = r8.clone();
    roiWindow<P8U> ang = r8.clone();
    roiWindow<P8U> peaks = r8.clone();
    std::vector<svl::feature> features;
    Gradient(r8,mag, ang);
    auto pks = SpatialEdge(mag, ang, features, 2);
    
    cvMatRefroiP8U(peaks, mim, CV_8UC1);
    cvMatRefroiP8U(ang, aim, CV_8UC1);
    
    
    auto rt = rightTailPost(mim, 0.005f);
    std::cout << pks << "    " << rt << std::endl;
    
    auto vec2point = [] (const fVector_2d& v, cv::Point& p) {p.x = v.x(); p.y = v.y();};
    
    for (auto fea : features){
        cv::Point pt;
        vec2point(fea.position(),pt);
        svl::drawCross(disp3c, pt,cv::Scalar(CVCOLOR_RED), 3, 1);
    }
    
    SHOW(name.c_str(), disp3c, 0);
    cv::waitKey();
    
}

TEST (ut_opencvutils, difference){
    auto res = dgenv_ptr->asset_path("image230.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat fixed = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    EXPECT_EQ(fixed.channels() , 1);
    EXPECT_EQ(fixed.cols , 512);
    EXPECT_EQ(fixed.rows , 128);
    
    auto res2 = dgenv_ptr->asset_path("image262.png");
    EXPECT_TRUE(res2.second);
    EXPECT_TRUE(boost::filesystem::exists(res2.first));
    cv::Mat moving = cv::imread(res2.first.c_str(),cv::ImreadModes::IMREAD_GRAYSCALE);
    EXPECT_EQ(moving.channels() , 1);
    EXPECT_EQ(moving.cols , 512);
    EXPECT_EQ(moving.rows , 128);
    
    CImg<unsigned char> cFixed (fixed);
    CImg<unsigned char> cFixedPast(fixed.clone());
    CImg<unsigned char> cMoving (moving);
    CImg<unsigned char> cMovingPast(moving.clone());
    
    //    const float sharpness=0.33f;
    //    const float anisotropy=0.6f;
    //    const float alpha=30.0f;
    //    const float sigma=1.1f;
    //    const float dl=0.8f;
    //    const float da=3;
    auto mImg = cMoving.blur_anisotropic( 10, 0.8, 3);
    auto mout = mImg.get_MAT();
    auto fImg = cFixed.blur_anisotropic( 10, 0.8, 3);
    auto fout = fImg.get_MAT();
    
    //  show_gradient(fout, " Fixed ");
    //  show_gradient(mout, " Moving ");
    
    auto fang = show_cv_angle(fout, " Fixed ");
    auto mang = show_cv_angle(mout, " Moving ");
    
}



TEST (ut_3d_per_element, standard_dev)
{
    
    roiWindow<P8U> pels0(5, 4); pels0.set(0);
    roiWindow<P8U> pels1(5, 4); pels1.set(1);
    roiWindow<P8U> pels2(5, 4); pels2.set(2);
    
    
    vector<roiWindow<P8U>> frames {pels0, pels1, pels2};
    
    auto test_allpels = [] (cv::Mat& img, float val, bool show_failure){
        for(int i=0; i<img.rows; i++)
            for(int j=0; j<img.cols; j++)
                if (! svl::equal(img.at<float>(i,j), val, 0.0001f )){
                    if(show_failure)
                        std::cout << " rejected value: " << img.at<float>(i,j) << std::endl;
                    return false;
                }
        return true;
    };
    
    cv::Mat m_sum, m_sqsum;
    int image_count = 0;
    std::atomic<bool> done;
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(frames),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count), std::ref(done));
    
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
    EXPECT_EQ(3, image_count);
    EXPECT_TRUE(test_allpels(m_sum, 3.0f, true));
    EXPECT_TRUE(test_allpels(m_sqsum, 5.0f, true));
    
    
    cv::Mat m_var_image;
    SequenceAccumulator::computeVariance(m_sum, m_sqsum, image_count, m_var_image);
    EXPECT_TRUE(test_allpels(m_var_image, 0.666667f, true));
}


TEST (ut_ss_voxel, basic){
    
    // Create N X M 1 dimentional roiWindows sized 1 x 64. Containing sin s
    
    // Create sin signals
    auto sinvec8 = [](float step, uint32_t size){
        std::vector<uint8_t> base(size);
        
        for (auto i : irange(0u, size)) {
            float v = (sin(i * 3.14159 *  step) + 1.0)*127.0f;
            base[i] = static_cast<uint8_t>(v);
        }
        
        return base;
    };
    
    double endtime;
    std::clock_t start;
    
    std::vector<std::vector<roiWindow<P8U>>> voxels;
    
    int rows = 32;
    int cols = 32;
    cv::Point2f ctr (cols/2.0f, rows/2.0f);
    
    start = std::clock();
    for (auto row = 0; row < rows; row++){
        std::vector<roiWindow<P8U>> rrs;
        for (auto col = 0; col < cols; col++){
            float r = (row+col)/2.0;
            r = std::max(1.0f,r);
            std::vector<uint8_t> tmp = sinvec8(1.0/r, 64);
            rrs.emplace_back(tmp);
        }
        voxels.push_back(rrs);
    }
    
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << " Generating Synthetic Data " << endtime  << " Seconds " << std::endl;
    std::vector<std::vector<float>> results;
    
#ifdef INTERACTIVE
    std::string filename = svl::toString(std::clock()) + ".csv";
    std::string imagename = svl::toString(std::clock()) + ".png";
    auto file_path = dgenv_ptr->output_path() / filename;
    auto image_path = dgenv_ptr->output_path() /  imagename;
#endif
    
    start = std::clock();
    auto cm = generateVoxelSelfSimilarities(voxels, results);
#ifdef INTERACTIVE
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    cv::imwrite(image_path, cm);
    std::cout << " Running Synthetic Data " << endtime  << " Seconds " << std::endl;
    output_array(results, file_path);
    
    
    /// Show in a window
    namedWindow( " ss voxel ", WINDOW_KEEPRATIO  | WINDOW_OPENGL);
    imshow( " ss voxel ", cm);
    cv::waitKey(-1);
#endif
    
}


TEST(SimpleGUITest, basic)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Build atlas
    unsigned char* tex_pixels = nullptr;
    int tex_w, tex_h;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);
    
    for (int n = 0; n < 5000; n++) {
        io.DisplaySize = ImVec2(1920, 1080);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
        
        static float f = 0.0f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        
        ImGui::Render();
    }
    
    ImGui::DestroyContext();
}

TEST(tracks, basic){
    
    // Generate test data
    namedTrack_t ntrack;
    timedVecOfVals_t& data = ntrack.second;
    data.resize(oneD_example.size());
    for (int tt = 0; tt < oneD_example.size(); tt++){
        data[tt].second = oneD_example[tt];
        data[tt].first.first = tt;
        data[tt].first.second = time_spec_t(tt / 1000.0);
    }
    
    std::vector<float> X, Y;
    domainFromPairedTracks_D (ntrack, X, Y);
    
    EXPECT_EQ(X.size(), oneD_example.size());
    EXPECT_EQ(Y.size(), oneD_example.size());
    for(auto ii = 0; ii < oneD_example.size(); ii++){
        EXPECT_TRUE(svl::RealEq(Y[ii],(float)oneD_example[ii]));
    }
}


TEST(units,basic)
{
    units_ut::run();
    eigen_ut::run();
    cardio_ut::run(dgenv_ptr);
}

TEST(timing8, corr)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(1920, 1080);
    double endtime;
    std::clock_t start;
    int l;
    start = std::clock();
    int num_loops = 10;
    // Time setting and resetting
    for (l = 0; l < num_loops; ++l)
    {
        basicCorrRowFunc<uint8_t> corrfunc(img1.get(), img2.get(), 1920, 1920, 1920, 1080);
        corrfunc.areaFunc();
        CorrelationParts cp;
        corrfunc.epilog(cp);
    }
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    double scale = 1000.0 / (num_loops);
    std::cout << " Correlation: 1920 * 1080 * 8 bit " << endtime * scale << " millieseconds per " << std::endl;
}

TEST(timing8, corr_ocv)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(1920, 1080);
    cv::Mat mat1(1080, 1920, CV_8UC(1), img1.get(), 1920);
    cv::Mat mat2(1080, 1920, CV_8UC(1), img2.get(), 1920);
    //    auto binfo = cv::getBuildInformation();
    //    auto numth = cv::getNumThreads();
    //    std::cout << binfo << std::endl << numth << std::endl;
    
    double endtime;
    std::clock_t start;
    int l;
    
    int num_loops = 10;
    Mat space(cv::Size(1,1), CV_32F);
    // Time setting and resetting
    start = std::clock();
    for (l = 0; l < num_loops; ++l)
    {
        cv::matchTemplate (mat1, mat2, space, TM_CCOEFF_NORMED);
    }
    
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    double scale = 1000.0 / (num_loops);
    std::cout << " OCV_Correlation: 1920 * 1080 * 8 bit " << endtime * scale << " millieseconds per " << std::endl;
}



TEST(ut_serialization, double_cv_mat){
    uint32_t cols = 210;
    uint32_t rows = 210;
    cv::Mat data (rows, cols, CV_64F);
    for (auto j = 0; j < rows; j++)
        for (auto i = 0; i < cols; i++){
            data.at<double>(j,i) = 1.0 / (i + j - 1);
        }
    
    auto tempFilePath = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
    {
        std::ofstream file(tempFilePath.c_str(), std::ios::binary);
        cereal::BinaryOutputArchive ar(file);
        ar(data);
    }
    // Load the data from the disk again:
    {
        cv::Mat loaded_data;
        {
            std::ifstream file(tempFilePath.c_str(), std::ios::binary);
            cereal::BinaryInputArchive ar(file);
            ar(loaded_data);
            
            EXPECT_EQ(rows, data.rows);
            EXPECT_EQ(cols, data.cols);
            
            EXPECT_EQ(data.rows, loaded_data.rows);
            EXPECT_EQ(data.cols, loaded_data.cols);
            for (auto j = 0; j < rows; j++)
                for (auto i = 0; i < cols; i++){
                    EXPECT_EQ(data.at<double>(j,i), loaded_data.at<double>(j,i));
                }
        }
    }
}

TEST(ut_localvar, basic)
{
    auto res = dgenv_ptr->asset_path("out0.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat out0 = cv::imread(res.first.c_str(),cv::ImreadModes::IMREAD_GRAYSCALE);
    std::cout << out0.channels() << std::endl;
    EXPECT_EQ(out0.channels() , 1);
    EXPECT_EQ(out0.cols , 512);
    EXPECT_EQ(out0.rows , 128);
    
    // create local variance filter size runner
    svl::localVAR tv (cv::Size(7,7));
    cv::Mat var0;
    cv::Mat std0U8;
    tv.process(out0, var0);
    EXPECT_EQ(tv.min_variance() , 0);
    EXPECT_EQ(tv.max_variance() , 11712);
}


TEST(ut_labelBlob, basic)
{
    using blob = svl::labelBlob::blob;
    
    static bool s_results_ready = false;
    static bool s_graphics_ready = false;
    static int64_t cid = 0;
    
    auto res = dgenv_ptr->asset_path("out0.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat out0 = cv::imread(res.first.c_str(), cv::ImreadModes::IMREAD_GRAYSCALE);
    EXPECT_EQ(out0.channels() , 1);
    EXPECT_EQ(out0.cols , 512);
    EXPECT_EQ(out0.rows , 128);
    
    // Histogram -> threshold at 5 percent from the right ( 95 from the left :) )
    int threshold = leftTailPost (out0, 95.0 / 100.0);
    EXPECT_EQ(threshold, 40);
    
    cv::Mat threshold_input, threshold_output;
    
    /// Detect regions using Threshold
    out0.convertTo(threshold_input, CV_8U);
    cv::threshold(threshold_input
                  , threshold_output, threshold, 255, THRESH_BINARY );
    
    labelBlob::ref lbr = labelBlob::create(out0, threshold_output, 6, 666);
    EXPECT_EQ(lbr == nullptr , false);
    std::function<labelBlob::results_ready_cb> res_ready_lambda = [](int64_t& cbi){ s_results_ready = ! s_results_ready; cid = cbi;};
    std::function<labelBlob::graphics_ready_cb> graphics_ready_lambda = [](){ s_graphics_ready = ! s_graphics_ready;};
    boost::signals2::connection results_ready_ = lbr->registerCallback(res_ready_lambda);
    boost::signals2::connection graphics_ready_ = lbr->registerCallback(graphics_ready_lambda);
    EXPECT_EQ(false, s_results_ready);
    EXPECT_EQ(true, cid == 0);
    EXPECT_EQ(true, lbr->client_id() == 666);
    lbr->run_async();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(true, s_results_ready);
    EXPECT_EQ(true, lbr->client_id() == 666);
    EXPECT_EQ(true, cid == 666);
    EXPECT_EQ(true, lbr->hasResults());
    const std::vector<blob> blobs = lbr->results();
    EXPECT_EQ(44, blobs.size());
    
#ifdef INTERACTIVE
    lbr->drawOutput();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(true, s_graphics_ready);
    /// Show in a window
    namedWindow( "LabelBlob ", WINDOW_AUTOSIZE | WINDOW_OPENGL);
    imshow( "LabelBlob", lbr->graphicOutput());
    cv::waitKey();
#endif
    
}

TEST(ut_stl_utils, accOverTuple)
{
    float val = tuple_accumulate(std::make_tuple(5, 3.2, 7U, 6.4f), 0L, functor());
    auto diff = std::fabs(val - 21.6);
    EXPECT_TRUE(diff < 0.000001);
    
    typedef std::tuple<int64_t,int64_t, uint32_t> partial_t;
    std::vector<partial_t> boo;
    for (int ii = 0; ii < 9; ii++)
        boo.emplace_back(12345679, -12345679, 1);
    
    
    auto res = std::accumulate(boo.begin(), boo.end(), std::make_tuple(int64_t(0),int64_t(0), uint32_t(0)), tuple_sum<int64_t,uint32_t>());
    bool check = res == make_tuple(111111111, -111111111, 9); //(int64_t(111111111),int64_t(-111111111), uint32_t(9));
    EXPECT_TRUE(check);
    
    
}

#if 0
TEST(UT_smfilter, basic)
{
    vector<int> ranks;
    vector<double> norms;
    norm_scale(oneD_example,norms);
    //    stl_utils::Out(norms);
    
    vector<double> output;
    savgol(norms, output);
    //  stl_utils::Out(norms);
    //  stl_utils::Out(output);
    
    auto median_value = contraction_analyzer::Median_levelsets(norms,ranks);
    
    std::cout << median_value << std::endl;
    for (auto ii = 0; ii < norms.size(); ii++)
    {
        //        std::cout << "[" << ii << "] : " << norms[ii] << "     "  << std::abs(norms[ii] - median_value) << "     "  << ranks[ii] << "     " << norms[ranks[ii]] << std::endl;
        //        std::cout << "[" << ii << "] : " << norms[ii] << "     " << output[ii] << std::endl;
    }
}
#endif

TEST(basicU8, histo)
{
    
    const char * frame[] =
    {
        "00000000000000000000",
        "00001020300000000000",
        "00000000000000000000",
        "00000000012300000000",
        "00000000000000000000",
        0};
    
    roiWindow<P8U> pels(20, 5);
    DrawShape(pels, frame);
    
    histoStats hh;
    hh.from_image<P8U>(pels);
    EXPECT_EQ(hh.max(0), 3);
    EXPECT_EQ(hh.min(0), 0);
    EXPECT_EQ(hh.sum(), 12);
    EXPECT_EQ(hh.sumSquared(), 28); // 2 * 9 + 2 * 4 + 2 * 1
}



TEST (UT_make_function, make_function)
{
    make_function_test::run();
}

#endif


TEST(ut_similarity, short_term)
{

    
    self_similarity_producer<P8U> sm(3,0);
    
    EXPECT_EQ(sm.depth() , D_8U);
    EXPECT_EQ(sm.matrixSz() , 3);
    EXPECT_EQ(sm.cacheSz() , 0);
    EXPECT_EQ(sm.aborted() , false);
    
    vector<roiWindow<P8U>> fill_images(3);
    
    
    
    //     cv::Mat gaussianTemplate(const std::pair<uint32_t,uint32_t>& dims, const vec2& sigma = vec2(1.0, 1.0), const vec2& center = vec2(0.5,0.5));
    std::pair<uint32_t,uint32_t> dims (32,32);
    
    for (uint32_t i = 0; i < fill_images.size(); i++) {
        float sigma = 0.5 + i * 0.5;
        cv::Mat fgm = getGaussianKernel(dims.first, dims.second, sigma, sigma);
        cv::Mat gm(fgm.rows, fgm.cols, CV_8U);
        fgm.convertTo(gm, CV_8U);
        cpCvMatToRoiWindow8U(gm,fill_images[i]);
    }
    roiWindow<P8U> tmp (dims.first, dims.second);
    tmp.randomFill(1066);
    
    /*
     Expected Update Output
     
     Fill     9.86492e-06            gaussian sigma 0.5
     6.21764e-06            gaussian sigma 1.0
     1.1948e-05             gaussian sigma 1.5
     Update   0.367755           update with random filled
     0.367793
     0.994818
     Update   0.367801           update with sigma 0.5
     0.994391
     0.367548
     Update   0.994314           update with sigma 1.0
     0.367543
     0.367757
     Update   9.86492e-06        update with sigma 1.5
     6.21764e-06
     1.1948e-05
     
     */
    deque<double> ent;
    bool fRet = sm.fill(fill_images);
    EXPECT_EQ(fRet, true);
    bool eRet = sm.entropies(ent);
    EXPECT_EQ(eRet, true);
    EXPECT_EQ(ent.size() , fill_images.size());
    EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 2);
    
    // Now feed the set of one random followed by 3 gaussins few times
    {
        bool u1 = sm.update(tmp);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(svl::equal(ent[0], ent[1] , 1.e-3), true);
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 2);
        
    }
    
    {
        bool u1 = sm.update(fill_images[0]);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(svl::equal(ent[0], ent[2] , 1.e-3), true);
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 1);
    }
    
    {
        bool u1 = sm.update(fill_images[1]);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(svl::equal(ent[1], ent[2] , 1.e-3), true);
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 0);
    }
    
    {
        bool u1 = sm.update(fill_images[2]);
        EXPECT_EQ(u1, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        EXPECT_EQ(ent.size() , fill_images.size());
        EXPECT_EQ(std::distance(ent.begin(), std::max_element(ent.begin(), ent.end())), 2);
    }
    
    // Test RefSimilarator
    self_similarity_producerRef simi (new self_similarity_producer<P8U> (7, 0));
    EXPECT_EQ (simi.use_count() , 1);
    self_similarity_producerRef simi2 (simi);
    EXPECT_EQ (simi.use_count() , 2);
    
}



TEST(ut_similarity, long_term)
{
    vector<roiWindow<P8U>> images(4);
    
    self_similarity_producer<P8U> sm((uint32_t) images.size(),0);
    
    EXPECT_EQ(sm.depth() , D_8U);
    EXPECT_EQ(sm.matrixSz() , images.size());
    EXPECT_EQ(sm.cacheSz() , 0);
    EXPECT_EQ(sm.aborted() , false);
    
    roiWindow<P8U> tmp (640, 480);
    tmp.randomFill(1066);
    
    for (uint32_t i = 0; i < images.size(); i++) {
        images[i] = tmp;
    }
    
    deque<double> ent;
    
    bool fRet = sm.fill(images);
    EXPECT_EQ(fRet, true);
    
    bool eRet = sm.entropies(ent);
    EXPECT_EQ(eRet, true);
    
    EXPECT_EQ(ent.size() , images.size());
    
    for (uint32_t i = 0; i < ent.size(); i++)
        EXPECT_EQ(svl::equal(ent[i], 0.0, 1.e-9), true);
    
    
    for (uint32_t i = 0; i < images.size(); i++) {
        roiWindow<P8U> tmp(640, 480);
        tmp.randomFill(i);
        images[i] = tmp;
    }
    
    
    
    fRet = sm.fill(images);
    EXPECT_EQ(fRet, true);
    
    eRet = sm.entropies(ent);
    EXPECT_EQ(eRet, true);
    
    EXPECT_EQ(ent.size() , images.size());
    EXPECT_EQ(fRet, true);
    
    
    deque<deque<double> > matrix;
    sm.selfSimilarityMatrix(matrix);
    //  rfDumpMatrix (matrix);
    
    // Test RefSimilarator
    self_similarity_producerRef simi (new self_similarity_producer<P8U> (7, 0));
    EXPECT_EQ (simi.use_count() , 1);
    self_similarity_producerRef simi2 (simi);
    EXPECT_EQ (simi.use_count() , 2);
    
    // Test Base Filer
    // vector<double> signal (32);
    // simi->filter (signal);
}



TEST (UT_cm_timer, run)
{
    cm_time c0;
    EXPECT_TRUE(c0.isValid());
    EXPECT_TRUE(c0.isZero());
    EXPECT_FALSE(c0.isNegativeInfinity());
    EXPECT_FALSE(c0.isPositiveInfinity());
    EXPECT_FALSE(c0.isIndefinite());
    EXPECT_TRUE(c0.isNumeric());
    
    cm_time c1 (0.033);
    EXPECT_EQ(c1.getScale(), 60000);
    EXPECT_EQ(c1.getValue(), 60000 * 0.033);
    c1.show();
    std::cout << c1 << std::endl;
    
    EXPECT_TRUE(c1.isValid());
    EXPECT_FALSE(c1.isZero());
    EXPECT_FALSE(c1.isNegativeInfinity());
    EXPECT_FALSE(c1.isPositiveInfinity());
    EXPECT_FALSE(c1.isIndefinite());
    EXPECT_TRUE(c1.isNumeric());
    
}





/***
 **  Use --gtest-filter=*NULL*.*shared" to select https://code.google.com/p/googletest/wiki/V1_6_AdvancedGuide#Running_Test_Programs%3a_Advanced_Options
 */


#define GTESTLOG "GLog"
#define GTESTLOG_INFO(...) spdlog::get("GLog")->info(__VA_ARGS__)
#define GTESTLOG_TRACE(...) spdlog::get("GLog")->trace(__VA_ARGS__)
#define GTESTLOG_ERROR(...) spdlog::get("GLog")->error(__VA_ARGS__)
#define GTESTLOG_WARNING(...) spdlog::get("GLog")->warn(__VA_ARGS__)
#define GTESTLOG_NOTICE(...) spdlog::get("GLog")->notice(__VA_ARGS__)
#define GTESTLOG_SEPARATOR() GTESTLOG_INFO("-----------------------------")


int main(int argc, char ** argv)
{
    
    static std::string id("VGtest");
    static std::string specifier ("--assets");
    fs::path pp(argv[0]);
    setup_loggers(pp.parent_path().string(), id);
    testing::InitGoogleTest(&argc, argv);

    
    bool check = argc >= 3 || specifier.compare(std::string(argv[1])) == 0;
    if(!check) std::cout << " Failed to supply asset path " << std::endl;
    else{
        std::shared_ptr<test_utils::genv>  shared_env (new test_utils::genv(argv[0]));
        shared_env->setUpFromArgs(argc, argv);
        
        dgenv_ptr = shared_env;
        auto ret = RUN_ALL_TESTS();
        return ret;
    }
    return -1;

    
}


/***
 **  Implementation of Support files
 */


bool setup_loggers (const std::string& log_path,  std::string id_name){
    
    try{
        // get a temporary file name
        std::string logname =  logging::reserve_unique_file_name(log_path,
                                                                 logging::create_timestamped_template(id_name));
        
        // Setup APP LOG
        auto daily_file_sink = std::make_shared<logging::sinks::daily_file_sink_mt>(logname, 23, 59);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern("G[%H:%M:%S:%e:%f %z] [%n] [%^---%L---%$] [thread %t] %v");
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(daily_file_sink);
        sinks.push_back(console_sink);
        
        auto combined_logger = std::make_shared<spdlog::logger>("GLog", sinks.begin(),sinks.end());
        combined_logger->info("Daily Log File: " + logname);
        //register it if you need to access it globally
        spdlog::register_logger(combined_logger);
        
        {
            auto combined_logger = std::make_shared<spdlog::logger>("VLog", sinks.begin(),sinks.end());
            combined_logger->info("Daily Log File: " + logname);
            //register it if you need to access it globally
            spdlog::register_logger(combined_logger);
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
    return true;
}




void output_array (const std::vector<std::vector<float>>& data, const std::string& output_file){
    std::string delim (",");
    fs::path opath (output_file);
    auto papa = opath.parent_path ();
    if (fs::exists(papa))
    {
        std::shared_ptr<std::ofstream> myfile = stl_utils::make_shared_ofstream(output_file);
        for (const vector<float>& row : data)
        {
            auto cnt = 0;
            auto cols = row.size() - 1;
            for (const float & col : row)
            {
                *myfile << col;
                if (cnt++ < cols)
                    *myfile << delim;
            }
            *myfile << std::endl;
        }
    }
}

SurfaceRef get_surface(const std::string & path){
    static std::map<std::string, SurfaceWeakRef> cache;
    static std::mutex m;
    
    std::lock_guard<std::mutex> hold(m);
    SurfaceRef sp = cache[path].lock();
    if(!sp)
    {
        auto ir = loadImage(path);
        cache[path] = sp = Surface8u::create(ir);
    }
    return sp;
}


void norm_scale (const std::vector<double>& src, std::vector<double>& dst)
{
    vector<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
    vector<double>::const_iterator top = std::max_element (src.begin (), src.end() );
    
    if (svl::equal(*top, *bot)) return;
    double scaleBy = *top - *bot;
    dst.resize (src.size ());
    for (int ii = 0; ii < src.size (); ii++)
        dst[ii] = (src[ii] - *bot) / scaleBy;
}



cv::Mat generateVoxelSelfSimilarities (std::vector<std::vector<roiWindow<P8U>>>& voxels,
                                       std::vector<std::vector<float>>& ss){
    
    int height = static_cast<int>(voxels.size());
    int width = static_cast<int>(voxels[0].size());
    
    
    // Create a single vector of all roi windows
    std::vector<roiWindow<P8U>> all;
    // semilarity producer
    auto sp = std::shared_ptr<sm_producer> ( new sm_producer () );
    for(std::vector<roiWindow<P8U>>& row: voxels){
        for(roiWindow<P8U>& voxel : row){
            all.emplace_back(voxel.frameBuf(), voxel.bound());
        }
    }
    
    sp->load_images (all);
    std::packaged_task<bool()> task([sp](){ return sp->operator()(0);}); // wrap the function
    std::future<bool>  future_ss = task.get_future();  // get a future
    std::thread(std::move(task)).join(); // Finish on a thread
    if (future_ss.get())
    {
        cv::Mat m_temporal_ss (height,width, CV_32FC1);
        m_temporal_ss.setTo(0.0f);
        
        const deque<double>& entropies = sp->shannonProjection ();
        vector<float> m_entropies;
        ss.resize(0);
        m_entropies.insert(m_entropies.end(), entropies.begin(), entropies.end());
        vector<float>::const_iterator start = m_entropies.begin();
        for (auto row =0; row < height; row++){
            vector<float> rowv;
            auto end = start + width; // point to one after
            rowv.insert(rowv.end(), start, end);
            ss.push_back(rowv);
            start = end;
        }
        
        auto getMat = [] (std::vector< std::vector<float> > &inVec){
            int rows = static_cast<int>(inVec.size());
            int cols = static_cast<int>(inVec[0].size());
            
            cv::Mat_<float> resmat(rows, cols);
            for (int i = 0; i < rows; i++)
            {
                resmat.row(i) = cv::Mat(inVec[i]).t();
            }
            return resmat;
        };
        
        m_temporal_ss = getMat(ss);
        return m_temporal_ss;
    }
    return cv::Mat ();
    
}




#pragma GCC diagnostic pop
