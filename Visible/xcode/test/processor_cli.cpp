//
//  processor_cli.cpp
//  VisibleGtest
//
//  Created by Arman Garakani on 3/9/19.
//

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <mutex>

#include <boost/foreach.hpp>
#include "boost/algorithm/string.hpp"
#include <boost/range/irange.hpp>
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"


#include "opencv2/highgui.hpp"
#include "vision/roiWindow.h"
#include "vision/opencv_utils.hpp"
#include "vision/gauss.hpp"
#include "vision/gradient.h"
#include "core/angle_units.h"

using namespace boost;

using namespace std;
namespace fs = boost::filesystem;
using paths_vector_t=std::vector<fs::path>;



bool pick_option(std::vector<std::string>& args, const std::string& option){
    
    auto it = find(args.begin(), args.end(), "-" + option);
    
    bool found = it != args.end();
    if (found)
        args.erase(it);
    
    return found;
}

std::string pick_option(std::vector<std::string>& args, const std::string& option, const std::string& default_value){
    auto arg = "-" + option;
    
    for (auto it = args.begin(); it != args.end(); it++) {
        if (*it == arg) {
            auto next = it + 1;
            if (next == args.end())
                continue;
            auto result = *next;
            args.erase(it, it + 2);
            return result;
        }
    }
    return default_value;
}

static std::mutex m_mutex;

int imagePaths( const std::string& imageDir, paths_vector_t& frame_paths, const std::vector<std::string>& supported_extensions = { ".jpg", ".png", ".JPG", ".jpeg"})
{
    
    if ( !boost::filesystem::exists( imageDir ) ) return -1;
    if ( !boost::filesystem::is_directory( imageDir ) ) return -1;
    
    paths_vector_t tmp_framePaths;
    
    // make a list of all image files in the directory
    // in to the tmp vector
    filesystem::directory_iterator end_itr;
    for( filesystem::directory_iterator i( imageDir ); i != end_itr; ++i )
    {
        std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
        // skip if not a file
        if( !filesystem::is_regular_file( i->status() ) ) continue;
        
        std::cout << "Checking " << i->path().string();
        
        if (std::find( supported_extensions.begin(), supported_extensions.end(), i->path().extension()  ) != supported_extensions.end())
        {
            std::cout << "  Extension matches " << std::endl;
            tmp_framePaths.push_back( i->path().string() );
        }
        else
        {
            std::cout << "Skipped " << i->path().string() << std::endl;
            continue;
        }
    }

    std::cout << tmp_framePaths.size () << " Files "  << std::endl;
    frame_paths = tmp_framePaths;
    return static_cast<int> (frame_paths.size());
}

#define cvMatOfroiWindow(a,b,cvType) cv::Mat b ((a).height(),(a).width(), cvType,(a).pelPointer(0,0), size_t((a).rowUpdate()))

/*
 *
 *  Main program:
 *   This program reads the following parameters from the console and
 *   then computes the optical flow:
 *   -i
 *   -o          first image
 *   -v     switch on/off messages
 *
 */

int main(int argc, char *argv[]) {

    using namespace std::chrono;
    system_clock::time_point today = system_clock::now();
    time_t tt;

    tt = system_clock::to_time_t(today);
    std::cerr << "today is: " << ctime(&tt);

    auto rowPointer = [] (void* data, size_t step, int32_t row ) { return reinterpret_cast<void*>( reinterpret_cast<uint8_t*>(data) + row * step ); };
    
    
    // process input
    std::vector<std::string> args(argv, argv + argc);
    auto input_dir = pick_option(args, "i", "");
    auto outputput_dir = pick_option(args, "o", "");
    auto verbose = pick_option(args, "v");

    paths_vector_t ipaths;
    auto ret = imagePaths(input_dir, ipaths);
    std::cout << " Returned " << ret << std::endl;

    
    /// Show in a window
    namedWindow( " TP Display ", CV_WINDOW_KEEPRATIO  | WINDOW_OPENGL);
    
    for (const auto ipath : ipaths){
        
        if(verbose)
            std::cout << ipath.c_str() << std::endl;
        
        // Only Gray -- get a cv::Mat and copy to a roiWindow
        cv::Mat out0 = cv::imread(ipath.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
        cv::GaussianBlur(out0, out0, cvSize(3,1), 0.0);
        unsigned cols = out0.cols;
        unsigned rows = out0.rows;
        roiWindow<P8U> rw(cols,rows);
        for (auto row = 0; row < rows; row++) {
            std::memcpy(rw.rowPointer(row), rowPointer(out0.data, out0.step, row), cols);
        }

        // Create gradient result images
        roiWindow<P8U> mag (cols,rows);
        roiWindow<P8U> ang (cols,rows);
        roiWindow<P8U> peaks (cols,rows);

        Gradient(rw, mag, ang);
        SpatialEdge(mag, ang, peaks, 5);
        
        for (auto row : irange(0u, rows)){
            for (auto col : irange(0u, cols)){

                if(peaks.getPixel(col,row)){
                    uint8_t r8 = ang.getPixel(col,row);
                    uAngle8 u8( r8 );
                    uDegree ud (u8);
                    ellipse(out0, cvPoint(col,row), cv::Size(2,1),ud.basic(), 0, 360, Scalar::all(r8));
                }
            }
        }

        cvMatOfroiWindow(mag, im, CV_8UC1);
        
//        cv::Mat im (mag.height(),ang.width(), CV_8UC(1),ang.pelPointer(0,0), size_t(ang.rowUpdate()));
        
        imshow(" TP Display ", out0);
        cv::waitKey(-1);
    }
}


