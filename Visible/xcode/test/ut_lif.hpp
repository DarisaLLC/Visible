//
//  ut_lif.hpp
//  VisibleGtest
//
//  Created by Arman Garakani on 9/12/18.
//
#ifndef ut_lif_h
#define ut_lif_h



#include <iostream>
#include "gtest/gtest.h"
#include <memory>
#include "boost/filesystem.hpp"
#include "vision/histo.h"
#include "vision/drawUtils.hpp"
#include "vision/ipUtils.h"
#include "core/pair.hpp"
#include "vision/roiWindow.h"
#include "vision/rowfunc.h"
#include "core/stl_utils.hpp"
#include "cinder_cv/cinder_xchg.hpp"
#include "otherIO/lifFile.hpp"
#include "core/gtest_env_utils.hpp"
#include "vision/roiMultiWindow.h"
#include "vision/opencv_utils.hpp"
#include "lif_content.hpp"
#include <boost/foreach.hpp>


std::string  expectedNames []
{
    "Series003"
    , "Series007"
    , "Series010"
    , "Series013"
    , "Series016"
    , "Series019"
    , "Series022"
    , "Series026"
    , "Series029"
    , "Series032"
    , "Series035"
    , "Series038"
    , "Series041"
    , "Series045"
};

std::string  expected3Names []
{
    "Series006"
    , "Series007"
    , "Series009"
    , "Series012"
    , "Series014"
    , "Series018"
    , "Series023"
    , "Series024"
    , "Series027"
    , "Series029"
    , "Series035"
    , "Series037"
    , "Preview038"
};

std::string  expected2Names []
{
    "AP SingleConcractHDCell"
};

#ifdef INTERACTIVE
TEST (ut_liffile, browser_single_channel_basic)
{
    std::string filename ("Sample1.lif");
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
      EXPECT_TRUE(res.second);
    auto lb_ref = lif_browser::create(res.first.string(),"IDLab_0");
    EXPECT_EQ(sizeof(expectedNames)/sizeof(std::string), lb_ref->get_all_series ().size ());
    
    BOOST_FOREACH(const lif_serie_data& si, lb_ref->get_all_series ()){
        /// Show in a window
        namedWindow(si.name(), CV_WINDOW_AUTOSIZE | WINDOW_OPENGL);
        imshow(si.name(), si.poster());
        cv::waitKey(30.0);
    }
    
}
#endif


TEST (ut_lifFile, single_channel)
{
    std::string filename ("Sample1.lif");
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    EXPECT_TRUE(res.second);
    auto lif = lifIO::LifReader::create(res.first.string(), "IDLab_0");
  //  cout << "LIF version "<<lif->getVersion() << endl;
    EXPECT_EQ(14, lif->getNbSeries() );
    size_t serie = 0;

    lifIO::LifSerie& se0 = lif->getSerie(serie);
    const std::vector<size_t>& dims = se0.getSpatialDimensions();
    EXPECT_EQ(512, dims[0]);
    EXPECT_EQ(128, dims[1]);
    std::vector<std::string> series;
    for (auto ss = 0; ss < lif->getNbSeries(); ss++)
        series.push_back (lif->getSerie(ss).getName());
    
    EXPECT_EQ(sizeof(expectedNames)/sizeof(std::string), series.size ());
    
    for (auto sn = 0; sn < series.size(); sn++)
    {
        EXPECT_EQ(0, expectedNames[sn].compare(series[sn]));
        EXPECT_EQ(250, lif->getSerie(sn).getNbTimeSteps());
        EXPECT_EQ(65536, lif->getSerie(sn).getNbPixelsInOneTimeStep ());
        EXPECT_EQ(1, lif->getSerie(sn).getChannels().size());
    }
    
    
    roiWindow<P8U> slice ((int)dims[0], (int)dims[1]);
    lif->getSerie(0).fill2DBuffer(slice.rowPointer(0));
    histoStats h;
    h.from_image(slice);
    EXPECT_NEAR(h.mean(), 114.271, 0.001);
    EXPECT_NEAR(h.median(), 114.0, 0.001);
    EXPECT_NEAR(h.min(), 44.0, 0.001);
    EXPECT_NEAR(h.max(), 255.0, 0.001);

}


