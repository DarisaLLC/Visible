#ifndef _STL_UTILS_HPP
#define _STL_UTILS_HPP

#include <map>
#include <ostream>
#include <vector>
#include <assert.h>
#include <set>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <random>
#include <iomanip>
#include <fstream>
#include <stdio.h>
#include <thread>
#include <condition_variable>
#include <cstddef>
#include <tuple>
#include <utility>
#include <type_traits>
#include <iostream>


#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>


using namespace std;

namespace stl_utils
{
    /*
        Watch dog with RAII design
     */
    
    class watchdog {
    public:
        watchdog(int secs) {
            thread_ = std::thread{[=] {
                auto tp =
                std::chrono::high_resolution_clock::now() + std::chrono::seconds(secs);
                std::unique_lock<std::mutex> guard{mtx_};
                while (! canceled_
                       && cv_.wait_until(guard, tp) != std::cv_status::timeout) {
                    // spin
                }
                if (! canceled_) {
                    std::cerr << "WATCHDOG: unit test did not finish within "
                    << secs << "s, abort\n";
                    abort();
                }
            }};
        }
        ~watchdog() {
            { // lifetime scope of guard
                std::lock_guard<std::mutex> guard{mtx_};
                canceled_ = true;
                cv_.notify_all();
            }
            thread_.join();
        }

    private:
        volatile bool canceled_ = false;
        std::mutex mtx_;
        std::condition_variable cv_;
        std::thread thread_;
    };
    
            /**************
             *  Tuple     *
             * Accumulate *
             * Print      *
             *            *
             **************/
    
    // helper function to print a tuple of any size
    template<class Tuple, std::size_t N>
    struct TuplePrinter {
        static void print(const Tuple& t)
        {
            TuplePrinter<Tuple, N-1>::print(t);
            std::cout << ", " << std::get<N-1>(t);
        }
    };
    
    template<class Tuple>
    struct TuplePrinter<Tuple, 1> {
        static void print(const Tuple& t)
        {
            std::cout << std::get<0>(t);
        }
    };
    
    template<class... Args>
    void printTuple(const std::tuple<Args...>& t)
    {
        std::cout << "(";
        TuplePrinter<decltype(t), sizeof...(Args)>::print(t);
        std::cout << ")\n";
    }
    
    template <typename T1, typename T2> struct tuple_sum : public std::binary_function< std::tuple<T1,T1,T2>, std::tuple<T1,T1,T2>, std::tuple<T1,T1,T2> >
    {
        static_assert(std::is_arithmetic<T1>::value, "NumericType must be numeric");
        static_assert(std::is_arithmetic<T2>::value, "NumericType must be numeric");
        std::tuple<T1,T1,T2> operator()(const std::tuple<T1,T1,T2>& lhs, const std::tuple<T1,T1,T2>& rhs)
        {
            return std::tuple<T1,T1,T2>(std::get<0>(lhs) + std::get<0>(rhs), std::get<1>(lhs) + std::get<1>(rhs), std::get<2>(lhs) + std::get<2>(rhs));
        }
    };
    
    template <typename T1, typename T2> struct tuple_minmax : public std::binary_function< std::tuple<T1,T2>, std::tuple<T1,T2>, std::tuple<T1,T2> >
    {
        static_assert(std::is_arithmetic<T1>::value, "NumericType must be numeric");
        static_assert(std::is_arithmetic<T2>::value, "NumericType must be numeric");
        std::tuple<T1,T2> operator()(const std::tuple<T1,T2>& lhs, const std::tuple<T1,T2>& rhs)
        {
            return std::tuple<T1,T2>(std::min(std::get<0>(lhs), std::get<0>(rhs)), std::max(std::get<1>(lhs), std::get<1>(rhs)));
        }
    };

