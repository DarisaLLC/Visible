#ifndef __APP_UTILS__
#define __APP_UTILS__


#include <boost/algorithm/string.hpp>


#include <random>
#include <iterator>
#include "core/sshist.hpp"
#include <vector>
#include <list>
#include <queue>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <condition_variable>
#include <iterator>
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <boost/operators.hpp>
#include <ctime>

#include <sys/param.h>
#include <mach-o/dyld.h>
#include <boost/scoped_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "svl_exception.hpp"
#include <boost/lexical_cast.hpp>

#include <memory>
#include <functional>
#include <utility>

#include <boost/functional/factory.hpp>
#include "core/singleton.hpp"
#include "stl_utils.hpp"
#include "core/csv.hpp"
#include "core/core.hpp"


using namespace std;
using namespace boost;
using namespace svl;
using namespace stl_utils;

namespace fs=boost::filesystem;


namespace app_utils
{
    
    template<typename W,typename C,typename K = uint16_t>
    class WindowMgr : public SingletonLight<WindowMgr <W,C,K> >
    {
    public:
        typedef K key_t;
        typedef std::pair<W,C> WUPair_ref_t;
        
        
        WindowMgr ()
        {
            
        }
        
        bool makePair (const W& win, const C& ctx, key_t& out)
        {
            key_t wk, uk;
            C ur;
            
            if (! contains_value (mKtoWindow, win, wk) &&
                ! contains_value (mKtoContext, ctx, uk) )
            {
                // Get a random key
                key_t kk = svl::randomT<key_t> ();
                mKtoWindow[kk] = win;
                mKtoContext[kk] = ctx;
                out = kk;
                return true;
            }
            return false;
        }
        
        bool getPair (const key_t& key, WUPair_ref_t& pair)
        {
            return (contains_key (mKtoWindow, key, pair.first) && contains_key(mKtoContext, key, pair.second));
        }
        
        bool setFirst (const key_t& key, const W& win, bool replace = false)
        {
            C ctx;
            W tmp;
            
            if (contains_key(mKtoContext, key, ctx))
            {
                if (replace || (! replace && ! contains_key(mKtoWindow, key, tmp)))
                {
                    mKtoWindow[key] = win;
                    return true;
                }
            }
            return false;
        }
        
        bool setSecond (const key_t& key, const C& ctx, bool replace = false)
        {
            W tmp;
            C temp;
            
            if (contains_key(mKtoWindow, key, tmp))
            {
                if (replace || (! replace && ! contains_key(mKtoContext, key, temp)))
                {
                    mKtoContext[key] = ctx;
                    return true;
                }
            }
            return false;
        }
        
        
    private:
        std::map<key_t, W> mKtoWindow;
        std::map<key_t, C> mKtoContext;
        
        
    };
    
    
}
namespace file_system
{
    
    //! Returns the path separator for the host operating system's file system, \c '\' on Windows and \c '/' on Mac OS
#if defined( CINDER_MSW )
    static inline char getPathSeparator() { return '\\'; }
#else
    static inline char getPathSeparator() { return '/'; }
#endif
    
    
    template<typename T>
    static inline std::string toString( const T &t ) { return boost::lexical_cast<std::string>( t ); }
    template<typename T>
    static inline T fromString( const std::string &s ) { return boost::lexical_cast<T>( s ); }
    
    static std::string getPathDirectory( const std::string &path )
    {
        size_t lastSlash = path.rfind( getPathSeparator(), path.length() );
        if( lastSlash == string::npos ) {
            return "";
        }
        else
            return path.substr( 0, lastSlash + 1 );
    }
    
    static std::string getPathFileName( const std::string &path )
    {
        size_t lastSlash = path.rfind( getPathSeparator(), path.length() );
        if( lastSlash == string::npos )
            return path;
        else
            return path.substr( lastSlash + 1, string::npos );
    }
    
    static std::string getPathExtension( const std::string &path )
    {
        size_t i = path.rfind( '.', path.length() );
        size_t lastSlash = path.rfind( getPathSeparator(), path.length() );
        // make sure that we found a dot, and that the dot is after the last directory separator
        if( i != string::npos &&
           ( ( lastSlash == string::npos ) || ( i > lastSlash ) ) ) {
            return path.substr( i+1, path.length() - i );
        }
        else
            return std::string();
    }
    
    static bool ext_is (const std::string &path, const std::string& this_ext)
    {
        return this_ext == getPathExtension (path);
    }
    
