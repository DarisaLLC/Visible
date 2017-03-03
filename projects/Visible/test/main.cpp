//
//  main.cpp
//  VisibleGtest
//
//  Created by Arman Garakani on 5/16/16.
//
//

#include <iostream>
#include <chrono>
#include "gtest/gtest.h"
#include <memory>
#include "boost/filesystem.hpp"
#include "opencv2/highgui.hpp"
#include "roiWindow.h"
#include "vision/self_similarity.h"
#include "cinder_buffer.hpp"
#include "qtime_frame_cache.hpp"
#include "cinder/qtime/Quicktime.h"
#include "cinder/app/Renderer.h"
#include "cinder/ImageIO.h"
#include "cinder_xchg.hpp"
#include "cinder/ip/Grayscale.h"
#include "ut_sm.hpp"
#include "AVReader.hpp"
#include "cm_time.hpp"
#include "core/gtest_image_utils.hpp"
#include "vision/colorhistogram.hpp"
#include "CinderOpenCV.h"
#include "ut_algo.hpp"

using namespace boost;

using namespace ci;
using namespace ci::ip;
namespace fs = boost::filesystem;


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


TEST (UT_qtimeCache, AVReader)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    
    auto res = dgenv_ptr->asset_path("box-move.m4v");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    avcc::avReaderRef rref = std::make_shared< avcc::avReader> (test_filepath.string());
    rref->run ();
    std::shared_ptr<qTimeFrameCache> sm = qTimeFrameCache::create(rref);
    
    EXPECT_TRUE(rref->isValid());
    EXPECT_TRUE(sm->count() == 57);
    
    
}

TEST(colorHistogram, basic)
{
    {
        std::string name = "red_bar.png";
        std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(name);
        EXPECT_TRUE(res.second);
        {
            std::pair<Surface8uRef, Channel8uRef> wp (svl::image_io_read_surface(res.first));
            roiWindow<P8UC4> rootwin = svl::NewFromSurface(wp.first.get());
            EXPECT_EQ(rootwin.width(), 420);
            EXPECT_EQ(rootwin.height(), 234);
        
            Surface8uRef rr = wp.first;
            cv::Mat rgb = cinder::toOcvRef(*wp.first.get());
            ColorSpaceHistogram sh (rgb);
            sh.run();
            
            sh.check_against(sh.spaceHistogram ());
            
           // std::cout << sh << std::endl;
            
        }
    }
    
}

#if 1

TEST (UT_SimilarityProducer, run)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("ump4.mov");
    
    auto res = dgenv_ptr->asset_path(qmov_name);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    {
        cb_similarity_producer test (test_filepath.string());
        EXPECT_EQ(0, test.run () );
        //        EXPECT_EQ(true, test.is_movie_loaded () );
        EXPECT_EQ(353, test.frameCount ());
        EXPECT_EQ(353, test.sp->shannonProjection().size () );
        EXPECT_EQ(false, svl::contains_nan(test.sp->shannonProjection().begin(), test.sp->shannonProjection().end()));
        
        std::ofstream f("/Users/arman/tmp/test.txt");
        for(auto i = test.sp->shannonProjection().begin(); i != test.sp->shannonProjection().end(); ++i) {
            f << *i << '\n';
        }
        
        EXPECT_EQ(true, test.mlies.empty());
    }
    
    
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


TEST (UT_AVReaderBasic, run)
{
    boost::filesystem::path test_filepath;
    avcc::avReader::progress_callback pcb;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("Bars.mov");
    
    auto res = dgenv_ptr->asset_path(qmov_name);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    if (res.second)
        test_filepath = res.first;
    
    {
        avcc::avReader rr (test_filepath.string());
        EXPECT_TRUE(rr.info().count == 11);
        EXPECT_FALSE(rr.isEmpty());
        tiny_media_info mif (rr.info());
        mif.printout();
        
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli> (2000));
        EXPECT_TRUE(rr.isValid());
        EXPECT_TRUE(rr.size().first == 11);
        EXPECT_TRUE(rr.size().second == 11);
        EXPECT_FALSE(rr.isEmpty());
        
        
    }
    
}




