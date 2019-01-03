#ifndef __FILE_SYSTEM_UTILS__
#define __FILE_SYSTEM_UTILS__

#include <string>
#include <vector>

#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace bf = boost::filesystem;

namespace svl {
    namespace io {
        
        
        /** checks if a file exists
         * @param rFile
         * @return true if file exsits
         */
        bool existsFile(const boost::filesystem::path &rFile);
        
        
        /** checks if a folder exists
         * @param rFolder
         * @return true if folder exsits
         */
        bool existsFolder(const boost::filesystem::path &dir) ;
        
        /** checks if folder already exists and if not, creates one
         * @param folder_name
         */
        void createDirIfNotExist(const boost::filesystem::path &dir);
        
        /** checks if the path for the filename already exists,
         * otherwise creates it
         * @param filename
         */
        void createDirForFileIfNotExist(const boost::filesystem::path &filename);
        
        /** Returns folder names in a folder </br>
         * @param dir
         * @return relative_paths
         */
        std::vector<std::string> getFoldersInDirectory(const boost::filesystem::path &dir);
        
        
        /** Returns a the name of files in a folder </br>
         * '(.*)bmp'
         * @param dir
         * @param regex_pattern examples "(.*)bmp",  "(.*)$"
         * @param recursive (true if files in subfolders should be returned as well)
         * @return files in folder
         */
        std::vector<std::string> getFilesInDirectory(const boost::filesystem::path &dir,
                                                     const std::string &regex_pattern = std::string(""), bool recursive = true);
        
     
        
        /** @brief copies a directory from source to destination
         * @param path of source directory
         * @param path of destination directory
         */
        void copyDir(const bf::path &sourceDir, const bf::path &destinationDir);
    
        /**
         * @brief removeDir remove a directory with all its contents (including subdirectories) from disk
         * @param path folder path
         */
        void removeDir(const bf::path &path);
    }
}

#endif