    static bool ext_is_rfymov (const std::string &path)
    {
        static std::string rfymov = "rfymov";
        return ext_is (path, rfymov);
    }
    
    static bool ext_is_m4v (const std::string &path)
    {
        static std::string mov = "m4v";
        return ext_is (path, mov);
    }
    
    static bool ext_is_mov (const std::string &path)
    {
        static std::string mov = "mov";
        return ext_is (path, mov);
    }
    static bool ext_is_stk (const std::string &path)
    {
        static std::string stk = "stk";
        return ext_is (path, stk);
    }
    
    static bool file_exists (const std::string& path)
    {
        boost::filesystem::path p (path);
        return exists(p) && is_regular_file (p);
    }
    
    static std::string create_filespec (const std::string& path_to, const std::string& filename)
    {
        boost::filesystem::path p (path_to);
        p /= filename;
        return p.string();
    }
    
}


class imageDirectoryInfo
{
public:
    typedef std::pair<int,int> isize_t;
    typedef std::vector<fs::path > paths_vector_t;
    typedef std::map<isize_t, std::vector<fs::path>> paths_size_map_t;
    
    static std::shared_ptr<std::ostream> getNullOStream ()
    {
        static std::shared_ptr<std::ostream> null_ostream_ref (new std::ostream(nullptr));
        return null_ostream_ref;
    }
    
    
    enum sizeMappingOption : int { dontCare = 0,  centeredCommon = 1, reportFail = 2 };
    
    static bool is_anaonymous_name (const boost::filesystem::path& pp, size_t check_size = 36)
    {
        string extension = pp.extension().string();
        return extension.length() == 0 && pp.filename().string().length() == check_size;
    }
    
    static const std::vector<std::string>& supportedExtensions ()
    {
        static std::vector<std::string> exts = { ".jpg", ".png", ".JPG", ".jpeg"};
        return exts;
    }
    
    imageDirectoryInfo () {}
    imageDirectoryInfo (const std::string& imageDir, sizeMappingOption szmap,
                        const std::shared_ptr<std::ostream> &out = getNullOStream())
    : m_smo (szmap)
    {
        paths_vector_t tmp_framePaths;
        
        // make a list of all image files in the directory
        // in to the tmp vector
        filesystem::directory_iterator end_itr;
        for( filesystem::directory_iterator i( imageDir ); i != end_itr; ++i )
        {
            std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
            // skip if not a file
            if( !filesystem::is_regular_file( i->status() ) ) continue;
            
            if (out) *out << "Checking " << i->path().string();
            
            if (std::find( supportedExtensions().begin(),
                          supportedExtensions().end(), i->path().extension()  ) != supportedExtensions().end())
            {
                if (out)*out << "  Extension matches ";
                tmp_framePaths.push_back( i->path() );
            }
            else if (is_anaonymous_name(i->path()))
            {
                if (out) *out << "  Anonymous rule matches ";
                tmp_framePaths.push_back( i->path() );
            }
            else
            {
                if(out) *out << "Skipped " << i->path().string();
                continue;
            }
            if(out) *out << std::endl;
        }
        
        // get image size without loading the image using getImageSize
        for (boost::filesystem::path& pp : tmp_framePaths)
        {
            std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );
            auto isize = imageDirectoryInfo::getImageSize (pp);
            if (isize.first == 0 || isize.second == 0) continue;
            m_paths_size_map[isize].push_back (pp);
            m_size_map[isize] +=1;
        }
        
        if (m_size_map.size() > 0)
        {
            m_sizes = stl_utils::keys_as_vector(m_size_map);
        }
        // All same size, or our pairwise compare function does not care
        if (m_size_map.size() == 1 || (m_size_map.size() > 1 &&  szmap == sizeMappingOption::dontCare))
        {
            m_framePaths  = tmp_framePaths;
            mValid = true;
        }
        
        // All different size, expected to be the same, report by failing
        else if (m_size_map.size() > 1 &&  szmap == sizeMappingOption::reportFail)
            mValid = false;
        
        // All different size, take the most common only
        else if (m_size_map.size() > 1 &&  szmap == sizeMappingOption::centeredCommon)
        {
            using pair_type = decltype(m_size_map)::value_type;
            std::map<isize_t, int32_t>::const_iterator pr = std::max_element (
                                                                              std::begin(m_size_map), std::end(m_size_map),
                                                                              [] (const pair_type & p1, const pair_type & p2) {
                                                                                  return p1.second < p2.second;
                                                                              }
                                                                              );
            
            const auto mm = pr->first;
            if(out) cout << "Size map contains " << m_size_map.size() <<
                " smallest " << mm.first << "," << mm.second << std::endl;
        }
        else if (m_size_map.empty())
        {
            mValid = false;
        }
        else
        {
            if(out) *out << __FILE__ << " Unexpected error ";
        }
    }
    
    const paths_size_map_t filesBySizeMap () { return m_paths_size_map; }
    const bool sameSize () { return m_paths_size_map.size() == 1; }
    const paths_vector_t paths () const { return m_framePaths; }
    bool isValid () { return mValid; }
    
    
    
    
