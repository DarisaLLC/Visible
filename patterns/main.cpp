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
#include "vision/corners.hpp"
#include "shmem_io/PnmIO.h"
#include "shmem_io/PngIO.h"
#include "tinyGL/tinyGLManager.h"
#include "tinyGL/tinyGLImage.h"
#include "stl_utils.hpp"
#include "gtest_image_utils.hpp"
using namespace DS;
using namespace tinyGL;

namespace fs = boost::filesystem;
static test_utils::genv * dgenv_ptr;


template <typename P>
void showImage(roiWindow<P> & rootwin, iRect & db)
{
    std::shared_ptr<tinyGL::Display> rootdisp(new tinyGL::DisplayImage<P>(db, "root", rootwin), null_deleter());
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


template <typename P>
void showImageAndResult(roiWindow<P> & rootwin, chains_directed_t & data, iRect & db)
{
    std::shared_ptr<tinyGL::DisplayImage<P>> rootdisp(new tinyGL::DisplayImage<P>(db, "root", rootwin), null_deleter());
    rootdisp->setData(data);
    rootdisp->enableDrawData(true);

    tinyGL::DisplayManager::get_mutable_instance().addNewDisplay(rootdisp.get());
    tinyGL::DisplayManager::get_mutable_instance().run();
}
template void showImage(roiWindow<P8U> &, iRect &);
template void showImage(roiWindow<P16U> &, iRect &);

template void showImageAndResult(roiWindow<P8U> &, chains_directed_t & data, iRect &);
template void showImageAndResult(roiWindow<P16U> &, chains_directed_t & data, iRect &);


char * pi341[] =
    {
     "0000000000000000",
     "0000000000000000",
     "0000000000000000",
     "0000000000000000",
     "0000011110000000",
     "0000001111000000",
     "0000000111100000",
     "0000000011110000",
     "0000000000000000"
     "0000000000000000",
     "0000000000000000",
     "0000000000000000", 0};

char * pi341234[] =
    {
     "0000000000000000",
     "0000000000000000",
     "0000000000000000",
     "0000000000000000",
     "0000012340000000",
     "0000001234000000",
     "0000000123400000",
     "0000000012340000",
     "0000000000000000"
     "0000000000000000",
     "0000000000000000",
     "0000000000000000", 0};

//TEST(basicTheta, angle_units)
//{
//	for (int aa = 0; aa <= 255; aa++)
//	{
//		uAngle8 a8(aa);
//		uRadian r(a8);
//		std::cout << (int)a8.basic() << " ~~~ " << r.Double() << std::endl;
//	}
//}
//




TEST(edgelinking, timing)
{
    std::string filename = "Checkerboard_pattern.png";
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(filename);
    if (res.second)
    {
        double scale = 1000.0;
        roiWindow<P8U> rootwin = image_io_read_and_convert_u16(res.first.string());
        int width = rootwin.width();
        int height = rootwin.height();

        roiWindow<P8U> mag(width, height);
        roiWindow<P8U> peaks(width, height);
        roiWindow<P8U> ang(width, height);

        Gradient(rootwin, mag, ang);

        unsigned int pks = SpatialEdge(mag, ang, peaks, 10, false);
        std::cout << pks << " true peaks detected " << std::endl;
        chains_directed_t output;
        extract(mag, ang, peaks, output, 15);
        iRect db(100, 150, 640, 480);


        showImageAndResult(ang, output, db);
    }
}


TEST(basicU8, motioncenter)
{
    roiWindow<P8U> tmp(16, 16);
    roiWindow<P8U> mag(16, 16);
    roiWindow<P8U> ang(16, 16);
    roiWindow<P8U> peaks(16, 16);

    tmp.set(255);
    iPair tl(tmp.width() / 4, tmp.height() / 4);
    iPair br(4 * tmp.width() / 4 - 1, 4 * tmp.height() / 4 - 1);
    roiWindow<P8U> box(tmp, tl.x(), tl.y(), br.x() - tl.x(), br.y() - tl.y());
    box.set(0xef);

    Gradient(tmp, mag, ang);

    unsigned int pks = SpatialEdge(mag, ang, peaks, 1, false);
    EXPECT_EQ(pks, 37);
    fPair center;
    GetMotionCenter(peaks, ang, center);
    fPair correct(3.52f, 3.52f);
    EXPECT_EQ(true, fabs(center.first - correct.first) < 0.1f);
    EXPECT_EQ(true, fabs(center.second - correct.second) < 0.1f);

    chains_directed_t lines;
    extract(mag, ang, peaks, lines, 0);
}

TEST(synth, basic)
{
    roiWindow<P8U> tmp(17, 17);
    tmp.set();
    tmp.setPixel(8, 8, 255);
    tmp.setPixel(4, 4, 255);
    tmp.setPixel(12, 12, 255);
    tmp.setPixel(4, 12, 255);
    tmp.setPixel(12, 4, 255);
    roiWindow<P8U> smooth(tmp.width(), tmp.height());
    Gauss3by3(tmp, smooth);

    //    smooth.print_pixel();
}

TEST(basicip, timing)
{
    std::string name = "Calibration_1461000001_left.psm";
    std::pair<test_utils::genv::path_t, bool> res = dgenv_ptr->asset_path(name);
    if (res.second)
    {
        int width(640), height(480);
        std::shared_ptr<uint16_t> buffer(new uint16_t[width * height]);
        EXPECT_EQ(0, read_psm_image_and_dim(res.first.string().c_str(), buffer.get(), width * height * sizeof(uint16_t), &width, &height));

        double scale = 1000.0;
        roiWindow<P8U> rootwin;

        {
            std::clock_t start;
            start = std::clock();
            rootwin = convert(buffer.get(), width, height);
            double endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
            std::cout << " Conversion & Normalization to 8 bit using LUT " << endtime * scale << " millieseconds per " << std::endl;
        }
        roiWindow<P8U> smooth(width, height);

        {
            std::clock_t start;
            start = std::clock();
            Gauss3by3(rootwin, smooth);
            double endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
            std::cout << " 3x3 Gaussian + 1x3 gauss on the outer 1 pixel " << endtime * scale << " millieseconds per " << std::endl;
        }
    }
}


#if 0

TEST(basicmorph, area)
{
    roiWindow<P8U> pels(17, 25);
    roiWindow<P8U> dst (17, 25);
    
    pels.set();
    pels.setPixel(8, 12, 255);
    
    PixelMin3by3 (pels, dst);
    
    EXPECT_EQ(dst.getPixel(8, 12), 64);
    
    pels.print_pixel();
    dst.print_pixel();
}

TEST(basicgauss, area)
{
    roiWindow<P8U> pels(17, 25);
    roiWindow<P8U> dst (17, 25);
    
    pels.set();
    pels.setPixel(8, 12, 255);
    
    Gauss3by3 (pels, dst);
    
    EXPECT_EQ(dst.getPixel(8, 12), 64);
    
    //pels.print_pixel();
    //dst.print_pixel();
}

TEST(basicgauss, edge)
{
    roiWindow<P8U> pels(17, 25);
    roiWindow<P8U> dst (17, 25);
    
    pels.set();
    pels.setPixel(16, 24, 255);
    
    Gauss3by3 (pels, dst);
    
    EXPECT_EQ(dst.getPixel(16, 24), 255);
    EXPECT_EQ(dst.getPixel(15, 23), 16);
    EXPECT_EQ(dst.getPixel(15, 24), 48);
    EXPECT_EQ(dst.getPixel(16, 23), 48);
    
    pels.set();
    pels.setPixel(0, 0, 255);
    
    Gauss3by3 (pels, dst);
    
    EXPECT_EQ(dst.getPixel(0, 0), 255);
    EXPECT_EQ(dst.getPixel(1, 1), 16);
    EXPECT_EQ(dst.getPixel(0, 1), 48);
    EXPECT_EQ(dst.getPixel(1, 0), 48);
    
    pels.set();
    pels.setPixel(0, 24, 255);
    
    Gauss3by3 (pels, dst);
    
    EXPECT_EQ(dst.getPixel(0, 24), 255);
    EXPECT_EQ(dst.getPixel(1, 23), 16);
    EXPECT_EQ(dst.getPixel(0, 23), 48);
    EXPECT_EQ(dst.getPixel(1, 24), 48);
    
    
    pels.set();
    pels.setPixel(16, 0, 255);
    
    Gauss3by3 (pels, dst);
    
    EXPECT_EQ(dst.getPixel(16, 0), 255);
    EXPECT_EQ(dst.getPixel(15, 1), 16);
    EXPECT_EQ(dst.getPixel(15, 0), 48);
    EXPECT_EQ(dst.getPixel(16, 1), 48);
    
    //    pels.print_pixel();
    //    dst.print_pixel();
}


TEST(basicU8, histo)
{
    
    char * frame[] =
    {"00000000000000000000",
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
}


TEST(basicU8, hyst)
{
    char * frame[] =
    {
        "00000000000000000000",
        "00001020300000000000",
        "00000000000000000000",
        "00000112000000000000",
        "00000000300000000000",
        "00000000000000000000",
        "00000000000000000000",
        0};
    
    roiWindow<P8U> fpels(22, 7);
    DrawShape(fpels, frame);
    roiWindow<P8U> dst;
    int32_t got = -1;
    hysteresisThreshold(fpels, dst, 1, 3, got, 0);
    EXPECT_EQ(5, got);
}


TEST(basicU8, gradient)
{
    char * shape[] =
    {
        "00000000000000000000",
        "00000000000000000000",
        "00000000zzz000000000",
        "0000000zzzzz00000000",
        "000000zzzzzzz0000000",
        "000000zzzzzzz0000000",
        "000000zzzzzzz0000000",
        "0000000zzzzz00000000",
        "00000000zzz000000000",
        "00000000000000000000",
        "00000000000000000000",
        0};
    int w = 20, h = 11;
    roiWindow<P8U> pels(w, h);
    roiWindow<P8U> mag(w, h);
    roiWindow<P8U> ang(w, h);
    roiWindow<P8U> peaks(w, h);
    DrawShape(pels, shape);
    Gradient(pels, mag, ang);
    
    unsigned int pks = SpatialEdge(mag, ang, peaks, 1, false);
    EXPECT_EQ(pks, 26);
}

TEST(timing8, gradient)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    double endtime;
    std::clock_t start;
    int l;
    
    
    int w = 1920, h = 1080;
    roiWindow<P8U> pels(w, h);
    roiWindow<P8U> mag(w, h);
    roiWindow<P8U> ang(w, h);
    roiWindow<P8U> peaks(w, h);
    
    start = std::clock();
    Gradient(pels, mag, ang);
    unsigned int pks = SpatialEdge(mag, ang, peaks, 1, false);
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    double scale = 1000.0;
    std::cout << " Edge: Mag, Ang, NonMax: 1920 * 1080 * 8 bit " << endtime * scale << " millieseconds per " << std::endl;
}


#if 1


TEST(basic, corr8)
{
    std::shared_ptr<uint8_t> img1 = test_utils::create_trig(1920, 1080);
    std::shared_ptr<uint8_t> img2 = test_utils::create_trig(1920, 1080);
    
    basicCorrRowFunc<uint8_t> corrfunc(img1.get(), img2.get(), 1920, 1920, 1920, 1080);
    corrfunc.areaFunc();
    CorrelationParts cp;
    corrfunc.epilog(cp);
    EXPECT_EQ(cp.r(), 1);
}


TEST(basic, corr16)
{
    std::shared_ptr<uint16_t> img1 = test_utils::create_easy_ramp16(1920, 1080);
    std::shared_ptr<uint16_t> img2 = test_utils::create_easy_ramp16(1920, 1080);
    
    basicCorrRowFunc<uint16_t> corrfunc(img1.get(), img2.get(), 1920, 1920, 1920, 1080);
    corrfunc.areaFunc();
    CorrelationParts cp;
    corrfunc.epilog(cp);
    EXPECT_EQ(cp.r(), 1);
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

TEST(timing16, corr)
{
    std::shared_ptr<uint16_t> img1 = test_utils::create_easy_ramp16(1920, 1080);
    std::shared_ptr<uint16_t> img2 = test_utils::create_easy_ramp16(1920, 1080);
    double endtime;
    std::clock_t start;
    int l;
    start = std::clock();
    int num_loops = 1000;
    // Time setting and resetting
    for (l = 0; l < num_loops; ++l)
    {
        basicCorrRowFunc<uint16_t> corrfunc(img1.get(), img2.get(), 1920, 1920, 1920, 1080);
        corrfunc.areaFunc();
        CorrelationParts cp;
        corrfunc.epilog(cp);
    }
    endtime = (std::clock() - start) / ((double)CLOCKS_PER_SEC);
    double scale = 1000.0 / (num_loops);
    std::cout << " Correlation: 1920 * 1080 * 16 bit " << endtime * scale << " millieseconds per " << std::endl;
}


#endif

#endif

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