    // Accumulate over std::tuple  https://stackoverflow.com/a/18562596
    //    int main(int /*argc*/, const char* /*argv*/[])
    //    {
    //        auto val = tuple_accumulate(std::make_tuple(5, 3.2, 7U, 6.4f), 0L, functor());
    //        std::cout << val << std::endl; //should output 21.6
    //        return 0;
    //    }
    //deduces the type resulted from the folding of a sequence from left to right
    //avoids the decltype nonsense
    template <typename T, typename OpT>
    class result_of_acumulate;
    
    template <typename... ArgsT, typename OpT>
    class result_of_acumulate<std::tuple<ArgsT...>, OpT>
    {
    private:
        template <typename... ArgsHeadT>
        struct result_of_acumulate_helper;
        
        template <typename ArgsHeadT>
        struct result_of_acumulate_helper<ArgsHeadT>
        {
            typedef ArgsHeadT type;
        };
        
        template <typename ArgsHead1T, typename ArgsHead2T>
        struct result_of_acumulate_helper<ArgsHead1T, ArgsHead2T>
        {
            typedef typename std::result_of<OpT(ArgsHead1T, ArgsHead2T)>::type type;
        };
        
        template <typename ArgsHead1T, typename ArgsHead2T, typename... ArgsTailT>
        struct result_of_acumulate_helper<ArgsHead1T, ArgsHead2T, ArgsTailT...>
        {
            typedef typename result_of_acumulate_helper<typename std::result_of<OpT(ArgsHead1T, ArgsHead2T)>::type, ArgsTailT...>::type type;
        };
        
    public:
        typedef typename result_of_acumulate_helper<ArgsT...>::type type;
    };
    
    template <std::size_t IndexI, typename T, typename OutT, typename OpT>
    constexpr typename std::enable_if<(IndexI == std::tuple_size<T>::value), OutT>::type
    tuple_accumulate_helper(T const& /*tuple*/, OutT const& init, OpT /*op*/)
    {
        return init;
    }
    
    template <std::size_t IndexI, typename T, typename OutT, typename OpT>
    constexpr typename std::enable_if
    <
    (IndexI < std::tuple_size<T>::value),
    typename result_of_acumulate<T, OpT>::type
    >::type
    tuple_accumulate_helper(T const& tuple, OutT const init, OpT op)
    {
        return tuple_accumulate_helper<IndexI + 1>(tuple, op(init, std::get<IndexI>(tuple)), op);
    }
    
    template <typename T, typename OutT, typename OpT>
    auto tuple_accumulate(T const& tuple, OutT const& init, OpT op)
    -> decltype(tuple_accumulate_helper<0>(tuple, init, op))
    {
        return tuple_accumulate_helper<0>(tuple, init, op);
    }
    
    struct functor
    {
        template <typename T1, typename T2>
        auto operator()(T1 t1, T2 t2)
        -> decltype(t1 + t2)
        {
            return t1 + t2;
        }
    };
    
 
    /**************
     *  Map       *
     *            *
     **************/

    template <typename Key, typename Value, typename Comparator = less<Key>, typename Alloc = std::allocator<std::pair<const Key, Value> > >
    bool contains_key (const std::map<Key, Value, Comparator, Alloc>& dmap, const Key& key, Value& out)
    {
        typedef typename std::map<Key, Value, Comparator, Alloc>::const_iterator ci_t;
        ci_t it = dmap.find(key);
        if (it != dmap.end() )
        {
            out = it->second;
            return true;
        }
        return false;
    }
    
    template <class Key, class Value, typename Comparator, typename  Alloc = std::allocator<std::pair<const Key, Value> > >
    bool contains_value (const std::map<Key, Value, Comparator, Alloc>& dmap, const Value& in, Key& out)
    {
        typedef typename std::pair<Key, Value> pr_t;
        for (pr_t pr : dmap)
        {
            if (pr.second == in)
            {
                out = pr.first;
                return true;
            }
        }
        return false;
    }
    
    
    
