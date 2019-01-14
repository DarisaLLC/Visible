#ifndef __FILE_SYSTEM_UTILS__
#define __FILE_SYSTEM_UTILS__

#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <iostream>

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
        
#if FIXED_LINK_ISSUE_FOR_getFilesInDirectory
        
        /** Returns a the name of files in a folder </br>
         * '(.*)bmp'
         * @param dir
         * @param regex_pattern examples "(.*)bmp",  "(.*)$"
         * @param recursive (true if files in subfolders should be returned as well)
         * @return files in folder
         */
        std::vector<std::string> getFilesInDirectory(const boost::filesystem::path &dir,
                                                     const std::string &regex_pattern = std::string(""), bool recursive = true){
            std::vector<std::string> relative_paths;
            
            if (!svl::io::existsFolder(dir)) {
                std::cerr << dir << " is not a directory!" << std::endl;
            } else {
                bf::path bf_dir = dir;
                bf::directory_iterator end_itr;
                for (bf::directory_iterator itr(bf_dir); itr != end_itr; ++itr) {
                    const bf::path file = itr->path().filename();
                    
                    // check if its a directory, then get files in it
                    if (bf::is_directory(*itr)) {
                        if (recursive) {
                            bf::path fn = dir / file;
                            std::vector<std::string> files_in_subfolder = getFilesInDirectory(fn.string(), regex_pattern, recursive);
                            for (const auto &sub_file : files_in_subfolder) {
                                bf::path sub_fn = file / sub_file;
                                relative_paths.push_back(sub_fn.string());
                            }
                        }
                    } else {
                        // check for correct file pattern (extension,..) and then add, otherwise ignore..
                        boost::smatch what;
                        const boost::regex file_filter(regex_pattern);
                        if (boost::regex_match(file.string(), what, file_filter))
                            relative_paths.push_back(file.string());
                    }
                }
                std::sort(relative_paths.begin(), relative_paths.end());
            }
            return relative_paths;
        }
#endif
        
        
     
        
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

