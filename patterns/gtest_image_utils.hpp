#ifndef __UTILS_HPP__
#define __UTILS_HPP__

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

class genv : public testing::Environment
{
public:
    typedef boost::filesystem::path path_t;
    typedef std::vector<path_t> path_vec; // store paths

    genv(const std::string & exec_path)
        : m_exec_path(exec_path)
    {
        m_exec_dir = executable_path().parent_path();
        add_content_directory((m_exec_dir.parent_path()).parent_path());
    }


    const path_vec & content_dirs() { return m_content_paths; }
    void add_content_directory(const path_t & content_dir)
    {
        m_content_paths.push_back(content_dir);
    }

 
    std::pair<path_t, bool> asset_path(const std::string& file_name)
    {
        for (auto content = content_dirs().begin(); content != content_dirs().end(); content++)
        {
            for(fs::directory_iterator it(*content); it != fs::directory_iterator() ; ++it)
                {
                    const fs::recursive_directory_iterator end;
                    const auto fit = find_if(fs::recursive_directory_iterator(*content), end,
                                             [&file_name](const fs::directory_entry& e) {
                                                return e.path().filename() == file_name;
                                            });
                    if (fit != end)
                       return std::make_pair(fit->path(), true);

                }
        }
        return std::make_pair(path_t(), false);
    }
    

    const path_t & executable_path() { return m_exec_path; }
    const path_t & executable_folder() { return m_exec_dir; }


    void TearDown() {}

private:
    std::vector<path_t> m_content_paths;
    path_t m_exec_path;
    path_t m_exec_dir;
};
}

#endif
