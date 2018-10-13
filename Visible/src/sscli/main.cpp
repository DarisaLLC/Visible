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
#include <strstream>
#include "core/stl_utils.hpp"
#include <iostream>
#include <string>
#include "sm_producer.h"
#include "core/core.hpp"

using namespace std;

using namespace stl_utils;
using namespace boost;

namespace fs=boost::filesystem;


// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

template <class Val>
void Out(const std::deque<Val>& v)
{
    if (v.size() > 1)
        std::copy(v.begin(), v.end() - 1, std::ostream_iterator<Val>(std::cout, ", "));
    if (v.size())
        std::cout << v.back() << std::endl;
}

typedef std::deque<std::string> string_deque_t;
typedef std::deque<string_deque_t> string_deque_array_t;

size_t imageDirectoryOutput (  std::shared_ptr<sm_producer>& sp,   string_deque_array_t&  output,
                             sm_producer::outputOrderOption ooo = sm_producer::outputOrderOption::sorted);

struct cb_similarity_producer
{
    cb_similarity_producer (const std::string& imageDir, bool auto_run = false)
    {
        m_imagedir = imageDir;
        m_auto = auto_run;
        last_output.resize(0);
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
            
            sp->load_image_directory(m_imagedir,  sm_producer::sizeMappingOption::mostCommon);
            sp->operator()(0, sp->frames_in_content());
            auto resc = imageDirectoryOutput ( sp,  last_output);
        
            
        }
         catch ( const std::exception & ex )
        {
            std::cout << ex.what () << std::endl;
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
    string_deque_array_t  last_output;

    bool movie_loaded;
    void clear_movie_loaded () { movie_loaded = false; }
    bool is_movie_loaded () { return movie_loaded; }
    int frameCount () { if (sp) return sp->frames_in_content (); return -1; }
    
   };


std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr)
{
    return std::shared_ptr<std::ofstream>(ifstream_ptr, ofStreamDeleter());
}

std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename)
{
    return make_shared_ofstream(new std::ofstream(filename, std::ofstream::out));
}

std::shared_ptr<std::ifstream> make_shared_ifstream(std::ifstream * ifstream_ptr)
{
    return std::shared_ptr<std::ifstream>(ifstream_ptr, ifStreamDeleter());
}

std::shared_ptr<std::ifstream> make_shared_ifstream(std::string filename)
{
    return make_shared_ifstream(new std::ifstream(filename, std::ifstream::in));
}

size_t imageDirectoryOutput (  std::shared_ptr<sm_producer>& sp,   string_deque_array_t&  output,
                             sm_producer::outputOrderOption ooo )
{
    if (! sp || sp->paths().size() != sp->shannonProjection().size ()) return 0;
    
    const sm_producer::ordered_outuple_t& ordered_output = sp->ordered_input_output(ooo);
    
    
    output.clear();
    auto cnt = ordered_output.size();
    
    for (auto ff = 0; ff < cnt; ff++)
    {
        const auto oo = ordered_output[ff];
        string_deque_t row;
        row.push_back(to_string(ff));
        row.push_back(to_string(std::get<1>(oo)));
        row.push_back(std::get<2>(oo).string());
        output.emplace_back(row);
    }
    
    return output.size();
}

int main(int ac, char* av[])
{
    std::string image_dir;
    std::string output_file;
    
    try {
        std::string inputPth, outputPth;
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
                output_file = vm["output"].as<std::string>();
                
                cout << vm["input"].as<std::string>() << std::endl;
                cout << vm["output"].as<std::string>() << std::endl;
                
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
        std::string delim (",");
        fs::path opath (output_file);
        auto papa = opath.parent_path ();
        if (fs::exists(papa) && ! runner.last_output.empty())
        {
            std::shared_ptr<std::ofstream> myfile = make_shared_ofstream(output_file);
            for (string_deque_t& row : runner.last_output)
            {
                auto cnt = 0;
                auto cols = row.size() - 1;
                for (std::string& col : row)
                {
                    *myfile << col;
                    if (cnt++ < cols)
                        *myfile << delim;
                }
                *myfile << std::endl;
            }
        }
        
    }
}



