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

#include <iostream>
#include <fstream>
#include <chrono>
#include "gtest/gtest.h"
#include <memory>
#include <thread>


#include <boost/foreach.hpp>
#include "boost/algorithm/string.hpp"
#include <boost/range/irange.hpp>
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"


#include "opencv2/highgui.hpp"
#include "vision/roiWindow.h"
#include "vision/self_similarity.h"
#include "seq_frame_container.hpp"
#include "cinder/ImageIo.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "ut_sm.hpp"
#include "AVReader.hpp"
#include "cm_time.hpp"
#include "core/gtest_env_utils.hpp"
#include "vision/colorhistogram.hpp"
#include "cinder_opencv.h"
#include "ut_util.hpp"
#include "algo_cardiac.hpp"
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
#include "cvmat_cereal_serialization.h"
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include "vision/opencv_utils.hpp"
#include "permutation_entropy.h"

#include "cvplot/cvplot.h"
#include "time_series/persistence1d.hpp"
#include "async_tracks.h"
#include "CinderImGui.h"
#include "logger.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/gauss.hpp"
#include "vision/dense_motion.hpp"
#include "vision/gradient.h"
#include "algo_Lif.hpp"

// @FIXME Logger has to come before these
#include "ut_units.hpp"
#include "ut_cardio.hpp"

#define cimg_plugin1 "plugins/cvMat.h"
#define cimg_display 0
#include "CImg.h"
using namespace cimg_library;


#define INTERACTIVE

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

void cvDrawPlot (std::vector<float>& tmp){
    
    std::string name = svl::toString(std::clock());
    cvplot::setWindowTitle(name, svl::toString(tmp.size()));
    cvplot::moveWindow(name, 0, 256);
    cvplot::resizeWindow(name, 512, 256);
    cvplot::figure(name).series(name).addValue(tmp).type(cvplot::Line).color(cvplot::Red);
    cvplot::figure(name).show();
}


std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr)
{
    return std::shared_ptr<std::ofstream>(ifstream_ptr, ofStreamDeleter());
}

std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename)
{
    return make_shared_ofstream(new std::ofstream(filename, std::ofstream::out));
}