TEST (ut_lifFile, triple_channel)
{
    std::string filename ("3channels.lif");
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    EXPECT_TRUE(res.second);
    auto lif = lifIO::LifReader::create(res.first.string(), "IDLab_0");
    cout << "LIF version "<<lif->getVersion() << endl;
    EXPECT_EQ(13, lif->getNbSeries() );
    size_t serie = 0;
    lifIO::LifSerie& se0 = lif->getSerie(serie);
    const std::vector<size_t>& dims = se0.getSpatialDimensions();
    EXPECT_EQ(512, dims[0]);
    EXPECT_EQ(128, dims[1]);
    std::vector<std::string> series;
    for (auto ss = 0; ss < lif->getNbSeries(); ss++)
        series.push_back (lif->getSerie(ss).getName());
    
    EXPECT_EQ(sizeof(expected3Names)/sizeof(std::string), series.size ());
    
    {
        EXPECT_EQ(0, expected3Names[0].compare(series[0]));
        EXPECT_EQ(31, lif->getSerie(0).getNbTimeSteps());
        EXPECT_EQ(65536, lif->getSerie(0).getNbPixelsInOneTimeStep ());
        EXPECT_EQ(3, lif->getSerie(0).getChannels().size());
    }
    for (auto sn = 1; sn < (series.size()-1); sn++)
    {
        EXPECT_EQ(0, expected3Names[sn].compare(series[sn]));
        EXPECT_EQ(500, lif->getSerie(sn).getNbTimeSteps());
        EXPECT_EQ(65536, lif->getSerie(sn).getNbPixelsInOneTimeStep ());
        EXPECT_EQ(3, lif->getSerie(sn).getChannels().size());
    }
    {
        EXPECT_EQ(0, expected3Names[series.size()-1].compare(series[series.size()-1]));
        EXPECT_EQ(1, lif->getSerie(series.size()-1).getNbTimeSteps());
        EXPECT_EQ(262144, lif->getSerie(series.size()-1).getNbPixelsInOneTimeStep ());
        EXPECT_EQ(3, lif->getSerie(series.size()-1).getChannels().size());
    }
    
//    roiWindow<P8U> slice ((int) dims[0], (int)dims[1]);
//    lif->getSerie(0).fill2DBuffer(slice.rowPointer(0));
//    histoStats h;
//    h.from_image(slice);
//    EXPECT_NEAR(h.mean(), 5.82, 0.001);
//    EXPECT_NEAR(h.median(), 0.0, 0.001);
//    EXPECT_NEAR(h.min(), 0.0, 0.001);
//    EXPECT_NEAR(h.max(), 205.0, 0.001);
    
    std::vector<std::string> names { "green", "red", "gray" };
    
    {
        lifIO::LifSerie& lls = lif->getSerie(0);
        roiFixedMultiWindow<P8UP3> oneBy3 (names, lls.getTimestamps()[0]);
        lls.fill2DBuffer(oneBy3.rowPointer(0), 0);
        
        EXPECT_EQ(oneBy3.timestamp(),lls.getTimestamps()[0] );
        histoStats h;
        h.from_image(oneBy3.plane(0));
        EXPECT_NEAR(h.mean(), 5.82, 0.001);
        EXPECT_NEAR(h.median(), 0.0, 0.001);
        EXPECT_NEAR(h.min(), 0.0, 0.001);
        EXPECT_NEAR(h.max(), 205.0, 0.001);
    }
    
    {
        lifIO::LifSerie& lls = lif->getSerie(0);
        roiFixedMultiWindow<P8UP3> oneBy3 (names, lls.getTimestamps()[30]);
        lif->getSerie(0).fill2DBuffer(oneBy3.rowPointer(0), 30);
        EXPECT_EQ(oneBy3.timestamp(),lls.getTimestamps()[30] );
        
        histoStats h;
        h.from_image(oneBy3.plane(2));
        EXPECT_NEAR(h.mean(), 125.4625, 0.001);
        EXPECT_NEAR(h.median(), 126.0, 0.001);
        EXPECT_NEAR(h.min(), 88.0, 0.001);
        EXPECT_NEAR(h.max(), 165.0, 0.001);
        EXPECT_NEAR(h.mode(), 125.0, 0.001);
    }
}


