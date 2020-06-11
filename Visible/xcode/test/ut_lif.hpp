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


//#ifdef INTERACTIVE
TEST (ut_liffile, browser_single_channel_basic)
{
    std::string filename ("two_channels.lif");
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    EXPECT_TRUE(res.second);
    auto lb_ref = lif_browser::create(res.first.string());
    EXPECT_EQ(1, lb_ref->get_all_series ().size ());
    
    BOOST_FOREACH(const lif_serie_data& si, lb_ref->get_all_series ()){
        /// Show in a window
        namedWindow(si.name(), cv::WINDOW_KEEPRATIO | cv::WINDOW_OPENGL);
        imshow(si.name(), si.poster());
        cv::waitKey(30.0);
    }
    
}
//#endif

    std::vector<std::string> names { "Green", "Gray" };

TEST (ut_liffile, browser_two_channel_basic)
{
    std::string filename ("two_channels.lif");
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    EXPECT_TRUE(res.second);
    auto lb_ref = lif_browser::create(res.first.string());
    EXPECT_EQ(1, lb_ref->get_all_series ().size ());

    BOOST_FOREACH(const lif_serie_data& si, lb_ref->get_all_series ()){
        EXPECT_EQ(2,si.channelCount());
        EXPECT_EQ(279,si.timesteps());
        EXPECT_EQ(512,si.buffer2d_dimensions()[0]);
        EXPECT_EQ(512,si.buffer2d_dimensions()[1]);
        EXPECT_EQ(0,si.ROIs2d()[0].tl().x);
        EXPECT_EQ(0,si.ROIs2d()[0].tl().y);
        EXPECT_EQ(0,si.ROIs2d()[1].tl().x);
        EXPECT_EQ(256,si.ROIs2d()[1].tl().y);
        EXPECT_EQ(512,si.ROIs2d()[0].size().width);
        EXPECT_EQ(256,si.ROIs2d()[0].size().height);
        EXPECT_EQ(512,si.ROIs2d()[1].size().width);
        EXPECT_EQ(256,si.ROIs2d()[1].size().height);
        EXPECT_EQ(names[0], si.channel_names()[0]);
        EXPECT_EQ(names[1], si.channel_names()[1]);
        std::cout << si << std::endl;

      }
}



#endif /* ut_lif_h */
