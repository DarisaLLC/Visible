//
//  main.cpp
//  sscli
//
//  Created by Arman Garakani on 3/10/17.
//
//

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#include <iostream>
#include <fstream>
#include <iterator>


#include <iostream>
#include <string>
#include "sm_producer.h"
#include "core/core.hpp"

using namespace std;

using namespace std;

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}


struct cb_similarity_producer
{
    cb_similarity_producer (const std::string& imageDir, bool auto_run = false)
    {
        m_imagedir = imageDir;
        m_auto = auto_run;
    }
    
    int run ()
    {
        try
        {
            sp =  std::shared_ptr<sm_producer> ( new sm_producer () );
            std::function<void (int&,double&)> frame_loaded_cb = boost::bind (&cb_similarity_producer::signal_frame_loaded, this, _1, _2);
            std::function<void ()> content_loaded_cb = std::bind (&cb_similarity_producer::signal_content_loaded, this);
            boost::signals2::connection fl_connection = sp->registerCallback(frame_loaded_cb);
            boost::signals2::connection ml_connection = sp->registerCallback(content_loaded_cb);
            
            int error;
            //            if (m_auto) sp->set_auto_run_on ();
           
            sp->load_image_directory(m_imagedir);
            sp->operator()(0, sp->frames_in_content());
            
//            auto fdone = std::async(&sm_producer::load_image_directory, sp, m_imagedir);
//            fdone.wait();
            
//            if (fdone.get() )
//            {
//            }
            
        }
        catch (...)
        {
            return 1;
        }
        return 0;
        
    }
    void signal_content_loaded ()
    {
        movie_loaded = true;
        std::cout << "Content Loaded " << std::endl;
    }
    void signal_frame_loaded (int& findex, double& timestamp)
    {
        frame_indices.push_back (findex);
        frame_times.push_back (timestamp);
        std::cout << frame_indices.size() << std::endl;
    }
    
    std::shared_ptr<sm_producer> sp;
    bool m_auto;
    std::vector<int> frame_indices;
    std::vector<double> frame_times;
    std::string m_imagedir;

    bool movie_loaded;
    void clear_movie_loaded () { movie_loaded = false; }
    bool is_movie_loaded () { return movie_loaded; }
    int frameCount () { if (sp) return sp->frames_in_content (); return -1; }
    
   };



int main(int ac, char* av[])
{
    std::string image_dir;
    std::string output_file;
    
    try {
        std::string inputPth, outputPth;
        unsigned int processedLevel;
        int component;
        float lowerThreshold, upperThreshold;
        po::options_description desc("Options");
        desc.add_options()
        ("help,h", "Displays this message")
        ("input,i", po::value<std::string>(&inputPth)->required(), "Path to image directory")
        ("output,o", po::value<std::string>(&output_file)->required(), "Path to outputfile ")
//        ("level,l", po::value<unsigned int>(&processedLevel)->default_value(0), "Set the level to be processed")
//        ("component,c", po::value<int>(&component)->default_value(-1), "Color component to select for threshold, if none, threshold all.")
        ;
        

        po::variables_map vm;
        try {
            po::store(po::parse_command_line(ac, av, desc),vm);
            if (!vm.count("input")) {
                cout << "Usage: sscli input output [options]" << endl;
            }
            else
            {
                image_dir = vm["input"].as<std::string>();
                
                cout << vm["input"].as<std::string>() << std::endl;
            }
            
            if (vm.count("help")) {
                std::cout << desc << std::endl;
                return 0;
            }
            po::notify(vm);
        }
        catch (boost::program_options::required_option& e) {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << "Use -h or --help for usage information" << std::endl;
            return 1;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Unhandled exception: "
        << e.what() << ", application will now exit" << std::endl;
        return 2;
    }
    
    if(!image_dir.empty())
    {
        cb_similarity_producer runner (image_dir);
        runner.run();
        
    }
}