private:
    size_t m_filecount;
    isize_t m_smallest;
    std::map<isize_t, int32_t> m_size_map;
    paths_size_map_t m_paths_size_map;
    std::mutex m_mutex;
    paths_vector_t m_framePaths;
    bool mValid;
    sizeMappingOption m_smo;
    std::vector<isize_t> m_sizes;
    
    
    // Method to get image size from JPG, PNG and GIF files. Not the best way but avoids loading the entire file
    static isize_t getImageSize(const boost::filesystem::path& path)
    {
        isize_t res (0,0);
        std::shared_ptr<FILE> fRef (fopen(path.c_str(),"rb"), stl_utils::FileCloser());
        if (!fRef) return res;
        
        
        fseek(fRef.get(),0,SEEK_END);
        long len=ftell(fRef.get());
        fseek(fRef.get(),0,SEEK_SET);
        if (len<24) return res;
        
        // Strategy:
        // reading GIF dimensions requires the first 10 bytes of the file
        // reading PNG dimensions requires the first 24 bytes of the file
        // reading JPEG dimensions requires scanning through jpeg chunks
        // In all formats, the file is at least 24 bytes big, so we'll read that always
        unsigned char buf[24]; fread(buf,1,24,fRef.get());
        
        // For JPEGs, we need to read the first 12 bytes of each chunk.
        // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
        if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF && buf[3]==0xE0 && buf[6]=='J' && buf[7]=='F' && buf[8]=='I' && buf[9]=='F')
        { long pos=2;
            while (buf[2]==0xFF)
            { if (buf[3]==0xC0 || buf[3]==0xC1 || buf[3]==0xC2 || buf[3]==0xC3 || buf[3]==0xC9 || buf[3]==0xCA || buf[3]==0xCB) break;
                pos += 2+(buf[4]<<8)+buf[5];
                if (pos+12>len) break;
                fseek(fRef.get(),pos,SEEK_SET); fread(buf+2,1,12,fRef.get());
            }
        }
        
        // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
        if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF)
        { res.second = (buf[7]<<8) + buf[8];
            res.first = (buf[9]<<8) + buf[10];
            return res;
        }
        
        // GIF: first three bytes say "GIF", next three give version number. Then dimensions
        if (buf[0]=='G' && buf[1]=='I' && buf[2]=='F')
        { res.first = buf[6] + (buf[7]<<8);
            res.second = buf[8] + (buf[9]<<8);
            return res;
        }
        
        // PNG: the first frame is by definition an IHDR frame, which gives dimensions
        if ( buf[0]==0x89 && buf[1]=='P' && buf[2]=='N' && buf[3]=='G' && buf[4]==0x0D && buf[5]==0x0A && buf[6]==0x1A && buf[7]==0x0A
            && buf[12]=='I' && buf[13]=='H' && buf[14]=='D' && buf[15]=='R')
        { res.first = (buf[16]<<24) + (buf[17]<<16) + (buf[18]<<8) + (buf[19]<<0);
            res.second = (buf[20]<<24) + (buf[21]<<16) + (buf[22]<<8) + (buf[23]<<0);
            return res;
        }
        
        return res;
    }
    
    
    
};


namespace gen_filename
{
    
    static bool insensitive_case_compare (const std::string& str1, const std::string& str2)
    {
        for(unsigned int i=0; i<str1.length(); i++){
            if(toupper(str1[i]) != toupper(str2[i]))
                return false;
        }
        return true;
    }
    
    class random_name
    {
        
        std::string _chars;
        std::mt19937 mBase;
    public:
        
        random_name ()
        {
            _chars = std::string (
                                  "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "1234567890");
        }
        
        int32_t nextInt( int32_t v )
        {
            if( v <= 0 ) return 0;
            return mBase() % v;
        }
        
        std::string get_anew ()
        {
            std::string ns;
            for(int i = 0; i < 8; ++i) ns.push_back (_chars[nextInt(_chars.size()-1)]);
            assert (ns.length() == 8);
            return ns;
        }
    };
    
}


namespace thread_safe
{
    template<typename Data>
    class concurrent_queue
    {
    private:
        std::queue<Data> the_queue;
        mutable std::mutex the_mutex;
        std::condition_variable the_condition_variable;
    public:
        