    struct null_deleter
    {
        void operator() (void const *) const {}
    };
    
    template<typename C>
    struct internal_null_deleter_t
    {
        void operator() (const C *) const {}
    };
    
    
    template <typename PT>
    inline PT align(PT val, std::size_t alignment)
    {
        return val+(alignment - val%alignment)%alignment;
    }
    
    template<typename PT>
    struct iBuffer
    {
        typedef std::shared_ptr<PT> PTRef;
        
        PTRef _base;
        std::ptrdiff_t _diff;
        
        
        iBuffer (int capture_size, int alignment)
        {
            _base  = PTRef (new PT [capture_size] );
            unsigned char* tmp=(alignment>0) ? (unsigned char*)align((std::size_t) _base.get(),alignment) : _base.get();
            _diff = (std::size_t)(tmp) -  (std::size_t) _base.get();
            assert (_diff >= 0);
        }
        
        
    };
    
    
    template<class T>
    std::string t_to_string(T i)
    {
        std::stringstream ss;
        std::string s;
        ss << i;
        s = ss.str();
        
        return s;
    }
    
    
    // int r = signextend<signed int,5>(x);  // sign extend 5 bit number x to r
    template <typename T, unsigned B>
    inline T signextend(const T x)
    {
        struct {T x:B;} s;
        return s.x = x;
    }
    
    template <class T>
    bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
    {
        std::istringstream iss(s);
        return !(iss >> f >> t).fail ();
    }
    
    /**
     * RAIII (Resource Allocataion is Initialization) Exception safe handling of openning and closing of files.
     * a functor object to delete an ifstream
     * utility functions to create
     */
    struct ofStreamDeleter
    {
        void operator()(std::ofstream* p) const
        {
            if ( p != nullptr )
            {
                p->close();
                delete p;
            }
        }
    };
    
    struct ifStreamDeleter
    {
        void operator()(std::ifstream* p) const
        {
            if ( p != nullptr )
            {
                p->close();
                delete p;
            }
        }
    };
    
    
    class FileCloser
    {
    public:
        void operator()(FILE* file)
        {
            if (file != 0)
                fclose (file);
        }
    };
    
    
    // Typically T = char
    template<typename T>
    std::string stringifyFile(const std::string& file)
    {
        std::ifstream t(file);
        std::string str;
        t.seekg(0, std::ios::end);
        str.reserve(t.tellg());
        t.seekg(0, std::ios::beg);
        str.assign((std::istreambuf_iterator<T>(t)), std::istreambuf_iterator<T>());
        return str;
    }
    
    
    struct naturalComparator
    {
        bool operator () (const std::string& a, const std::string& b)
        {
            if (a.empty())
                return true;
            if (b.empty())
                return false;
            if (std::isdigit(a[0]) && !std::isdigit(b[0]))
                return true;
            if (!std::isdigit(a[0]) && std::isdigit(b[0]))
                return false;
            if (!std::isdigit(a[0]) && !std::isdigit(b[0]))
            {
                if (std::toupper(a[0]) == std::toupper(b[0]))
                    return naturalComparator () (a.substr(1), b.substr(1));
                return (std::toupper(a[0]) < std::toupper(b[0]));
            }
            
            // Both strings begin with digit --> parse both numbers
            std::istringstream issa(a);
            std::istringstream issb(b);
            int ia, ib;
            issa >> ia;
            issb >> ib;
            if (ia != ib)
                return ia < ib;
            
            // Numbers are the same --> remove numbers and recurse
            std::string anew, bnew;
            std::getline(issa, anew);
            std::getline(issb, bnew);
            return (naturalComparator () (anew, bnew));
        }
    };
    
