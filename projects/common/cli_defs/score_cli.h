#ifndef __SCORECLI__
#define __SCORECLI__

#include <utility>
#include <string>
#include <iostream>
#include <strstream>

#include "opencv2/opencv.hpp"

using namespace std;

namespace cli_defs
{
    class skin_bag
    {
    public:
        
        struct result_base
        {
            float redness;
            float thickness;
            float scale;
            float size;
        };

        typedef std::pair<uint8_t,std::vector<std::pair< uint8_t, float>>> zscore_result_t;
        
        
        skin_bag () : auto_mask_ruler_dot (true), debug_output (false), 
        no_graphics(true), scale_input (auto_mask_ruler_dot ? 1.0 : 0.333), left_tail_cut (0.09)
        {}
        
        bool auto_mask_ruler_dot;
        bool no_graphics;
        double scale_input;
        bool write_out_masks;
        bool debug_output;
        
        float left_tail_cut;
        
        cv::Mat input;
        
        cv::Mat rg;
        cv::Mat hueChannel;
        cv::Mat satChannel;
        cv::Mat valChannel;
        cv::Mat mask;
        cv::Mat RedChannel;
        cv::Mat GreenChannel;
        cv::Mat BlueChannel;

        result_base truth;
        result_base check;
        
        
        std::string image_in_fqfn;
        std::string mask_out_fqfn;
        
        zscore_result_t zscore;
        float skin_info;
        
        std::string output ()
        {
            std::string outp =
              image_in_fqfn  +  "," +
             to_string(truth.redness)  +  ","  +   to_string(check.redness)  +  "," +
            to_string(truth.thickness)  +  ","  +   to_string(check.thickness)  +  "," +
              to_string(truth.scale)  +  ","  +   to_string(check.scale)  +  "," +
              to_string(truth.size)  +  ","  +   to_string(check.size)  +  '\n';

            return outp;
        }
        
    };
    
    struct spectrum_bag
    {
        std::vector<std::vector<float>> mRGBhists;
        std::vector<uint32_t> rpeak;
        std::vector<uint32_t> gpeak;
        std::vector<uint32_t> bpeak;
        uint32_t nopeak;
    };
    
    void measure_skin (const cv::Mat&, skin_bag& );
    
}



#endif
