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
#include "qtime_frame_cache.hpp"
#include "cinder/ImageIO.h"
#include "cinder_xchg.hpp"
#include "ut_sm.hpp"
#include "AVReader.hpp"
#include "cm_time.hpp"
#include "core/gtest_image_utils.hpp"
#include "vision/colorhistogram.hpp"
#include "CinderOpenCV.h"
#include "algoFunctions.hpp"
#include "ut_util.hpp"
#include "ut_algo.hpp"
#include "getLuminanceAlgo.hpp"
#include "core/csv.hpp"
#include "core/kmeans1d.hpp"
#include "core/stl_utils.hpp"
#include "contraction.hpp"
#include "sm_producer.h"
#include "algo_mov.hpp"
#include "sg_filter.h"

using namespace boost;

using namespace ci;
using namespace ci::ip;
using namespace spiritcsv;
using namespace kmeans1D;

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


void norm_scale (const std::vector<double>& src, std::deque<double>& dst)
{
    vector<double>::const_iterator bot = std::min_element (src.begin (), src.end() );
    vector<double>::const_iterator top = std::max_element (src.begin (), src.end() );
    
    if (svl::equal(*top, *bot)) return;
    double scaleBy = *top - *bot;
    dst.resize (src.size ());
    for (int ii = 0; ii < src.size (); ii++)
        dst[ii] = (src[ii] - *bot) / scaleBy;
}




void savgol (const deque<double>& signal, deque<double>& dst)
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


TEST(UT_smfilter, basic)
{
    vector<int> ranks;
    deque<double> norms;
    norm_scale(acid,norms);
//    stl_utils::Out(norms);
    
    deque<double> output;
    savgol(norms, output);
    stl_utils::Out(norms);
    stl_utils::Out(output);
    
    auto median_value = contraction_analyzer::Median_levelsets(norms,ranks);
    
    std::cout << median_value << std::endl;
    for (auto ii = 0; ii < norms.size(); ii++)
    {
//        std::cout << "[" << ii << "] : " << norms[ii] << "     "  << std::abs(norms[ii] - median_value) << "     "  << ranks[ii] << "     " << norms[ranks[ii]] << std::endl;
        std::cout << "[" << ii << "] : " << norms[ii] << "     " << output[ii] << std::endl;
    }
    
    
}





TEST(UT_contraction, basic)
{
    contraction_analyzer::contraction ctr;
    typedef vector<double>::iterator dItr_t;
    
    double contraction_start_threshold = 0.1;
    
    std::vector<double> fder, fder2;
    auto bItr = acid.begin();
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
                   bind2nd(greater<double>(),0.1));  // criterion

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
                        bind2nd(greater<double>(),0.1)).base();  // criterion
    ctr.relaxation_end.first = std::distance (fder2.begin(), rpos);
    
    EXPECT_EQ(ctr.contraction_start.first,16);
    EXPECT_EQ(ctr.contraction_peak.first,35);
    EXPECT_EQ(ctr.contraction_max_acceleration.first,27);
    EXPECT_EQ(ctr.relaxation_max_acceleration.first,43);
    EXPECT_EQ(ctr.relaxation_end.first,52);
    
    contraction_profile_analyzer ca;
    ca.run(acid);
    bool test = contraction_analyzer::contraction::equal(ca.pols(), ctr);
    EXPECT_TRUE(test);
    
    
}
void done_callback (void)
{
        std::cout << "Done"  << std::endl;
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
    std::shared_ptr<qTimeFrameCache> sm = qTimeFrameCache::create(rref);
    
    EXPECT_TRUE(rref->isValid());
    EXPECT_TRUE(sm->count() == 57);

#if 0 // UpdatetothenewAPI
    meanLumAlgorithm::Ref_t al0 = meanLumAlgorithm::create("zero", sm);
    
    std::vector<std::string> names = {"zero", "one", "two"};
    meanLumMultiChannelAlgorithm::Ref_t al3 = meanLumMultiChannelAlgorithm::create(names, sm);

    al0->run();
    
    al3->run();
#endif
//    std::unique_ptr<lum_func_t> f_ut(new lum_func_t (&algo_registry_ut));
//    
//    auto lum_func_ti = algo_library::instance().add(f_ut);
//    
//    auto return_val = algo_library::instance().call(lum_func_ti, al);
//    
//    EXPECT_TRUE(return_val);
    
}



TEST(ut_similarity, run)
{
    vector<roiWindow<P8U>> images(4);
    double tiny = 1e-10;
    
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



TEST (UT_mov_processor, basic)
{
    boost::filesystem::path test_filepath;
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    
    auto res = dgenv_ptr->asset_path("box-move.m4v");
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(boost::filesystem::exists(res.first));
    
    test_filepath = res.first;
    
    std::vector<std::string> names = {  "blue", "green", "red"};
    auto movProcRef = std::make_shared<mov_processor> ();
    auto m_movie = ocvPlayerRef ( new OcvVideoPlayer );
    if (m_movie->load (test_filepath.string())) {
        std::cout <<  m_movie->getFilePath() << " loaded successfully: " << std::endl;
        
        std::cout <<  " > Codec: "        << m_movie->getCodec() << std::endl;
        std::cout <<  " > Duration: "    << m_movie->getDuration() << std::endl;
        std::cout <<  " > FPS: "        << m_movie->getFrameRate() << std::endl;
        std::cout <<  " > Num frames: "    << m_movie->getNumFrames() << std::endl;
        std::cout <<  " > Size: "        << m_movie->getSize() << std::endl;
    }
    
    EXPECT_TRUE(m_movie->isLoaded());
        
    auto mFrameSet = qTimeFrameCache::create (m_movie);
    std::vector<mov_processor::channel_images_t> channels;
    mov_processor::load_channels_from_images(mFrameSet, channels);
    
#if 0 // Not Yet
    auto sp =  std::shared_ptr<sm_producer> ( new sm_producer () );
    sp->load_images(channels[0]);
    sp->operator()(0, 0);
        
        EXPECT_EQ(57, sp->frames_in_content());
        EXPECT_EQ(57, sp->shannonProjection().size () );
        EXPECT_EQ(false, svl::contains_nan(sp->shannonProjection().begin(),sp->shannonProjection().end()));
        
        std::ofstream f("/Users/arman/tmp/test.txt");
        for(auto i = sp->shannonProjection().begin(); i != sp->shannonProjection().end(); ++i) {
            f << *i << '\n';
        }
#endif
    
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
        avcc::avReader rr (test_filepath.string(), false);
        rr.setUserDoneCallBack(done_callback);
        rr.run ();
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

