//
//  file_system.cpp
//  Visible
//
//  Created by Arman Garakani on 1/2/19.
//

#include "core/file_system.hpp"
#include <iostream>
#include <boost/algorithm/string.hpp>


namespace svl {
    namespace io {
        
        
        /** checks if a file exists
         * @param rFile
         * @return true if file exsits
         */
        bool existsFile(const boost::filesystem::path &rFile){
            return (bf::exists(rFile) && bf::is_regular_file(rFile));
        }
        
        
        /** checks if a folder exists
         * @param rFolder
         * @return true if folder exsits
         */
        bool existsFolder(const boost::filesystem::path &dir) {
            return bf::exists(dir);
        }
        /** checks if folder already exists and if not, creates one
         * @param folder_name
         */
        void createDirIfNotExist(const boost::filesystem::path &dir){
            if (!bf::exists(dir))
                bf::create_directories(dir);
        }
        
        /** checks if the path for the filename already exists,
         * otherwise creates it
         * @param filename
         */
        void createDirForFileIfNotExist(const boost::filesystem::path &filename){
            createDirIfNotExist(filename.parent_path());
        }
        
        /** Returns folder names in a folder </br>
         * @param dir
         * @return relative_paths
         */
        std::vector<std::string> getFoldersInDirectory(const boost::filesystem::path &dir){
            std::vector<std::string> relative_paths;
            
            bf::directory_iterator end_itr;
            for (bf::directory_iterator itr(dir); itr != end_itr; ++itr) {
                if (bf::is_directory(*itr))
                    relative_paths.push_back(itr->path().filename().string());
            }
            std::sort(relative_paths.begin(), relative_paths.end());
            
            return relative_paths;
        }
        

        /** @brief copies a directory from source to destination
         * @param path of source directory
         * @param path of destination directory
         */
        void copyDir(const bf::path &sourceDir, const bf::path &destinationDir){
             if (!bf::exists(sourceDir) || !bf::is_directory(sourceDir))
                 throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
            
            if (bf::exists(destinationDir)) {
                throw std::runtime_error("Destination directory " + destinationDir.string() + " already exists");
            }
            if (!bf::create_directory(destinationDir)) {
                throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
            }
            
            typedef bf::recursive_directory_iterator RDIter;
            for (auto it = RDIter(sourceDir), end = RDIter(); it != end; ++it) {
                const auto &iteratorPath = it->path();
                auto relativeIteratorPathString = iteratorPath.string();
                boost::replace_first(relativeIteratorPathString, sourceDir.string(), "");
                
                bf::copy(iteratorPath, destinationDir / relativeIteratorPathString);
            }
        }


        
        /**
         * @brief removeDir remove a directory with all its contents (including subdirectories) from disk
         * @param path folder path
         */
        void removeDir(const bf::path &path){
            if (svl::io::existsFolder(path)) {
                for (bf::directory_iterator end_dir_it, it(path); it != end_dir_it; ++it)
                    bf::remove_all(it->path());
                
                bf::remove(path);
            } else
                std::cerr << "Folder " << path.string() << " does not exist." << std::endl;
        }

        
    }
}