    struct naturalPathComparator
    {
        bool operator () (const boost::filesystem::path& a_p, const boost::filesystem::path& b_p)
        {
            return naturalComparator () (a_p.filename ().string(), b_p.filename ().string() );
        }
    };
    
    
    template <class Val>
    void Out(const std::vector<Val>& v)
    {
        if (v.size() > 1)
            std::copy(v.begin(), v.end() - 1, std::ostream_iterator<Val>(std::cout, ", "));
        if (v.size())
            std::cout << v.back() << std::endl;
    }
    
    template <class Val>
    void Out(const std::deque<Val>& v)
    {
        if (v.size() > 1)
            std::copy(v.begin(), v.end() - 1, std::ostream_iterator<Val>(std::cout, ", "));
        if (v.size())
            std::cout << v.back() << std::endl;
    }
    
 

    /*
        Simple 1D filter interface and median filtering derivation
     
     */
    
    template <class T>
    class FilterInterface
    {
        
    public:
        virtual std::vector<T> filter(const std::vector<T> & in) = 0;
        
    private:

        
    };
    
    template <class T>
    class median1D : public FilterInterface<T>
    {
        std::vector<T> _history;
        std::vector<T> _pool;
        unsigned       _median;
        
    public:
        
        median1D(const unsigned long window_size)
        :
        _history(keep_odd(window_size), T()),
        _pool(_history),
        _median(window_size / 2 + 1)
        {
            assert(window_size >= 3);
        }
        
        
        std::vector<T> filter(const std::vector<T> & in)
        {
            assert(in.size() > 0);
            
            //---------------------------------------------------------------------
            // init state
            
            unsigned hist_ptr = 0;
            
            std::fill(_history.begin(), _history.end(), in[0]);
            
            //---------------------------------------------------------------------
            // filter input
            
            std::vector<T> out;
            out.reserve(in.size());
            
            for(auto x : in)
            {
                // step 1, write to history pool
                
                _history[hist_ptr] = x;
                
                hist_ptr = (hist_ptr + 1) % _history.size();
                
                // step 2, copy history to pool
                
                std::copy(_history.begin(), _history.end(), _pool.begin());
                
                // step 3, compute median on pool
                
                auto first = _pool.begin();
                auto last  = _pool.end();
                auto middle = first + (last - first) / 2;
                
                std::nth_element(first, middle, last);
                
                out.push_back(*middle);
            }
            
            return out;
        }
        
        unsigned keep_odd(unsigned n)
        {
            if(n % 2 == 0) return n + 1;
            
            return n;
        }
    };
    
    
    // Functions on sets
    //============================================================================
    
    /**
     * computes the union of two sets.
     */
    template <typename T>
    std::set<T> set_union(const std::set<T>& a, const std::set<T>& b) {
        std::set<T> output;
        std::set_union(a.begin(), a.end(),
                       b.begin(), b.end(),
                       std::inserter(output, output.begin()));
        return output;
    }
    
    template <typename T>
    std::set<T> set_union(const std::set<T>& a, const T& b) {
        std::set<T> output = a;
        output.insert(b);
        return output;
    }
    
    template <typename T>
    std::set<T> set_intersect(const std::set<T>& a, const std::set<T>& b) {
        std::set<T> output;
        std::set_intersection(a.begin(), a.end(),
                              b.begin(), b.end(),
                              std::inserter(output, output.begin()));
        return output;
    }
    
    template <typename T>
    std::set<T> set_difference(const std::set<T>& a, const std::set<T>& b) {
        std::set<T> output;
        std::set_difference(a.begin(), a.end(),
                            b.begin(), b.end(),
                            std::inserter(output, output.begin()));
        return output;
    }
    
    
    template <typename T>
    std::set<T> set_difference(const std::set<T>& a, const T& b) {
        std::set<T> output = a;
        output.erase(b);
        return output;
    }
    
    //! @return 2 sets: <s in partition, s not in partition>
    template <typename T>
    std::pair<std::set<T>,std::set<T> >
    set_partition(const std::set<T>& s, const std::set<T>& partition) {
        std::set<T> a, b;
        a = set_intersect(s, partition);
        b = set_difference(s, partition);
        return std::make_pair(a, b);
    }
    