        concurrent_queue () {}
        
        void push(Data const& data)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_queue.push(std::move(data));
            the_condition_variable.notify_one();
        }
        
        bool empty() const
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            return the_queue.empty();
        }
        
        bool try_pop(Data& popped_value)
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            if(the_queue.empty())
            {
                //     std::cout << "poping on empty " << std::endl;
                return false;
            }
            
            popped_value=std::move (the_queue.front());
            the_queue.pop();
            //static_cast<int32>(status)    std::cout << "poped " << std::endl;
            return true;
        }
        
        void wait_and_pop(Data& popped_value)
        {
            std::unique_lock<std::mutex> lock(the_mutex);
            the_condition_variable.wait (lock, [this] { return ! empty(); } );
            std::cout << "poped " << std::endl;
            popped_value=std::move (the_queue.front());
            the_queue.pop();
        }
        
    };
    
    struct cancelled_error {};
    
    class cancellation_point
    {
    public:
        cancellation_point(): stop_(false) {}
        
        void cancel() {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
            cond_.notify_all();
        }
        
        template <typename P>
        void wait(const P& period) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stop_ || cond_.wait_for(lock, period) == std::cv_status::no_timeout) {
                stop_ = false;
                throw cancelled_error();
            }
        }
    private:
        bool stop_;
        std::mutex mutex_;
        std::condition_variable cond_;
    };
    
}

namespace csv
{
    typedef std::vector<float> rowf_t;
    typedef std::vector<rowf_t> matf_t;
    typedef std::vector<std::string> row_type;
    typedef std::vector<row_type> rows_type;
    