TEST (ut_lifFile, two_channel)
{
    std::string filename ("2channels.lif");
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    EXPECT_TRUE(res.second);
    auto lif = lifIO::LifReader::create(res.first.string(), "IDLab_0");
    cout << "LIF version "<<lif->getVersion() << endl;
    EXPECT_EQ(1, lif->getNbSeries() );
    size_t serie = 0;
    lifIO::LifSerie& se0 = lif->getSerie(serie);
    const std::vector<size_t>& dims = se0.getSpatialDimensions();
    EXPECT_EQ(512, dims[0]);
    EXPECT_EQ(256, dims[1]);
    std::vector<std::string> series;
    for (auto ss = 0; ss < lif->getNbSeries(); ss++)
        series.push_back (lif->getSerie(ss).getName());
    
    EXPECT_EQ(sizeof(expected2Names)/sizeof(std::string), series.size ());
    
    {
        EXPECT_EQ(0, expected2Names[0].compare(series[0]));
        EXPECT_EQ(279, lif->getSerie(0).getNbTimeSteps());
        EXPECT_EQ(131072, lif->getSerie(0).getNbPixelsInOneTimeStep ());
        EXPECT_EQ(2, lif->getSerie(0).getChannels().size());
    }
 
    
    //    roiWindow<P8U> slice ((int) dims[0], (int)dims[1]);
    //    lif->getSerie(0).fill2DBuffer(slice.rowPointer(0));
    //    histoStats h;
    //    h.from_image(slice);
    //    EXPECT_NEAR(h.mean(), 5.82, 0.001);
    //    EXPECT_NEAR(h.median(), 0.0, 0.001);
    //    EXPECT_NEAR(h.min(), 0.0, 0.001);
    //    EXPECT_NEAR(h.max(), 205.0, 0.001);
    
    std::vector<std::string> names { "green", "gray" };
    
    {
        lifIO::LifSerie& lls = lif->getSerie(0);
        roiFixedMultiWindow<P8UP2,512,256,2> oneBy2 (names, lls.getTimestamps()[0]);
        lls.fill2DBuffer(oneBy2.rowPointer(0), 0);
        
        EXPECT_EQ(oneBy2.timestamp(),lls.getTimestamps()[0] );
        histoStats h;
        h.from_image(oneBy2.plane(0));
        EXPECT_NEAR(h.mean(), 3.774, 0.001);
        EXPECT_NEAR(h.min(), 0.0, 0.001);
        EXPECT_NEAR(h.max(), 255.0, 0.001);
    }
    
    {
        lifIO::LifSerie& lls = lif->getSerie(0);
        roiFixedMultiWindow<P8UP2,512,256,2> oneBy2 (names, lls.getTimestamps()[0]);
        lls.fill2DBuffer(oneBy2.rowPointer(0), 0);
        
        EXPECT_EQ(oneBy2.timestamp(),lls.getTimestamps()[0] );
        histoStats h;
        h.from_image(oneBy2.plane(1));
        EXPECT_NEAR(h.mean(), 123.630, 0.001);
        EXPECT_NEAR(h.min(), 46.0, 0.001);
        EXPECT_NEAR(h.max(), 203.0, 0.001);
    }
    
   
}


#endif /* ut_lif_h */
