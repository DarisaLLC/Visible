
#ifndef __STL_UTILS__
#define __STL_UTILS__

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

namespace stl_utils {
  
#if 0
    std::vector<std::string> getFilesFromDirectory(
                                                   const std::string strDir,
                                                   const std::string strBase,
                                                   const std::string strExt
                                                   )
    {
        std::vector<std::string> nameFiles;
        std::vector<boost::filesystem::path> pathEach;
        boost::filesystem::path pathDir( strDir.c_str() );
        copy( boost::filesystem::directory_iterator(pathDir), boost::filesystem::directory_iterator(), back_inserter(pathEach) );
        
        for( std::vector<boost::filesystem::path>::const_iterator it(pathEach.begin()), it_end(pathEach.end()); it != it_end; ++it )
        {
            if( boost::filesystem::is_regular_file( *it ) )
            {
                if( (*it).extension().string().find( strExt ) != std::string::npos )
                { // select by file extension
                    if( (*it).filename().string().find( strBase ) != std::string::npos )
                    { // select by file extension
                        nameFiles.push_back( (*it).filename().string() );
                    }
                }
            }
        }
        
        return nameFiles;
    }

#endif

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
   * Get teh set of keys in a map as a vector
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
}; // end of namespace stl_util




#endif

