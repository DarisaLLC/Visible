#ifndef _utils_h
#define _utils_h

// utils.h  Utility functions (many are OS specific)


#include <vector>
#include <string>

#include "types.h"
using namespace std;


class Utils
{
 public:

  // Return all files in a directory. Sub-directories are also returned
  static Strings get_file_list (const std::string& dir, Strings& files);

  // Return true if the file exists
  static bool file_exists (const std::string& path);
  
  // Move file via renaming
  static bool movefile (const std::string& src, const std::string& dst);

  // Remove a file
  static bool removefile (const std::string& file);

  // Remove all files in a directory
  static bool remove_files (const std::string& dir);

  // Make Directory
  static bool create_directory (const std::string& dir);

  static std::string read_file_as_string (const std::string& file);
  static bool write_string_to_file (const std::string& file, const std::string& contents);

  // Convert an integer to a string
  static std::string to_string(int i);
  static std::string double_to_string(double d);

  // Split a string
  static Strings split (const std::string& str, const std::string& sep);

  static void getFilesInDirectory(const string& dirName, vector<string>& fileNames, const vector<string>& validExtensions);
    
};


#endif // _utils_h
