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


static std::shared_ptr<uint16_t> create_ramp16(int asp, int pct = 100)
{
    static int full_pct(100);
    int count = 65536; // 256 * 256
    int cliff = (count * pct) / full_pct;
    int w = 256;
    int h = 256;
    int index = 0;
    std::shared_ptr<uint16_t> image = create<uint16_t>(w / asp, h * asp);
    uint16_t * pptr = image.get();

    for (; index < count; index++)
    {
        pptr[index] = (index < cliff) ? index : cliff;
    }

    return image;
}




static std::shared_ptr<uint16_t> create_easy_ramp16(int w, int h)
{
    int count = w * h;
    int index = 0;
    std::shared_ptr<uint16_t> image = create<uint16_t>(w, h);
    uint16_t * pptr = image.get();

    for (; index < count; index++)
    {
        pptr[index] = (uint16_t)std::sqrt(index);
    }

    return image;
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
    
class genv : public testing::Environment
{
public:
    typedef boost::filesystem::path path_t;
    typedef std::vector<path_t> path_vec; // store paths

    
    
    genv(const std::string & exec_path) : m_exec_path (exec_path)
    {
        m_exec_dir = executable_path().parent_path();
        add_content_directory(m_exec_dir);
    }
    
    void setUpFromArgs (int argc, char** argv,  std::string input_marker = "--assets")
    {
        boost::filesystem::path installpath = std::string(argv[0]);
        
        if (argc == 3)
        {
            std::string current_exec_name = argv[0]; // Name of the current exec program
            std::vector<std::string> all_args;
            all_args.assign(argv + 1, argv + argc);
            boost::filesystem::path input;
            bool got_input;
            got_input = false;
            
            
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
            
            if (!boost::filesystem::exists(input))
            {
                std::cerr << input.string() << " does not exist. Use --help\n";
            }
            
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
        std::vector<boost::filesystem::path> nameFiles = getFilesFromDirectory(file_name);
        if (nameFiles.size() == 1)
            return std::make_pair(nameFiles[0], true);
        
        return std::make_pair(path_t (), false);
    }

    const path_t & executable_path() { return m_exec_path; }
    const path_t & executable_folder() { return m_exec_dir; }


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
                nameFiles.push_back(p);
            }
        }
        
        return nameFiles;
    }
    
    mutable std::vector<path_t> m_content_paths;
    mutable path_t m_exec_path;
    mutable path_t m_exec_dir;
};
}

#endif
