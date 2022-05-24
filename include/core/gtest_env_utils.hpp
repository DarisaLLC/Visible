#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <memory>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/foreach.hpp>

namespace bfs = boost::filesystem;

namespace test_utils
{


// Create a buffer T sized width * height, i.e. stride equals width
template <typename T>
static std::shared_ptr<T> create(int width, int height)
{
    return std::shared_ptr<T>(new T[width * height]);
}

// Create a buffer T sized width * height, i.e. stride equals width
template <typename T>
static std::shared_ptr<std::vector<T>> create_array(int width, int height)
{
    std::shared_ptr<std::vector<T>> rows(height);
    for (int i = i; i < height; i++)
        rows[i].resize(width);
    return rows;
}


template <typename T>
bool is_raw_same(const std::shared_ptr<T> & left, const std::shared_ptr<T> & right, unsigned total_ts)
{
    const T * lptr = left.get();
    const T * rptr = right.get();

    for (unsigned uu = 0; uu < total_ts; uu++)
        if (lptr[uu] != rptr[uu])
            return false;
    return true;
}


template <typename T>
bool has_correct_adjacent_backward_diff(const std::shared_ptr<T> & left, unsigned total_ts, int diff = 1)
{
    const T * lptr = left.get();

    for (unsigned uu = 0; uu < (total_ts - 1 - diff); uu++)
    {
        int md = lptr[uu + diff] - lptr[uu];
        if (md != diff) return false;
    }

    return true;
}


    
static std::shared_ptr<uint8_t> create_trig(int w, int h)
{
    int count = w * h;
    int index = 0;
    std::shared_ptr<uint8_t> image = create<uint8_t>(w, h);
    uint8_t * pptr = image.get();

    for (; index < count; index++)
    {
        float row = (index / w) / (float)h;
        float col = (index % w) / (float)w;
        float r2 = (row*row + col*col) / 2.0f;
        float val = (std::sin(r2) + 1.0f) / 2.0f;
        pptr[index] = (uint8_t)(val * 255);
    }

    return image;
}


namespace fs=boost::filesystem;

struct null_deleter
{
    void operator()(void const *) const
    {
    }
};

struct recursive_directory_range
{
    typedef fs::recursive_directory_iterator iterator;
    recursive_directory_range(fs::path p) : p_(p) {}
    
    iterator begin() { return fs::recursive_directory_iterator(p_,boost::filesystem::symlink_option::recurse); }
    iterator end() { return fs::recursive_directory_iterator(); }
    
    fs::path p_;
};

    
class genv : public testing::Environment
{
public:
    typedef boost::filesystem::path path_t;
    typedef std::vector<path_t> path_vec; // store paths

    
    
    genv(const std::string & exec_path) : m_exec_path (exec_path)
    {
        m_exec_dir = executable_path().parent_path();
        add_content_directory(m_exec_dir);
        // Create a directory for output if it does not exists
        m_output_path = m_exec_dir / "output";
        boost::system::error_code ec;
        if(!bfs::exists(m_output_path)){
            bfs::create_directory(m_output_path, ec);
        }
    
    }
    
    bool executable_path_exists () const{
        return boost::filesystem::exists(m_exec_dir);
    }
    void setUpFromArgs (int argc, char** argv,  std::string input_marker = "--assets")
    {
        boost::filesystem::path installpath = std::string(argv[0]);
        
        std::string current_exec_name = argv[0]; // Name of the current exec program
        std::vector<std::string> all_args;
        all_args.assign(argv + 1, argv + argc);

        // Get all assets marker indexes in all args
        std::vector<int> mindex;
        for (auto argcc = 0; argcc < all_args.size(); argcc++)
        {
            std::size_t found = all_args.at(argcc).find(input_marker);
            if (found != std::string::npos) mindex.push_back(argcc);
        }
        for (auto mm : mindex){
        
            boost::filesystem::path input = all_args[mm+1];
            if (boost::filesystem::exists(input))
                add_content_directory(input);
        }
    }
    
    
    const path_vec & content_dirs() { return m_content_paths; }
    void add_content_directory(const path_t & content_dir)
    {
        m_content_paths.push_back(content_dir);
    }
    void add_content_directory(path_t & content_dir)
    {
        m_content_paths.push_back(content_dir);
    }
 
    std::pair<path_t, bool> asset_path(const std::string& file_name)
    {
    //    if (m_content_paths.size() != 2)
     //        return std::make_pair(path_t (), false);
        
        for (auto& content : m_content_paths){
            
            // In the future this can be a simple cache
            for (auto it : recursive_directory_range(content))
            {
                auto diff = it.path().filename ().compare (file_name);
                if (diff == 0)
                {
                   return std::make_pair(it.path(), true);
                }
            }
        }
    
        return std::make_pair(path_t (), false);
    }

    const path_t & executable_path() { return m_exec_path; }
    const path_t & executable_folder() { return m_exec_dir; }
    const path_t & output_path() { return m_output_path; }
    


 //   void TearDown() {}

private:
    std::vector<boost::filesystem::path> getFilesFromDirectory(const std::string strBase)
    {
        std::vector<boost::filesystem::path> nameFiles;
        
        for( std::vector<boost::filesystem::path>::const_iterator it(m_content_paths.begin()),
            it_end(m_content_paths.end()); it != it_end; ++it )
        {
            BOOST_FOREACH(
                          const boost::filesystem::path& p,
                          std::make_pair(
                                         boost::filesystem::directory_iterator(*it),
                                         boost::filesystem::directory_iterator()
                                         )
                          )
            if( p.filename().string().find( strBase ) != std::string::npos )
            {
                // select by file extension
                std::cout << "Asset File: " << p.string() << std::endl;
                nameFiles.push_back(p);
            }
        }
        
        return nameFiles;
    }
    
    mutable std::vector<path_t> m_content_paths;
    mutable path_t m_exec_path;
    mutable path_t m_exec_dir;
    mutable path_t m_output_path;
};
}

#endif