    template <typename T>
    bool set_disjoint(const std::set<T>& a, const std::set<T>& b) {
        return (intersection_size(a,b) == 0);
    }
    
    template <typename T>
    bool set_equal(const std::set<T>& a, const std::set<T>& b) {
        if (a.size() != b.size()) return false;
        return a == b; // defined in <set>
    }
    
    template <typename T>
    bool includes(const std::set<T>& a, const std::set<T>& b) {
        return std::includes(a.begin(), a.end(), b.begin(), b.end());
    }
    
    /**
     * Returns true if $a \subseteq b$
     */
    template <typename T>
    bool is_subset(const std::set<T>& a, const std::set<T>& b) {
        return includes(b, a);
    }
    
    template <typename T>
    bool is_superset(const std::set<T>& a,const std::set<T>& b) {
        return includes(a, b);
    }
    
    //! Writes a human representation of the set to the supplied stream.
    template <typename T>
    std::ostream& operator<<(std::ostream& out, const std::set<T>& s) {
        return print_range(out, s, '{', ' ', '}');
    }
    
    // Functions on maps
    //============================================================================
    
    /**
     * constant lookup in a map. assertion failure of key not found in map
     */
    template <typename Key, typename T>
    const T& safe_get(const std::map<Key, T>& map,
                      const Key& key) {
        typedef typename std::map<Key, T>::const_iterator iterator;
        iterator iter = map.find(key);
        assert (iter != map.end());
        return iter->second;
    } // end of safe_get
    
    /**
     * constant lookup in a map. If key is not found in map,
     * 'default_value' is returned. Note that this can't return a reference
     * and must return a copy
     */
    template <typename Key, typename T>
    const T safe_get(const std::map<Key, T>& map,
                     const Key& key, const T default_value) {
        typedef typename std::map<Key, T>::const_iterator iterator;
        iterator iter = map.find(key);
        if (iter == map.end())   return default_value;
        else return iter->second;
    } // end of safe_get
    
    /**
     * Transform each key in the map using the key_map
     * transformation. The resulting map will have the form
     * output[key_map[i]] = map[i]
     */
    template <typename OldKey, typename NewKey, typename T>
    std::map<NewKey, T>
    rekey(const std::map<OldKey, T>& map,
          const std::map<OldKey, NewKey>& key_map) {
        std::map<NewKey, T> output;
        typedef std::pair<OldKey, T> pair_type;
        BOOST_FOREACH(const pair_type& pair, map) {
            output[safe_get(key_map, pair.first)] = pair.second;
        }
        return output;
    }
    
    /**
     * Transform each key in the map using the key_map
     * transformation. The resulting map will have the form
     output[i] = remap[map[i]]
     */
    template <typename Key, typename OldT, typename NewT>
    std::map<Key, NewT>
    remap(const std::map<Key, OldT>& map,
          const std::map<OldT, NewT>& val_map) {
        std::map<Key, NewT> output;
        typedef std::pair<Key, OldT> pair_type;
        BOOST_FOREACH(const pair_type& pair, map) {
            output[pair.first] = safe_get(val_map, pair.second);
        }
        return output;
    }
    
    /**
     * Inplace version of remap
     */
    template <typename Key, typename T>
    void remap(std::map<Key, T>& map,
               const std::map<T, T>& val_map) {
        typedef std::pair<Key, T> pair_type;
        BOOST_FOREACH(pair_type& pair, map) {
            pair.second = safe_get(val_map, pair.second);
        }
    }
    
    /**
     * Computes the union of two maps
     */
    template <typename Key, typename T>
    std::map<Key, T>
    map_union(const std::map<Key, T>& a,
              const std::map<Key, T>& b) {
        // Initialize the output map
        std::map<Key, T> output;
        std::set_union(a.begin(), a.end(),
                       b.begin(), b.end(),
                       std::inserter(output, output.begin()),
                       output.value_comp());
        return output;
    }
    
