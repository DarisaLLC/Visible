#ifndef __APP_UTILS__
#define __APP_UTILS__


#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/math/special_functions.hpp>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/thread/once.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string/classification.hpp> // Include boost::for is_any_of
#include <boost/algorithm/string/split.hpp> // Include for boost::split

#include <random>
#include <iterator>
#include "sshist.hpp"
#include <vector>
#include <list>
#include <queue>
#include <fstream>
#include <mutex>
#include <string>
#include <condition_variable>

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

using namespace std;
using namespace boost;


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


    
    /**
     @brief
     The visible_framework_core class is a singleton used to configure the program basics.
     @remark
     You should only create this singleton once because it destroys the identifiers!
     */
    class civf : public internal_singleton <civf>
    {
    public:
        
        
        
        civf() : _dev_run (false)
        {
            char buffer[1024];
            _epoch_time = boost::posix_time::ptime (boost::gregorian::date (1970, 1, 1));
            
            //   TimeStamp::frequency = ticks_per_second (); // using microssecond clock
            
            uint32_t path_len = 1023;
            if (_NSGetExecutablePath(buffer, &path_len))
                throw svl::runtime_error (" Could not retrieve executable path" );
            
            _executablePath =  std::string (buffer, strlen (buffer));
            std::cout << "civf_init: " << _executablePath << std::endl;
            
            
        }
        
        ~civf()
        {
            std::cout << "civf_end: " << _executablePath << std::endl;
        }
        
        //! Returns the path to the executable folder
        static const std::string& getExecutablePath()
        { return instance()._executablePath; }
        //! Returns the path to the log files
        
        static const std::string& getLogPath()
        { return instance()._logPath; }
        
        //! Return trrue for runs in the build directory (not installed)
        static bool isDevelopmentRun() { return instance()._dev_run; }
        
     //   static bool force_simd (bool v) { instance()._force_simd_if_present = v; return instance()._force_simd_if_present; }
        
        /**
         *   Absolute time
         */
        double get_absolute_time ()
        {
            boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::local_time ();
            return (current_time - _epoch_time).total_nanoseconds () * 1.0e-9;
        }
        
        boost::posix_time::ptime get_ptime ()
        {
            return boost::posix_time::microsec_clock::local_time ();
        }
        
        int64_t get_ticks ()
        {
            boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::local_time ();
            return (current_time - _epoch_time).ticks();
            
        }
        
        // Timestamps are ticks from epoch. Check to see if it is at least seconds after epoch
        bool check_distance_from_epoch (const int64_t& base, int64_t seconds_after)
        {
            return base > seconds_after * ticks_per_second();
        }
        
        int64_t ticks_per_second ()
        {
            return boost::date_time::micro_res::to_tick_count(0,0,1,0);
       }
        
        
    private:
        boost::posix_time::ptime _epoch_time;
        
        //! Path to the parent directory of the ones above if program was installed with relativ paths
        std::string _executablePath;        //!< Path to the executable
        std::string _logPath;               //!< Path to the log files folder
        bool                     _dev_run;               //!< True for runs in the build directory (not installed)
        
        civf(const civf&);
        civf& operator=( const civf& );
        
        int                           _debug_level_logfile;      //!< The debug level for the log file (belongs to OutputHandler)
        bool                          _force_simd_if_present;
        
    };
    

 
    // Need this useless class for explicitly instatiating template constructor
    template <typename T>
    struct tid {
        typedef T type;
    };

     
    namespace csv
    {
        typedef std::vector<float> rowf_t;
        typedef std::vector<rowf_t> matf_t;
        typedef std::vector<std::string> strings_t;
        typedef std::vector<strings_t> row_of_rows_t;
        
        // c++11
        static bool is_number(const std::string& s)
        {
            return !s.empty() && std::find_if(s.begin(),
                                              s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
        }
        
        //! Convert an input stream to csv rows.
        
        static csv::row_of_rows_t to_rows(std::ifstream &input)
        {
            if (!input.is_open())
                perror("error while opening file");
            
            csv::row_of_rows_t rows;
            std::string line;
            //for each line in the input stream
            while (std::getline(input, line))
            {
                boost::trim_if(line, boost::is_any_of("\t\r "));
                csv::strings_t row;
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
                row_of_rows_t crows;
                const strings_t& col = rows[0];
                for (int i = 0; i < columns; i++)
                {
                    strings_t row (1, col[i]);
                    crows.push_back (row);
                }
                
                rows = crows;
            }
            
            return rows;
        }
        
        // Returns starting row of numeric data as a positive number if it is a legacy visible file or
        // -1 * number of rows if it is not
        static int is_legacy_visible_output (csv::row_of_rows_t& rows)
        {
            if (rows.size () < 1) return false;
            const csv::strings_t& row = rows[0];
            bool is_visible = false;
            bool is_timestamp = false;
            bool is_millisecond = false;
            bool is_seconds = false;
            bool is_accel_data = false;
            int is_X(0), is_Y(0), is_Z(0);
            
            for (int i = 0; i < row.size(); i++)
            {
                std::cout << row[i] << std::endl;
                if (row[i].find ("Timestamp") != string::npos ) { is_timestamp = true; }
                if (row[i].find ("(ms)") != string::npos ) { is_millisecond = true; }
                if (row[i].find ("(g)") != string::npos ) { is_accel_data = true; }
                if (row[i].find ("X") != string::npos ) { is_X = true; }
                if (row[i].find ("Y") != string::npos ) { is_Y = true; }
                if (row[i].find ("Z") != string::npos ) { is_Z = true; }
                if (row[i].find ("Visible") != string::npos ) { is_visible = true; break; }
            }
            if (! is_visible )
            {
                
            }
            

            int last_row = -1;
            for (int rr = 1; rr < rows.size(); rr++)
            {
                const csv::strings_t& row = rows[rr];
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
            csv::row_of_rows_t rows = csv::to_rows (istream);
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
        
        
        static bool csv2vectors ( const std::string &csv_file, csv::row_of_rows_t& m_results, bool only_visible_format, bool columns_if_possible, bool verbose = false)
        {
            {
                m_results.resize (0);
                std::ifstream istream (csv_file);
                csv::row_of_rows_t rows = csv::to_rows (istream);
                int visible_row = is_legacy_visible_output (rows);
                int start_row = visible_row < 0 ? 0 : visible_row;
                if ( only_visible_format && visible_row < 0 ) return false;
                if (visible_row < 0 ) assert (start_row >= 0 && start_row < rows.size ());
                
                
                
                csv::matf_t datas;
                csv::matf_t column_wise;
                datas.resize (0);
                column_wise.resize (0);
                
                // Histogram column widths
                sshist  column_width (16);
                
                // Get All rows
                for (int rr = start_row; rr < rows.size(); rr++)
                {
                    const csv::strings_t& row = rows[rr];
                     const csv::rowf_t data;
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
                
                // if there were all same number of columns
                if (! shist.empty () && datas.size() && columns_if_possible)
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
    
    namespace dict
    {
        
        /*!
         * A templated dictionary class with a python-like interface.
         */
        template <typename Key, typename Val> class dict
        {
        public:
            /*!
             * Create a new empty dictionary.
             */
            dict(void);
            
            /*!
             * Input iterator constructor:
             * Makes boost::assign::map_list_of work.
             * \param first the begin iterator
             * \param last the end iterator
             */
            template <typename InputIterator>
            dict(InputIterator first, InputIterator last);
            
            /*!
             * Get the number of elements in this dict.
             * \return the number of elements
             */
            std::size_t size(void) const;
            
            /*!
             * Get a list of the keys in this dict.
             * Key order depends on insertion precedence.
             * \return vector of keys
             */
            std::vector<Key> keys(void) const;
            
            /*!
             * Get a list of the values in this dict.
             * Value order depends on insertion precedence.
             * \return vector of values
             */
            std::vector<Val> vals(void) const;
            
            /*!
             * Does the dictionary contain this key?
             * \param key the key to look for
             * \return true if found
             */
            bool has_key(const Key &key) const;
            
            /*!
             * Get a value in the dict or default.
             * \param key the key to look for
             * \param other use if key not found
             * \return the value or default
             */
            const Val &get(const Key &key, const Val &other) const;
            
            /*!
             * Get a value in the dict or throw.
             * \param key the key to look for
             * \return the value or default
             */
            const Val &get(const Key &key) const;
            
            /*!
             * Set a value in the dict at the key.
             * \param key the key to set at
             * \param val the value to set
             */
            void set(const Key &key, const Val &val);
            
            /*!
             * Get a value for the given key if it exists.
             * If the key is not found throw an error.
             * \param key the key to look for
             * \return the value at the key
             * \throw an exception when not found
             */
            const Val &operator[](const Key &key) const;
            
            /*!
             * Set a value for the given key, however, in reality
             * it really returns a reference which can be assigned to.
             * \param key the key to set to
             * \return a reference to the value
             */
            Val &operator[](const Key &key);
            
            /*!
             * Pop an item out of the dictionary.
             * \param key the item key
             * \return the value of the item
             * \throw an exception when not found
             */
            Val pop(const Key &key);
            
        private:
            typedef std::pair<Key, Val> pair_t;
            std::list<pair_t> _map; //private container
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



