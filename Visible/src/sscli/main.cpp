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
#include <iomanip>
#include "core/stl_utils.hpp"
#include <string>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>
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
                             sm_producer::outputOrderOption ooo = sm_producer::outputOrderOption::input);
bool movieOutput (  std::shared_ptr<sm_producer>& sp,   string_deque_array_t&  output);


struct cb_similarity_producer
{
    cb_similarity_producer (const std::string& content, bool auto_run = false)
    {
        m_content_path = fs::path(content);
        assert(fs::exists(m_content_path));
        is_movie = fs::is_regular_file(m_content_path);
        is_dir = fs::is_directory(m_content_path);
        assert(is_movie != is_dir);
        m_auto = auto_run;
        last_output.resize(0);
        content_loaded = false;
    }
    
    
    int run ()
    {
        try
        {
            sp =  std::shared_ptr<sm_producer> ( new sm_producer () );
            std::function<void ()> content_loaded_cb = std::bind (&cb_similarity_producer::signal_content_loaded, this);
            boost::signals2::connection ml_connection = sp->registerCallback(content_loaded_cb);
            if (is_dir)
                sp->load_image_directory(m_content_path.string(),  sm_producer::sizeMappingOption::mostCommon);
            if (is_movie)
                sp->load_content_file(m_content_path.string());
            auto resp = sp->operator()(0);
            std::cout << std::boolalpha << resp << std::endl;
            if (is_dir){
                auto resc = imageDirectoryOutput ( sp,  last_output);
                std::cout << " Image Directory Output " << resc << std::endl;
            }
            if (is_movie){
                std::cout << " Content Movie " << std::endl;
                movieOutput(sp, last_output);
            }
            
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
        content_loaded = true;
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
    fs::path m_content_path;
    string_deque_array_t  last_output;
    bool is_movie;
    bool is_dir;
    bool content_loaded;
    int frameCount () { if (sp) return sp->frames_in_content (); return -1; }
    
   };



std::shared_ptr<std::ifstream> make_shared_ifstream(std::ifstream * ifstream_ptr)
{
    return std::shared_ptr<std::ifstream>(ifstream_ptr, ifStreamDeleter());
}

std::shared_ptr<std::ifstream> make_shared_ifstream(std::string filename)
{
    return make_shared_ifstream(new std::ifstream(filename, std::ifstream::in));
}

bool sortbysec(const std::pair<int,int>& a,
               const std::pair<int,int>& b)
{
    return a.second < b.second;
}
  
size_t imageDirectoryOutput (  std::shared_ptr<sm_producer>& sp,   string_deque_array_t&  output,
                             sm_producer::outputOrderOption ooo )
{
    if (! sp || sp->paths().size() != sp->shannonProjection().size ()) return 0;
    auto entropy = sp->shannonProjection();
    for (auto en : entropy) std::cout << fixed << showpoint << std::setprecision(16) << en << std::endl;
    std::cout << std::endl;
    
    const sm_producer::ordered_outuple_t& ordered_output = sp->ordered_input_output(ooo);
    std::vector<std::pair<int,int>> order(ordered_output.size());
    std::string::size_type sz;
    for (auto cc = 0; cc < order.size(); cc++){
        auto stem = get<2>(ordered_output[cc]).stem().string().substr(5,4);
        order[cc].first = cc;
        order[cc].second = std::stoi(stem, &sz);
    }
    auto seq = order;
    sort(order.begin(), order.end(), sortbysec);
    for (auto cc : order) std::cout << cc.first << " " << cc.second << std::endl;

    
    
    
    
    output.clear();
    auto cnt = ordered_output.size();
    
    for (auto ff = 0; ff < cnt; ff++)
    {
        strstream msg;
        const auto oo = ordered_output[order[ff].second];
        string_deque_t row;
        row.push_back(to_string(ff));
        msg << fixed << showpoint << std::setprecision(16) << std::get<1>(oo);
        row.push_back(msg.str());
        row.push_back(std::get<2>(oo).string());
        output.emplace_back(row);
    }
    
    return output.size();
}

bool movieOutput (  std::shared_ptr<sm_producer>& sp,   string_deque_array_t&  output)
{
    if (! sp || sp->paths().size() != sp->shannonProjection().size ()) return false;
    auto entropy = sp->shannonProjection();
    for (auto en : entropy) std::cout << fixed << showpoint << std::setprecision(16) << en << std::endl;
    std::cout << std::endl;
    
    output.clear();
    auto cnt = entropy.size();
    
    for (auto ff = 0; ff < cnt; ff++)
    {
        string_deque_t row;
        strstream msg;
        msg << fixed << showpoint << std::setprecision(16) << entropy[ff];
        row.push_back(msg.str());
        output.emplace_back(row);
    }
    return true;
}

int main(int ac, char* av[])
{
    std::string input_content;
    std::string output_file;
    
    try {
        po::options_description desc("Options");
        desc.add_options()
        ("help,h", "Displays this message")
        ("input,i", po::value<std::string>(&input_content)->required(), "Path to image directory")
        ("output,o", po::value<std::string>(&output_file)->required(), "Path to outputfile ")
        ;
        
        po::variables_map vm;
        try {
            po::store(po::parse_command_line(ac, av, desc),vm);
            if (!vm.count("input")) {
                cout << "Usage: sscli input output [options]" << endl;
            }
            else
            {
                input_content = vm["input"].as<std::string>();
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
    
    if(!input_content.empty())
    {
        cb_similarity_producer runner (input_content);
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
                    std::cout << col << std::endl;
                    *myfile << col;
                    if (cnt++ < cols)
                        *myfile << delim;
                }
                *myfile << std::endl;
            }
        }
        
    }
}



