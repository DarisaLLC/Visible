// PatternsTest.cpp : Defines the entry point for the console application.

#include <iostream>
#include "gtest/gtest.h"
#include <memory>
#include "boost/filesystem.hpp"
#include "vision/histo.h"
#include "vision/drawUtils.hpp"
#include "vision/gradient.h"
#include "vision/ipUtils.h"
#include "pair.hpp"
#include "vision/roiWindow.h"
#include "vision/rowfunc.h"
#include "vision/gauss.hpp"
#include "vision/gmorph.hpp"
#include "vision/sample.hpp"
#include "imageIO/PnmIO.h"
#include "imageIO/PngIO.h"
#include "tinyGL/tinyGLManager.h"
#include "tinyGL/tinyGLImage.h"
#include "stl_utils.hpp"
#include "gtest_image_utils.hpp"
using namespace svl;
using namespace tinyGL;

namespace fs = boost::filesystem;
static test_utils::genv * dgenv_ptr;


template <typename P>
void showImage(roiWindow<P> & rootwin, iRect & db)
{
    static std::shared_ptr<tinyGL::Display> rootdisp(new tinyGL::DisplayImage<P>(db, "root", rootwin), null_deleter());
    tinyGL::DisplayManager::get_mutable_instance().addNewDisplay(rootdisp.get());
    tinyGL::DisplayManager::get_mutable_instance().run();
}

std::pair<std::pair<uint32_t, uint32_t>, uint32_t> get_psm_info(const fs::path & file)
{
    std::ifstream inpsm(file.string());
    std::string marker;
    std::pair<uint32_t, uint32_t> sizewh;
    uint32_t maxval;
    std::getline(inpsm, marker);
    std::cout << marker << std::endl;
    inpsm >> sizewh.first >> sizewh.second;
    inpsm >> maxval;
    return std::make_pair(sizewh, maxval);
}

roiWindow<P8U> image_io_read_and_convert_u16(const std::string & fqfn)
{
    fs::path pp(fqfn);
    const std::string extension = pp.extension().string();
    std::shared_ptr<uint16_t> buffer;
    int32_t width_actual, height_actual;

    if (extension == std::string(".png"))
    {
        PngReader pr;
        int width_actual, height_actual, od, ct;
        pr.readInfoAndPrep(fqfn.c_str(), &width_actual, &height_actual, &od, &ct);
        buffer = std::shared_ptr<uint16_t>(new uint16_t[width_actual * height_actual]);
        pr.readLum16(fqfn.c_str(), buffer.get());
        return convert(buffer.get(), width_actual, height_actual);
    }
    else if (extension == ".psm")
    {
        std::pair<std::pair<uint32_t, uint32_t>, uint32_t> info = get_psm_info(pp.string());
        buffer = std::shared_ptr<uint16_t>(new uint16_t[info.first.first * info.first.second]);
        int rc = read_psm_image_and_dim(pp.string().c_str(), buffer.get(), info.first.first * info.first.second * sizeof(uint16_t),
                                        &width_actual, &height_actual);
        return convert(buffer.get(), width_actual, height_actual);
    }
    return roiWindow<P8U>();
}


roiWindow<P8U> image_io_read_u8(const std::string & fqfn)
{
    fs::path pp(fqfn);
    const std::string extension = pp.extension().string();
    int32_t width_actual, height_actual;
    
    if (extension == std::string(".png"))
    {
        PngReader pr;
        int width_actual, height_actual, od, ct;
        pr.readInfoAndPrep(fqfn.c_str(), &width_actual, &height_actual, &od, &ct);
        roiWindow<P8U> buffer (width_actual, height_actual);
        pr.readLum8(fqfn.c_str(), buffer.pelPointer(0,0));
        return buffer;
    }
    return roiWindow<P8U>();
}


roiWindow<P8UC3> image_io_read_u8c3(const std::string & fqfn)
{
    fs::path pp(fqfn);
    const std::string extension = pp.extension().string();
    int32_t width_actual, height_actual;
    
    if (extension == std::string(".png"))
    {
        PngReader pr;
        int width_actual, height_actual, od, ct;
        pr.readInfoAndPrep(fqfn.c_str(), &width_actual, &height_actual, &od, &ct);
        roiWindow<P8UC3> buffer (width_actual, height_actual);
        pr.readRgb (fqfn.c_str(), buffer.pelPointer(0,0));
        return buffer;
    }
    return roiWindow<P8UC3>();
}