    /**
     * Computes the intersection of two maps
     */
    template <typename Key, typename T>
    std::map<Key, T>
    map_intersect(const std::map<Key, T>& a,
                  const std::map<Key, T>& b) {
        // Initialize the output map
        std::map<Key, T> output;
        // compute the intersection
        std::set_intersection(a.begin(), a.end(),
                              b.begin(), b.end(),
                              std::inserter(output, output.begin()),
                              output.value_comp());
        return output;
    }
    
    /**
     * Returns the entries of a map whose keys show up in the set keys
     */
    template <typename Key, typename T>
    std::map<Key, T>
    map_intersect(const std::map<Key, T>& m,
                  const std::set<Key>& keys) {
        std::map<Key, T> output;
        BOOST_FOREACH(const Key& key, keys) {
            typename std::map<Key,T>::const_iterator it = m.find(key);
            if (it != m.end())
                output[key] = it->second;
        }
        return output;
    }
    
    /**
     * Computes the difference between two maps
     */
    template <typename Key, typename T>
    std::map<Key, T>
    map_difference(const std::map<Key, T>& a,
                   const std::map<Key, T>& b) {
        // Initialize the output map
        std::map<Key, T> output;
        // compute the intersection
        std::set_difference(a.begin(), a.end(),
                            b.begin(), b.end(),
                            std::inserter(output, output.begin()),
                            output.value_comp());
        return output;
    }
    
    
    /**
     * Returns the set of keys in a map
     */
    template <typename Key, typename T>
    std::set<Key> keys(const std::map<Key, T>& map) {
        std::set<Key> output;
        typedef std::pair<Key, T> pair_type;
        BOOST_FOREACH(const pair_type& pair, map) {
            output.insert(pair.first);
        }
        return output;
    }
    
    /**
     * Get the set of keys in a map as a vector
     */
    template <typename Key, typename T>
    std::vector<Key> keys_as_vector(const std::map<Key, T>& map) {
        std::vector<Key> output(map.size());
        typedef std::pair<Key, T> pair_type;
        size_t i = 0;
        BOOST_FOREACH(const pair_type& pair, map) {
            output[i++] = pair.first;
        }
        return output;
    }
    
    
    /**
     * Gets the values from a map
     */
    template <typename Key, typename T>
    std::set<T> values(const std::map<Key, T>& map) {
        std::set<T> output;
        typedef std::pair<Key, T> pair_type;
        BOOST_FOREACH(const pair_type& pair, map) {
            output.insert(pair.second);
        }
        return output;
    }
    
    template <typename Key, typename T>
    std::vector<T> values(const std::map<Key, T>& m,
                          const std::set<Key>& keys) {
        std::vector<T> output;
        
        BOOST_FOREACH(const Key &i, keys) {
            output.push_back(safe_get(m, i));
        }
        return output;
    }
    
    template <typename Key, typename T>
    std::vector<T> values(const std::map<Key, T>& m,
                          const std::vector<Key>& keys) {
        std::vector<T> output;
        BOOST_FOREACH(const Key &i, keys) {
            output.push_back(safe_get(m, i));
        }
        return output;
    }
    
    //! Creates an identity map (a map from elements to themselves)
    template <typename Key>
    std::map<Key, Key> make_identity_map(const std::set<Key>& keys) {
        std::map<Key, Key> m;
        BOOST_FOREACH(const Key& key, keys)
        m[key] = key;
        return m;
    }
    
    //! Writes a map to the supplied stream.
    template <typename Key, typename T>
    std::ostream& operator<<(std::ostream& out, const std::map<Key, T>& m) {
        out << "{";
        for (typename std::map<Key, T>::const_iterator it = m.begin();
             it != m.end();) {
            out << it->first << "-->" << it->second;
            if (++it != m.end()) out << " ";
        }
        out << "}";
        return out;
    }
    
