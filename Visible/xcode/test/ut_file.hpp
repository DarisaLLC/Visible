#ifndef __UT_FILE_H
#define __UT_FILE_H


#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <boost/foreach.hpp>
#include <cstdio>
#include <sstream>
#include <string>
#include <iostream>
#include "vf_utils.hpp"
#include "vf_math.h"

using namespace std;
using namespace vf_utils::csv;
using namespace vf_utils::file_system;

struct UT_fileutils 
{
    UT_fileutils(std::string py) : test_data_path (py) {}
    ~UT_fileutils() {}
    
    uint32 run()
    {
        int errs = 0;
        
        errs += (is_directory (test_data_path) == false);
        
        std::string pathy ("/Users/arman/boo/foo/test_o_0.rfymov"); // made up name. File does not exist
        std::string rfmovfile ("test_o_0");
        std::string filename ("test_o_0.rfymov");
        std::string Filename ("test_O_0.rfymov");        
        std::string fileext ("rfymov");
        std::string dotfileext (".rfymov");        
        std::string dotmovfileext (".mov");
        std::string csvfile ("test.csv");

        
        errs += (ext_is_rfymov(pathy) == false);
        errs += (ext_is_rfymov(filename) == false);
        errs += (ext_is_rfymov(Filename) == false);

        std::string csv_filename = vf_utils::file_system::create_filespec (test_data_path, csvfile);
        errs += (file_exists(csv_filename) == false);

        std::ifstream istream (csv_filename.c_str());        
        vf_utils::csv::rows_type rows = vf_utils::csv::to_rows (istream);
        errs += ((rows.size() == 3310) == false);
        
        std::vector<vector<float> > datas;
        BOOST_FOREACH(const vf_utils::csv::row_type &row, rows)
        {
            vector<float> data (4);
            if (row.size () != 4) continue;
            int c=0;
            for (int i = 0; i < 4; i++) 
            {
                std::istringstream iss(row[i]);
                iss >> data[i];
                c++;
            }
            if (c != 4) continue;
            if (rfSum(data) == 0) continue;
            datas.push_back(data);
        }
        
        uint32 expected = rows.size() - 14;
        errs += ( ( datas.size () == expected ) == false);

        int row_start= is_legacy_visible_output (rows);
        EXPECT_TRUE (row_start == 14);
        EXPECT_TRUE (file_is_legacy_visible_output (csv_filename) == 14);

        std::string onecol ("onecolumn.txt");
        std::string onec_filename = vf_utils::file_system::create_filespec (test_data_path, onecol);
        EXPECT_TRUE (file_is_legacy_visible_output (onec_filename) == -3296);    
        
        std::string matx ("matrix.txt");
        std::string matx_filename = vf_utils::file_system::create_filespec (test_data_path, matx);
        EXPECT_TRUE (file_is_legacy_visible_output (matx_filename) == -300);    

        bool only_visible_format = true;

        // test accepting only visible format and getting columns or rows
        test_file (csv_filename, true, 4, only_visible_format, true);
        test_file (csv_filename, true, 3296, only_visible_format, false);     
        test_file (onec_filename, false, 3296, only_visible_format, false);  
        test_file (onec_filename, false, 1, only_visible_format, true);        
        
        // test getting rows
        only_visible_format = false;
        test_file (onec_filename, true, 3296, only_visible_format, false);  
        test_file (onec_filename, true, 1, only_visible_format, true);        
        
        test_file (matx_filename, true, 300, only_visible_format, false);  
        test_file (matx_filename, true, 300, only_visible_format, true);        
        
        return errs;        
    }
    
    void test_file (std::string& fqfn, bool is_valid, int rows_or_columns, bool force_numeric, bool columns_if )
    {
        std::vector<vector<float> > datas;
        bool verify = csv2vectors (fqfn, datas, force_numeric, columns_if);
        EXPECT_TRUE(is_valid == verify);
        if (! is_valid ) { EXPECT_TRUE (datas.empty()); return; }
        EXPECT_TRUE (rows_or_columns == datas.size());
    }
    
private:
    std::string test_data_path;

};

#endif 