template <typename P>
void showImageAndResult(roiWindow<P> & rootwin, edgels::directed_features_t & data, iRect & db)
{
    std::shared_ptr<tinyGL::DisplayImage<P>> rootdisp(new tinyGL::DisplayImage<P>(db, "root", rootwin), null_deleter());
    rootdisp->setData(data);
    rootdisp->enableDrawData(true);

    tinyGL::DisplayManager::get_mutable_instance().addNewDisplay(rootdisp.get());
    tinyGL::DisplayManager::get_mutable_instance().run();
}
template void showImage(roiWindow<P8U> &, iRect &);
template void showImage(roiWindow<P16U> &, iRect &);

template void showImageAndResult(roiWindow<P8U> &, edgels::directed_features_t & data, iRect &);
template void showImageAndResult(roiWindow<P16U> &, edgels::directed_features_t & data, iRect &);


TEST(colorcalib, basic)
{
    std::string filename = "color-bars/all_color.png";
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    if (res.second)
    {
        iRect db(100, 150, 640, 480);
        
        double scale = 1000.0;
        roiWindow<P8UC3> rootwin = image_io_read_u8c3(res.first.string());
        showImage(rootwin, db);
     }

  
}

TEST(edgelinking, timing)
{
    std::string filename = "color-bars/Reduced_4_4_gray.png"; // "checker/checkerboard_u8.png";
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    if (res.second)
    {
        iRect db(100, 150, 640, 480);
        
        double scale = 1000.0;
        roiWindow<P8U> rootwin = image_io_read_u8(res.first.string());
        showImage(rootwin, db);
        
        int width = rootwin.width();
        int height = rootwin.height();

        roiWindow<P8U> mag(width, height);
        roiWindow<P8U> peaks(width, height);
        roiWindow<P8U> ang(width, height);

        Gradient(rootwin, mag, ang);

        unsigned int pks = SpatialEdge(mag, ang, peaks, 10, false);
        std::cout << pks << " true peaks detected " << std::endl;

      

        edgels::directed_features_t output;
        extract(mag, ang, peaks, output, 15);
        showImageAndResult(rootwin, output, db);
    }
}




/***
 **  Use --gtest-filter=*NULL*.*shared" to select https://code.google.com/p/googletest/wiki/V1_6_AdvancedGuide#Running_Test_Programs%3a_Advanced_Options
 */


int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: --assets <fully qualified path> to directory containing data \n";
        exit(1);
    }

    tinyGL::DisplayManager::get_mutable_instance().init(argc, argv);

    fs::path installpath = std::string(argv[0]);
   
    std::string current_exec_name = argv[0]; // Name of the current exec program
    std::vector<std::string> all_args;
    all_args.assign(argv + 1, argv + argc);

    fs::path input;
    bool got_input;
    got_input = false;
    std::string input_marker("--assets");

    for (unsigned ac = 0; ac < all_args.size(); ac++)
    {
        std::cout << all_args[ac] << std::endl;
        if (!got_input)
        {
            std::size_t found = all_args.at(ac).find(input_marker);
            
            if (found != std::string::npos && ac < (all_args.size() - 1))
            {
                input = all_args[ac + 1];
                got_input = true;
            }
        }
    }

    std::cout << input.string() << std::endl;
    if (!fs::exists(input))
    {
        std::cerr << input.string() << " does not exist. Use --help\n";
        exit(1);
    }

    fs::path assetpath = input;
    // Gtest will take over the allocated testing environment.
    dgenv_ptr = new test_utils::genv(installpath.string());
    ::testing::Environment * const DSgEnv = ::testing::AddGlobalTestEnvironment(dgenv_ptr);
    dgenv_ptr->add_content_directory(assetpath);

    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
    while (true)
        ;
}



#if 0

TEST(connected, basic)
{
    const  char *_img = {
        "              "
        "    *  0  *   "
        "   **  0  *   "
        "    *******   "
        "      *       "
        "   *          "
        "  ***         "
    };
    const unsigned char *img = (const unsigned char *)_img;
    int width = 14, height = 7;

    std::shared_ptr<uint8_t> out_uc(new uint8_t(width*height));

    ConnectedComponents cc(30);
    cc.connected(img, out_uc.get(), width, height,
                 std::equal_to<uint8_t>(),
                 false);

    //    00000000000000
    //        00002001002000
    //        00022001002000
    //        00002222222000
    //        00000020000000
    //        00030000000000
    //        00333000000000

    EXPECT_EQ(2, out_uc.get()[width + 5]);
    /* for(int r=0; r<height; ++r) {
    for(int c=0; c<width; ++c)
    putchar('0'+ out_uc.get()[r*width+c]);
    putchar('\n');
    }*/

}


TEST(basicGL, display)
{
    std::string name = "synthetic-checker.png";
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(name);
    if (res.second)
    {
        double scale = 1000.0;
        roiWindow<P8U> rootwin = image_io_read_and_convert_u16(res.first.string());

        iRect db (100, 150, 320, 240);
        showImage(rootwin, db);
    }

}

#endif