    // c++11
    static bool is_number(const std::string& s)
    {
        return !s.empty() && std::find_if(s.begin(),
                                          s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
    }
    
    //! Convert an input stream to csv rows.
    
    static rows_type to_rows(std::ifstream &input)
    {
        if (!input.is_open())
            perror("error while opening file");
        
        csv::rows_type rows;
        std::string line;
        //for each line in the input stream
        while (std::getline(input, line))
        {
            boost::trim_if(line, boost::is_any_of("\t\r "));
            csv::row_type row;
            boost::split(row, line, boost::is_any_of("\t\r ,"));
            
            rows.push_back(row);
        }
        if (input.bad())
            perror("error while reading file");
        
        // Handle cases where file conained one long delimated sequence of numbers
        // which were parsed in to 1 row and many columns
        if ( ! rows.empty () && (rows[0].size() / rows.size() ) == rows[0].size() )
        {
            int columns = rows[0].size ();
            assert (rows.size () == 1);
            rows_type crows;
            const row_type& col = rows[0];
            for (int i = 0; i < columns; i++)
            {
                row_type row (1, col[i]);
                crows.push_back (row);
            }
            
            rows = crows;
        }
        
        return rows;
    }
    
    // Returns starting row of numeric data as a positive number if it is a legacy visible file or
    // -1 * number of rows if it is not
    static int is_legacy_visible_output (csv::rows_type& rows)
    {
        if (rows.size () < 1) return false;
        const csv::row_type& row = rows[0];
        bool is_visible = false;
        bool is_timestamp = false;
        bool is_millisecond = false;
        bool is_seconds = false;
        bool is_accel_data = false;
        int is_X(0), is_Y(0), is_Z(0);
        
        for (int i = 0; i < row.size(); i++)
        {
            if (row[i].find ("Timestamp") != string::npos ) { is_timestamp = true; }
            if (row[i].find ("(ms)") != string::npos ) { is_millisecond = true; }
            if (row[i].find ("(g)") != string::npos ) { is_accel_data = true; }
            if (row[i].find ("X") != string::npos ) { is_X++; }
            if (row[i].find ("Y") != string::npos ) { is_Y++; }
            if (row[i].find ("Z") != string::npos ) { is_Z++; }
            if (row[i].find ("Visible") != string::npos ) { is_visible = true; break; }
        }
        if (! is_visible )
        {
            return (is_timestamp) ? 1 : -1;
        }
        int last_row = -1;
        for (int rr = 1; rr < rows.size(); rr++)
        {
            const csv::row_type& row = rows[rr];
            for (int i = 0; i < row.size(); i++)
            {
                if (row[i].find ("seconds") != string::npos )
                {
                    last_row = rr + ( rr < (rows.size() - 1) ? 1 : 0) ; break;
                }
            }
            if (last_row > 0) break;
        }
        return last_row;
    }
    
    // Returns starting row of numeric data as a positive number if it is a legacy visible file or
    // -1 * number of rows if it is not
    static int file_is_legacy_visible_output (std::string& fqfn)
    {
        std::ifstream istream (fqfn);
        csv::rows_type rows = csv::to_rows (istream);
        return is_legacy_visible_output ( rows);
        
    }
    
    static bool validate_matf (matf_t tm)
    {
        if (tm.empty()) return false;
        size_t d = tm.size();
        if (d <= 0 || d > 10000 ) return false; // arbitrary upper limit. I know !!
        for (int rr=0; rr < d; rr++)
            if (tm[rr].size() != d) return false;
        return true;
        
    }
    
    
    static bool csv2vectors ( const std::string &csv_file, matf_t& m_results, bool only_visible_format, bool columns_if_possible, bool verbose = false)
    {
        {
            m_results.resize (0);
            std::ifstream istream (csv_file);
            csv::rows_type rows = csv::to_rows (istream);
            int visible_row = is_legacy_visible_output (rows);
            int start_row = visible_row < 0 ? 0 : visible_row;
            if ( only_visible_format && visible_row < 0 ) return false;
            if (visible_row < 0 ) assert (start_row >= 0 && start_row < rows.size ());
            
            
            
            matf_t datas;
            matf_t column_wise;
            datas.resize (0);
            column_wise.resize (0);
            
            sshist  column_width (16);
            
            // Get All rows
            for (int rr = start_row; rr < rows.size(); rr++)
            {
                const csv::row_type& row = rows[rr];
                vector<float> data;
                for (int t = 0; t < row.size(); t++)
                {
                    char *end_ptr;
                    float f = strtof(row[t].c_str(), &end_ptr);
                    if (end_ptr == row[t].c_str()) continue;
                    data.push_back (f);
                }
                datas.push_back(data);
                column_width.add (data.size());
            }
            
            sshist::sorted_container_t shist;
            column_width.get_value_sorted (shist);
            if (verbose)
            {
                for (auto pt : shist) std::cout << "[" << pt.first << "]: " << pt.second << std::endl;
            }
            
            if (! shist.empty () && datas.size() && columns_if_possible) // if there were all same number of columns
            {
                uint32_t cw = shist.front().first;
                column_wise.resize(cw);
                for (uint i = 0; i < datas.size (); i++)
                {
                    const vector<float>& vc = datas[i];
                    if (vc.size () != cw) continue;
                    
                    for (uint cc=0; cc<cw;cc++)
                        column_wise[cc].push_back(vc[cc]);
                }
            }
            
            if (!column_wise.empty ())
            {
                assert (columns_if_possible);
                m_results = column_wise;
            }
            else if ( ! datas.empty () )
            {
                m_results = datas;
            }
            
            return ! m_results.empty ();
        }
        
    }
}


#if 0
namespace factory
{
    
    
    struct manufacturable {
        virtual ~manufacturable() {};
        virtual void api_method() const = 0;
    };
    using manufacturable_ptr = std::shared_ptr<struct manufacturable>;
    
    struct factory {
        virtual ~factory() {}
        virtual manufacturable_ptr create() const = 0;
    };
    using factory_ptr = std::shared_ptr<struct factory>;
    
    template <class T, class... Args>
    struct factory_tpl final: factory
    {
        std::function<std::shared_ptr<T>()> impl_;
        factory_tpl(Args&&... args):
        impl_(std::bind(boost::factory<std::shared_ptr<T>>(), std::forward<Args>(args)...))
        {}
        
        virtual manufacturable_ptr create() const override { return std::dynamic_pointer_cast<struct manufacturable>(impl_()); }
    };
    
    template <class T, class... Args>
    factory_ptr make_factory(Args&&... args)
    {
        return std::dynamic_pointer_cast<struct factory>(std::make_shared< factory_tpl<T, Args...> >(std::forward<Args>(args)...));
    }
    
    struct concrete_manufacturable final: manufacturable
    {
        std::string const domain_;
        concrete_manufacturable(std::string const& id): domain_(domain_) {}
        virtual void api_method() const override {}
    };
    
    static int test_main()
    {
        factory_ptr factory = make_factory<concrete_manufacturable>("demo");
        manufacturable_ptr object = factory->create();
        object->api_method();
        return 0;
    }
}
#endif

#endif

// @todo fix this #include "dict.ipp"