TEST(ut_similarity, run)
{
    vector<roiWindow<P8U>> images(4);
    double tiny = 1e-10;
    
    self_similarity_producer<P8U> sm((uint32_t) images.size(),0, self_similarity_producer<P8U>::eNorm,
                                     false,
                                     0,
                                     tiny);
    
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
    
    
    EXPECT_EQ (sm.longTermCache() , false);
    EXPECT_EQ (sm.longTermCache (true) , true);
    EXPECT_EQ (sm.longTermCache() , true);
    
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


TEST (UT_surface_roi_conversion, run)
{
    Surface8uRef s8 = ci::Surface8u::create(3, 7, true);
    
    EXPECT_TRUE(s8->hasAlpha());
    int32_t sums[4] = {0,0,0};
    for (int r = 0; r < s8->getHeight(); r++)
        for (int c = 0; c < s8->getWidth(); c++)
        {
            vec2 pos (c, r);
            ColorAT<uint8_t> col (r, c, r+c, 255);
            sums[0] += col[0];
            sums[1] += col[1];
            sums[2] += col[2];
            s8->setPixel(pos, col);
        }
    
    roiWindow<P8UC4> uc4 = svl::NewFromSurface(s8.get());
    EXPECT_EQ(3, uc4.width());
    EXPECT_EQ(7, uc4.height());
    
    for (int r = 0; r < s8->getHeight(); r++)
    {
        uint32_t* pel = (uint32_t*) uc4.rowPointer(r);
        for (int c = 0; c < s8->getWidth(); c++, pel++)
        {
            ColorAT<uint8_t> col (r, c, r+c, 255);
            uint8_t* pels = (uint8_t*) pel;
            EXPECT_TRUE(pels[0] == col[0]);
            EXPECT_TRUE(pels[1] == col[1]);
            EXPECT_TRUE(pels[2] == col[2]);
            EXPECT_TRUE(pels[3] == col[3]);
        }
    }
    
    Channel8uRef gray = Channel8u::create(s8->getWidth(), s8->getHeight());
    ip::grayscale(*s8, gray.get());
    
    roiWindow<P8U> u8 = svl::NewFromChannel(*gray);
    EXPECT_EQ(uc4.width(), u8.width());
    EXPECT_EQ(uc4.height(), u8.height());
    
    
}

TEST (UT_fileutils, run)
{
    boost::filesystem::path test_filepath;
    
    std::string txtfile ("onecolumn.txt");
    std::string matfile ("matrix.txt");
    
    auto res = dgenv_ptr->asset_path(txtfile);
    if (res.second)
        test_filepath = res.first;
    
    VisibleAudioSourceRef   vis ( new VisibleAudioSource (test_filepath));
    EXPECT_TRUE (vis->getNumChannels() == 1);
    EXPECT_TRUE (vis->getNumFrames () == 3296);
    
    auto res2 = dgenv_ptr->asset_path(matfile);
    if (res2.second)
        test_filepath = res2.first;
    
    vector<vector<float> > matrix;
    csv::csv2vectors(test_filepath.string(), matrix, false, false, true);
    EXPECT_TRUE(matrix.size() == 300);
    for (int rr=0; rr < matrix.size(); rr++)
        EXPECT_TRUE(matrix[rr].size() == 300);
    
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
    
    std::shared_ptr<qTimeFrameCache> sm = qTimeFrameCache::create(m_movie);
    
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

#endif



/***
 **  Use --gtest-filter=*NULL*.*shared" to select https://code.google.com/p/googletest/wiki/V1_6_AdvancedGuide#Running_Test_Programs%3a_Advanced_Options
 */


int main(int argc, char ** argv)
{
    std::shared_ptr<test_utils::genv>  shared_env (new test_utils::genv(argv[0]));
    shared_env->setUpFromArgs(argc, argv);
    dgenv_ptr = shared_env;
    testing::InitGoogleTest(&argc, argv);
    
    
    RUN_ALL_TESTS();
    
}