    /** Removes white space (space and tabs) from the beginning and end of str,
     returning the resultant string
     */
    inline std::string trim(const std::string& str) {
        std::string::size_type pos1 = str.find_first_not_of(" \t");
        std::string::size_type pos2 = str.find_last_not_of(" \t");
        return str.substr(pos1 == std::string::npos ? 0 : pos1,
                          pos2 == std::string::npos ? str.size()-1 : pos2-pos1+1);
    }
    
    /**
     * Convenience function for using std streams to convert anything to a string
     */
    template<typename T>
    std::string tostr(const T& t) {
        std::stringstream strm;
        strm << t;
        return strm.str();
    }
    
    /**
     * Convenience function for using std streams to convert a string to anything
     */
    template<typename T>
    T fromstr(const std::string& str) {
        std::stringstream strm(str);
        T elem;
        strm >> elem;
        assert(! strm.fail());
        return elem;
    }
    
    /**
     Returns a string representation of the number,
     padded to 'npad' characters using the pad_value character
     */
    inline std::string pad_number(const size_t number,
                                  const size_t npad,
                                  const char pad_value = '0') {
        std::stringstream strm;
        strm << std::setw((int)npad) << std::setfill(pad_value)
        << number;
        return strm.str();
    }
    
    
    // inline std::string change_suffix(const std::string& fname,
    //                                  const std::string& new_suffix) {
    //   size_t pos = fname.rfind('.');
    //   assert(pos != std::string::npos);
    //   const std::string new_base(fname.substr(0, pos));
    //   return new_base + new_suffix;
    // } // end of change_suffix
    
    
    /**
     Using splitchars as delimiters, splits the string into a vector of strings.
     if auto_trim is true, trim() is called on all the extracted strings
     before returning.
     */
    inline std::vector<std::string> strsplit(const std::string& str,
                                             const std::string& splitchars,
                                             const bool auto_trim = false) {
        std::vector<std::string> tokens;
        for(size_t beg = 0, end = 0; end != std::string::npos; beg = end+1) {
            end = str.find_first_of(splitchars, beg);
            if(auto_trim) {
                if(end - beg > 0) {
                    std::string tmp = trim(str.substr(beg, end - beg));
                    if(!tmp.empty()) tokens.push_back(tmp);
                }
            } else tokens.push_back(str.substr(beg, end - beg));
        }
        return tokens;
        // size_t pos = 0;
        // while(1) {
        //   size_t nextpos = s.find_first_of(splitchars, pos);
        //   if (nextpos != std::string::npos) {
        //     ret.push_back(s.substr(pos, nextpos - pos));
        //     pos = nextpos + 1;
        //   } else {
        //     ret.push_back(s.substr(pos));
        //     break;
        //   }
        // }
        // return ret;
    }
    
    
    template< class T >
    std::ostream & operator << ( std::ostream & os, const std::vector< T > & v ) {
        for ( const auto & i : v ) {
            os << i << std::endl;
        }
        return os;
    }
    
    
    
    
    template<class A,class B>
    inline ostream& operator<<(ostream& out, const multimap<A,B>& m)
    {
        out << "{{";
        for (typename multimap<A,B>::const_iterator it = m.begin();
             it != m.end();
             ++it) {
            if (it != m.begin()) out << ",";
            out << it->first << "=" << &it->second;
        }
        out << "}}";
        return out;
    }
    
}



#if 0
static std::string get_random_string (int length)
{
    static std::string chars(
                             "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890");
    
    std::random::random_device rng;
    boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
    std::vector<char> str_c;
    str_c.resize (length);
    for(int i = 0; i < length; ++i) str_c[i] = chars[index_dist(rng)];
    return std::string (&str_c[0]);
    
}
#endif

#endif
