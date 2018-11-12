
#include "cv_functions.h"
#include <iostream>
#include <fstream>


//openCV 3.x
#include "opencv2/stitching.hpp"


using namespace std;
using namespace cv;

bool try_use_gpu = false;

cv::Mat stitch (vector<Mat>& images)
{
    Mat pano;
    Stitcher stitcher = Stitcher::createDefault(try_use_gpu);
    Stitcher::Status status = stitcher.stitch(images, pano);
    
    if (status != Stitcher::OK)
        {
        cout << "Can't stitch images, error code = " << int(status) << endl;
            //return 0;
        }
    return pano;
}