void output_array (const std::vector<std::vector<float>>& data, const std::string& output_file){
    std::string delim (",");
    fs::path opath (output_file);
    auto papa = opath.parent_path ();
    if (fs::exists(papa))
    {
        std::shared_ptr<std::ofstream> myfile = make_shared_ofstream(output_file);
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


std::shared_ptr<test_utils::genv> dgenv_ptr;

typedef std::weak_ptr<Surface8u>	Surface8uWeakRef;
typedef std::weak_ptr<Surface8u>	SurfaceWeakRef;
typedef std::weak_ptr<Surface16u>	Surface16uWeakRef;
typedef std::weak_ptr<Surface32f>	Surface32fWeakRef;

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


std::vector<Point2f> ellipse_test = {
    {666.895422,372.895287},
    {683.128254,374.338401},
    {698.197455,379.324060},
    {735.650898,381.410000},
    {726.754550,377.262089},
    {778.094505,381.039470},
    {778.129881,381.776216},
    {839.415543,384.804510}};

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
    dmf ff(frame_size, fixed_half_size, moving_half_size);
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
    
    std::cout << std::endl;
    for(std::vector<int>& row : space){
        for (const int& score : row){
           std::cout << setw(4) << score << '\t';
        }
        std::cout << std::endl;
    }
    
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
    
    std::vector<Point2f> peaks;
    PeakDetect(im, peaks, 0);
    EXPECT_EQ(peaks.size(),gold.size());
    for (auto cc = 0; cc < peaks.size(); cc++)
        EXPECT_TRUE(same_point(peaks[cc], gold[cc],0.05));
    
    
    RotatedRect box = fitEllipse(peaks);
    EXPECT_TRUE(same_point(box.center, center_gold, 0.05));
    EXPECT_TRUE(svl::equal(box.angle, 60.85f, 1.0f));
    
}


std::vector<double> acid = {39.1747, 39.2197, 39.126, 39.0549, 39.0818, 39.0655, 39.0342,
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

TEST(ut_median, basic){
    std::vector<double> dst;
    bool ok = rolling_median_3(acid.begin(), acid.end(), dst);
    EXPECT_TRUE(ok);
    
}

void done_callback (void)
{
    std::cout << "Done"  << std::endl;
}

TEST (ut_opencvutils, anistropic_diffusion){
   
    double endtime;
    std::clock_t start;
    
   
    auto res = dgenv_ptr->asset_path("image230.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat image = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    EXPECT_EQ(image.channels() , 1);
    EXPECT_EQ(image.cols , 512);
    EXPECT_EQ(image.rows , 128);
    cv::Mat image_anisotropic;
    start = std::clock();
    Anisotrpic_Diffusion(image, image_anisotropic, 0.25, 10, 45, 45);
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << " Anisotrpic Diffusion " << endtime  << " Seconds " << std::endl;
    
#ifdef INTERACTIVE
    std::string file_path = "/Users/arman/tmp/simple_aniso.png";
    cv::imwrite(file_path, image_anisotropic);
    cv::imshow("Anistorpic", image_anisotropic);
    cv::waitKey();
#endif
}

cv::Mat show_cv_angle (const cv::Mat& src, const std::string& name){
    cv::Mat mag, ang;
    svl::sobel_opencv(src, mag, ang, 7);
    
#ifdef INTERACTIVE
    namedWindow(name.c_str(), CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    cv::imshow(name.c_str(), ang);
    cv::waitKey();
#endif
    return ang;
    
}


void show_gradient (const cv::Mat& src, const std::string& name){
    cv::Mat disp3c(src.size(), CV_8UC3, Scalar(255,255,255));
    
    auto cpToRoiWindow = [](const cv::Mat& m, roiWindow<P8U>& r){
        auto rowPointer = [] (void* data, size_t step, int32_t row ) { return reinterpret_cast<void*>( reinterpret_cast<uint8_t*>(data) + row * step ); };
        unsigned cols = m.cols;
        unsigned rows = m.rows;
        roiWindow<P8U> rw(cols,rows);
        for (auto row = 0; row < rows; row++) {
            std::memcpy(rw.rowPointer(row), rowPointer(m.data, m.step, row), cols);
        }
        r = rw;
    };
    
    roiWindow<P8U> r8(src.cols, src.rows);
    cpToRoiWindow(src, r8);
    roiWindow<P8U> mag = r8.clone();
    roiWindow<P8U> ang = r8.clone();
    roiWindow<P8U> peaks = r8.clone();
    std::vector<svl::feature> features;
    Gradient(r8,mag, ang);
    auto pks = SpatialEdge(mag, ang, features, 2);
    
    cvMatOfroiWindow(peaks, mim, CV_8UC1);
    cvMatOfroiWindow(ang, aim, CV_8UC1);
    
    
    auto rt = rightTailPost(mim, 0.005f);
    std::cout << pks << "    " << rt << std::endl;
    
    auto vec2point = [] (const fVector_2d& v, cv::Point& p) {p.x = v.x(); p.y = v.y();};
    
    for (auto fea : features){
        cv::Point pt;
        vec2point(fea.position(),pt);
        svl::drawCross(disp3c, pt,cv::Scalar(CVCOLOR_RED), 3, 1);
    }
    
#ifdef INTERACTIVE
    namedWindow(name.c_str(), CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    cv::imshow(name.c_str(), disp3c);
    cv::waitKey();
#endif
    
}

TEST (ut_opencvutils, difference){
    auto res = dgenv_ptr->asset_path("image230.png");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    cv::Mat fixed = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    EXPECT_EQ(fixed.channels() , 1);
    EXPECT_EQ(fixed.cols , 512);
    EXPECT_EQ(fixed.rows , 128);
    
    auto res2 = dgenv_ptr->asset_path("image262.png");
    EXPECT_TRUE(res2.second);
    EXPECT_TRUE(boost::filesystem::exists(res2.first));
    cv::Mat moving = cv::imread(res2.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
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
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(SequenceAccumulator(),std::ref(frames),
                             std::ref(m_sum), std::ref(m_sqsum), std::ref(image_count));
    
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
 
    // Create random signals
//    auto randvec8 = []( uint32_t size){
//        std::vector<uint8_t> base(size);
//
//        for (auto i : irange(0u, size)) {
//            // base[i] = ((i % 256) / 256.0f - 0.5f) * 0.8;
//            base[i] = rand() % 255;
//        }
//
//        return base;
//    };
    
    double endtime;
    std::clock_t start;

    std::vector<std::vector<roiWindow<P8U>>> voxels;
    
    int rows = 32;
    int cols = 32;
    cv::Point2f ctr (cols/2.0f, rows/2.0f);
    
//    auto d2ctr = [](int r, int c, cv::Point2f& center){
//        return sqrt(svl::Sqr(c-center.x)+svl::Sqr(r-center.y));
//    };
    start = std::clock();
    for (auto row = 0; row < rows; row++){
        std::vector<roiWindow<P8U>> rrs;
        for (auto col = 0; col < cols; col++){
            float r = (row+col)/2.0;
            r = std::max(1.0f,r);

            std::vector<uint8_t> tmp = sinvec8(1.0/r, 64);

            rrs.emplace_back(tmp);
            
        
     //       cvDrawPlot(signal, title);
        }
        voxels.push_back(rrs);
    }

    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << " Generating Synthetic Data " << endtime  << " Seconds " << std::endl;
    

    std::vector<std::vector<float>> results;
    
#ifdef INTERACTIVE
    std::string filename = svl::toString(std::clock()) + ".csv";
    std::string imagename = svl::toString(std::clock()) + ".png";
    std::string file_path = "/Users/arman/tmp/" + filename;
    std::string image_path = "/Users/arman/tmp/" + imagename;
#endif
    
    start = std::clock();
    auto cm = generateVoxelSelfSimilarities(voxels, results);
#ifdef INTERACTIVE
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    cv::imwrite(image_path, cm);
    std::cout << " Running Synthetic Data " << endtime  << " Seconds " << std::endl;
    output_array(results, file_path);
 
    
    /// Show in a window
    namedWindow( " ss voxel ", CV_WINDOW_KEEPRATIO  | WINDOW_OPENGL);
    imshow( " ss voxel ", cm);
    cv::waitKey(-1);
#endif
    
}


TEST (UT_AVReaderBasic, run)
{
    static bool check_done_val = false;
    static int frame_count = 0;
    const auto check_done = [] (){
        check_done_val = true;
    };
    
    // Note current implementation uses default callbacks for building timestamps
    // and images. User supplied callbacks for replace the default ones.
    const auto progress_report = [] (CMTime time) {
        cm_time t(time); std::cout << frame_count << "  " << t << std::endl; frame_count++;
    };
    
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.m4v");
    
    auto res = dgenv_ptr->asset_path(qmov_name);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    {
        check_done_val = false;
        avcc::avReader rr (test_filepath.string(), false);
        rr.setUserDoneCallBack(check_done);
        rr.setUserProgressCallBack(progress_report);
        rr.run ();

        EXPECT_TRUE(rr.info().count == 30);
        EXPECT_FALSE(rr.isEmpty());
        tiny_media_info mif (rr.info());
        mif.printout();
        
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli> (2000));
        EXPECT_TRUE(check_done_val);
        EXPECT_TRUE(rr.isValid());
        EXPECT_TRUE(rr.size().first == 57);
        EXPECT_TRUE(rr.size().second == 57);
        EXPECT_FALSE(rr.isEmpty());
        
        
    }
    
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
    data.resize(acid.size());
    for (int tt = 0; tt < acid.size(); tt++){
        data[tt].second = acid[tt];
        data[tt].first.first = tt;
        data[tt].first.second = time_spec_t(tt / 1000.0);
    }
    
    std::vector<float> X, Y;
    domainFromPairedTracks_D (ntrack, X, Y);
    
    EXPECT_EQ(X.size(), acid.size());
    EXPECT_EQ(Y.size(), acid.size());
    for(auto ii = 0; ii < acid.size(); ii++){
        EXPECT_TRUE(svl::RealEq(Y[ii],(float)acid[ii]));
    }
}



void savgol (const vector<double>& signal, vector<double>& dst)
{
    // for scalar data:
    int order = 4;
    int winlen = 17 ;
    SGF::real sample_time = 0; // this is simply a float or double, you can change it in the header sg_filter.h if yo u want
    SGF::ScalarSavitzkyGolayFilter filter(order, winlen, sample_time);
    dst.resize(signal.size());
    SGF::real output;
    for (auto ii = 0; ii < signal.size(); ii++)
    {
        filter.AddData(signal[ii]);
        if (! filter.IsInitialized()) continue;
        dst[ii] = 0;
        int ret_code;
        ret_code = filter.GetOutput(0, output);
        dst[ii] = output;
    }
    
}

TEST(units,basic)
{
    units_ut::run();
    eigen_ut::run();
    cardio_ut::run(dgenv_ptr);
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
TEST(cardiac_ut, interpolated_length)
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
    
    double            MicronPerPixel = 291.19 / 512.0;
    double            Length_max   = 118.555 * MicronPerPixel; // 67.42584072;
    double            Lenght_min   = 106.551 * MicronPerPixel; // 60.59880018;
   // double            shortening   = Length_max - Lenght_min;
    double            MSI_max  =  0.37240525;
    double            MSI_min   = 0.1277325;
    // double            shortening_um   = 6.827040547;
    
    data_t dst;
    flip(array,dst);
    auto check = std::minmax_element(dst[1].begin(), dst[1].end() );
    EXPECT_TRUE(svl::equal(*check.first, MSI_min, 1.e-05));
    EXPECT_TRUE(svl::equal(*check.second, MSI_max, 1.e-05));
    
    auto car = contraction_analyzer::create();
    car->load(dst[1]);
    EXPECT_TRUE(car->isPreProcessed());
    EXPECT_TRUE(car->isValid());
    EXPECT_TRUE(car->leveled().size() == dst[1].size());
    car->find_best();
    const std::pair<double,double>& minmax = car->leveled_min_max ();
    EXPECT_TRUE(svl::equal(*check.first, minmax.first, 1.e-05));
    EXPECT_TRUE(svl::equal(*check.second, minmax.second, 1.e-05));
    
    const vector<double>& signal = car->leveled();
    auto start = std::clock();
    car->compute_interpolated_geometries(Lenght_min, Length_max);
    auto endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << endtime << " interpolated length " << std::endl;
    start = std::clock();
    car->compute_force(signal.begin(),signal.end(),Lenght_min, Length_max );
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    std::cout << endtime << " FORCE " << std::endl;
    const vector<double>& lengths = car->interpolated_length();
    EXPECT_EQ(dst[2].size(), lengths.size());
    std::vector<double> diffs;
    for(auto row = 0; row < dst[2].size(); row++)
    {
        auto dd = dst[2][row] - lengths[row];
        diffs.push_back(dd);
    }
    
    auto dcheck = std::minmax_element(diffs.begin(), diffs.end() );
    EXPECT_TRUE(svl::equal(*dcheck.first, 0.0, 1.e-05));
    EXPECT_TRUE(svl::equal(*dcheck.second, 0.0, 1.e-05));
 
    {
        auto name = "force";
        cvplot::setWindowTitle(name, " Force ");
        cvplot::moveWindow(name, 0, 256);
        cvplot::resizeWindow(name, 512, 256);
        cvplot::figure(name).series("Force").addValue(car->total_reactive_force()).type(cvplot::Line).color(cvplot::Red);
        cvplot::figure(name).show();
    }
    {
        auto name = "interpolated length";
        cvplot::setWindowTitle(name, " Length ");
        cvplot::moveWindow(name,0,0);
        cvplot::resizeWindow(name, 512, 256);
        cvplot::figure(name).series("Length").addValue(car->interpolated_length()).type(cvplot::Line).color(cvplot::Blue);
        cvplot::figure(name).show();
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
    persistenace1d<double> p;
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
 //   Out(tmins);
 //   Out(tmaxs);
    
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
//    Out(lmins);
//    Out(lmaxs);
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

TEST(UT_contraction_profiler, basic)
{
    contraction_analyzer::contraction_t ctr;
    typedef vector<double>::iterator dItr_t;
    
    std::vector<double> fder, fder2;
    fder.resize (acid.size());
    fder2.resize (acid.size());
    
    // Get contraction peak ( valley ) first
    auto min_iter = std::min_element(acid.begin(),acid.end());
    ctr.contraction_peak.first = std::distance(acid.begin(),min_iter);
    
    // Computer First Difference,
    adjacent_difference(acid.begin(),acid.end(), fder.begin());
    std::rotate(fder.begin(), fder.begin()+1, fder.end());
    fder.pop_back();
    auto medianD = stl_utils::median1D<double>(7);
    fder = medianD.filter(fder);
    std::transform(fder.begin(), fder.end(), fder2.begin(), [](double f)->double { return f * f; });
    // find first element greater than 0.1
    auto pos = find_if (fder2.begin(), fder2.end(),    // range
                        std::bind2nd(greater<double>(),0.1));  // criterion
    
    ctr.contraction_start.first = std::distance(fder2.begin(),pos);
    auto max_accel = std::min_element(fder.begin()+ ctr.contraction_start.first ,fder.begin()+ctr.contraction_peak.first);
    ctr.contraction_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_start.first, max_accel);
    ctr.contraction_max_acceleration.first += ctr.contraction_start.first;
    auto max_relax = std::max_element(fder.begin()+ ctr.contraction_peak.first ,fder.end());
    ctr.relaxation_max_acceleration.first = std::distance(fder.begin()+ ctr.contraction_peak.first, max_relax);
    ctr.relaxation_max_acceleration.first += ctr.contraction_peak.first;
    
    // Initialize rpos to point to the element following the last occurance of a value greater than 0.1
    // If there is no such value, initialize rpos = to begin
    // If the last occurance is the last element, initialize this it to end
    dItr_t rpos = find_if (fder2.rbegin(), fder2.rend(),    // range
                           std::bind2nd(greater<double>(),0.1)).base();  // criterion
    ctr.relaxation_end.first = std::distance (fder2.begin(), rpos);
    
    EXPECT_EQ(ctr.contraction_start.first,16);
    EXPECT_EQ(ctr.contraction_peak.first,35);
    EXPECT_EQ(ctr.contraction_max_acceleration.first,27);
    EXPECT_EQ(ctr.relaxation_max_acceleration.first,43);
    EXPECT_EQ(ctr.relaxation_end.first,52);
  
    
    
    contraction_profile_analyzer ca;
    ca.run(acid);
        bool test = contraction_analyzer::contraction_t::equal(ca.contraction(), ctr);
       EXPECT_TRUE(test);
//    {
//        cvplot::figure("myplot").series("myline").addValue(ca.first_derivative_filtered());
//        cvplot::figure("myplot").show();
//    }
    
}
TEST(timing8, corr)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(1920, 1080);
    double endtime;
    std::clock_t start;
    int l;
    start = std::clock();
    int num_loops = 1000;
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

    int num_loops = 1000;
    Mat space(cv::Size(1,1), CV_32F);
    // Time setting and resetting
    start = std::clock();
    for (l = 0; l < num_loops; ++l)
    {
        cv::matchTemplate (mat1, mat2, space, CV_TM_CCOEFF_NORMED);
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
    cv::Mat out0 = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
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
    cv::Mat out0 = cv::imread(res.first.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
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
    
    labelBlob::ref lbr = labelBlob::create(out0, threshold_output, 666);
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
    EXPECT_EQ(59, blobs.size());
    
#ifdef INTERACTIVE
    lbr->drawOutput();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(true, s_graphics_ready);
    /// Show in a window
    namedWindow( "LabelBlob ", CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
    imshow( "LabelBlob", lbr->graphicOutput());
    cv::waitKey(60.0);
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

TEST(UT_smfilter, basic)
{
    vector<int> ranks;
    vector<double> norms;
    norm_scale(acid,norms);
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


TEST (UT_algo, AVReader)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    
    auto res = dgenv_ptr->asset_path("box-move.m4v");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    avcc::avReaderRef rref = std::make_shared< avcc::avReader> (test_filepath.string(), false);
    rref->setUserDoneCallBack(done_callback);
    
    rref->run ();
    std::shared_ptr<seqFrameContainer> sm = seqFrameContainer::create(rref);
    
    EXPECT_TRUE(rref->isValid());
    EXPECT_TRUE(sm->count() == 57);
    
}




TEST(ut_similarity, short_term)
{
   
    self_similarity_producer<P8U> sm(3,0);
    
    EXPECT_EQ(sm.depth() , D_8U);
    EXPECT_EQ(sm.matrixSz() , 3);
    EXPECT_EQ(sm.cacheSz() , 0);
    EXPECT_EQ(sm.aborted() , false);

    vector<roiWindow<P8U>> fill_images(3);

    
    auto cpToRoiWindow = [](cv::Mat& m, roiWindow<P8U>& r){
        auto rowPointer = [] (void* data, size_t step, int32_t row ) { return reinterpret_cast<void*>( reinterpret_cast<uint8_t*>(data) + row * step ); };
        unsigned cols = m.cols;
        unsigned rows = m.rows;
        roiWindow<P8U> rw(cols,rows);
        for (auto row = 0; row < rows; row++) {
            std::memcpy(rw.rowPointer(row), rowPointer(m.data, m.step, row), cols);
        }
        r = rw;
    };
    
//     cv::Mat gaussianTemplate(const std::pair<uint32_t,uint32_t>& dims, const vec2& sigma = vec2(1.0, 1.0), const vec2& center = vec2(0.5,0.5));
    std::pair<uint32_t,uint32_t> dims (32,32);
    
    for (uint32_t i = 0; i < fill_images.size(); i++) {
        float sigma = 0.5 + i * 0.5;
        cv::Mat gm = gaussianTemplate(dims,vec2(sigma,sigma));
        cpToRoiWindow(gm,fill_images[i]);
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


TEST (UT_QtimeCache, run)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.m4v");
    
    auto res = dgenv_ptr->asset_path(qmov_name);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    ci::qtime::MovieSurfaceRef m_movie;
    m_movie = qtime::MovieSurface::create( test_filepath.string() );
    EXPECT_TRUE(m_movie->isPlayable ());
    
    std::shared_ptr<seqFrameContainer> sm = seqFrameContainer::create(m_movie);
    
    Surface8uRef s8 = ci::Surface8u::create(123, 321, false);
    time_spec_t t0 = 0.0f;
    time_spec_t t1 = 0.1f;
    time_spec_t t2 = 0.2f;
    
    EXPECT_TRUE(sm->loadFrame(s8, t0)); // loaded the first time
    EXPECT_FALSE(sm->loadFrame(s8, t0)); // second time at same stamp, uses cache return true
    EXPECT_TRUE(sm->loadFrame(s8, t1)); // second time at same stamp, uses cache return true
    EXPECT_TRUE(sm->loadFrame(s8, t2)); // second time at same stamp, uses cache return true
    EXPECT_FALSE(sm->loadFrame(s8, t0)); // second time at same stamp, uses cache return true
    
}







/***
 **  Use --gtest-filter=*NULL*.*shared" to select https://code.google.com/p/googletest/wiki/V1_6_AdvancedGuide#Running_Test_Programs%3a_Advanced_Options
 */


int main(int argc, char ** argv)
{
    static std::string id("VGtest");
    std::shared_ptr<test_utils::genv>  shared_env (new test_utils::genv(argv[0]));
    shared_env->setUpFromArgs(argc, argv);
    dgenv_ptr = shared_env;
    fs::path pp(argv[0]);
    logging::setup_loggers (pp.parent_path().string(), id);
    testing::InitGoogleTest(&argc, argv);
    
    
    auto ret = RUN_ALL_TESTS();
    std::cerr << ret << std::endl;
    
}


#include "ut_lif.hpp"



#pragma GCC diagnostic pop
