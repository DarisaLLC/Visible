#include <iostream>
#include <string>
//#include <stlplus_lite.hpp>
#include <unistd.h>
#include <gtest/gtest.h>
#include "ut_file.hpp"
#include "ut_sm.hpp"
#include "qtime_cache.h"
#include "vf_utils.hpp"
#include "vf_sm_producer.h"
#include "ut_qtime.h"
#include "frame.h"
#include "vf_types.h"
#include "vf_utils.hpp"

//#include "../applications/opencv_bench/awb.hpp"
using namespace ci;
using namespace std;
using namespace vf_utils::file_system;


class genv: public testing::Environment
{
public:
    genv (const path& exec_path) : m_exec_path (exec_path)
    {
        m_exec_path = m_exec_path.parent_path();
    }
    typedef std::vector<path> path_vec;             // store paths,

    const std::string test_folder_name = "test";
    const std::string test_data_folder_name = "test_data";

    const path& executable_folder () { return m_exec_path; }
    const std::string& test_folder () { return m_test_path.string(); }
    const std::string& test_data_folder () { return m_test_content_path.string(); }
    
    void SetUp ()
    {
        m_test_content_path = "";
        m_test_path = "";
        
        path bin_path = executable_folder ();
        
        using namespace boost::filesystem;

        path it(bin_path);
        path end = m_exec_path.root_path ();
        
        while (it != end)
        {
            path_vec testies;
            if (has_directory(it, test_folder_name, testies ) && has_directory (testies[0], test_data_folder_name, testies) )
            {
                m_test_path = testies[0];
                m_test_content_path = testies[1];
                break;
            }
            it = it.parent_path ();
            
        }
        

        directory_iterator itr(m_test_content_path); directory_iterator end_itr;
        std::cout << m_test_content_path.root_path() << std::endl; std::cout << m_test_content_path.parent_path() << std::endl;
        while(itr!=end_itr && !is_directory(itr->path())){
            std::cout << "-----------------------------------"<< std::endl;
            std::cout << "Path: "<< itr->path()<< std::endl;
            std::cout << "Filename: "<< itr->path().filename()<< std::endl;
            std::cout << "Is File: "<< is_regular_file(itr->path()) << std::endl; std::cout << "File Size :"<< file_size(itr->path())<< std::endl; std::cout << "-----------------------------------"<< std::endl;
            itr ++;
        }
    }
    
    void TearDown () {}
private:
    path m_root_path;
    path m_test_content_path;
    path m_test_path;
    path m_exec_path;
    bool is_test_root_directory (path& candidate, path_vec& paths)
    {
        
        bool ok = is_directory (candidate) && candidate.filename() == test_folder_name;
        if (!ok) return ok;
        paths.push_back (candidate);
        
        path_vec v;                                // so we can sort them later
        path tp (candidate);
        copy(directory_iterator(tp), directory_iterator(), back_inserter(v));
        
        for (path_vec::const_iterator vit (v.begin()); vit != v.end(); ++vit)
        {
            if (vit->filename() == test_data_folder_name)
            {
                paths.push_back (*vit);
                return true;
            }
        }
        
        return false;
    }
    
    bool has_directory (path& candidate, const string& test, path_vec& paths)
    {
        std::cout << "name: " << test << std::endl;

        bool ok = is_directory (candidate);
        if (!ok) return ok;
        
        path_vec v;                                // so we can sort them later
        path tp (candidate);
        copy(directory_iterator(tp), directory_iterator(), back_inserter(v));
        
        for (path_vec::const_iterator vit (v.begin()); vit != v.end(); ++vit)
        {
            if (vit->filename().string() == test)
            {
                paths.push_back (*vit);
                return true;
            }
        }
        
        return false;
    }
    
};

genv* s_gvp = 0;


TEST (UT_fileutils, run)
{
    {
    //UT_fileutils test (s_s_gvp->test_data_folder ());
    //EXPECT_EQ(0, test.run () );
    
    std::string txtfile ("onecolumn.txt");
    std::string csv_filename = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder(), txtfile);
    
    path fpath ( csv_filename );
    
#ifdef HAS_AUDIO_2
    unique_ptr<SourceFile> sfr =  unique_ptr<SourceFile>( new vf_cinder::VisibleAudioSource (DataSourcePath::create (fpath )) );
    EXPECT_TRUE (sfr->getNumChannels() == 1);
    EXPECT_TRUE (sfr->getNumFrames () == 3296);
#endif
        
    std::string matfile ("matrix.txt");
    std::string mat_filename = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder(), matfile);
    vector<vector<float> > matrix;
    vf_utils::csv::csv2vectors(mat_filename, matrix, false, false, true);
    EXPECT_TRUE(matrix.size() == 300);
    for (int rr=0; rr < matrix.size(); rr++)
        EXPECT_TRUE(matrix[rr].size() == 300);
    
    }
    
    //    fs::path fpath ( csv_filename );
    
    
}


TEST( UT_FrameBuf, run )
{

//	UT_FrameBuf test;
//	EXPECT_EQ(0, test.run());
    
    
    // cached_frame_ref
    {
        cached_frame_ref buf( new raw_frame ( 640, 480, rpixel8 ) );
        
       EXPECT_TRUE( buf.refCount() == 1 );
        
        cached_frame_ref share1( buf );
        
        EXPECT_TRUE( share1.refCount() == 2 );
        {
            cached_frame_ref share2( buf );
            EXPECT_TRUE( share2.refCount() == 3 );
        }
        EXPECT_TRUE( share1.refCount() == 2 );
    }
    
    
}



TEST (UT_QtimeCache, run)
{
    
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.mov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    
    UT_QtimeCache test (qmov);
//    test.set_which ();
    EXPECT_EQ(0, test.run () );
}


TEST(cb_similarity_producer, run)
{
    static std::string qmov_name ("box-move.mov");
    static std::string rfymov_name ("box-move.rfymov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);

    if (0)
    {
        cb_similarity_producer test (rfymov);
        EXPECT_EQ(0, test.run () );
        EXPECT_EQ(true, test.is_movie_loaded () );
        EXPECT_EQ(57, test.frameCount ());
        EXPECT_EQ(57, test.sp->shannonProjection().size () );
        EXPECT_EQ(false, vf_utils::math::contains_nan(test.sp->shannonProjection().begin(), test.sp->shannonProjection().end()));
        
        EXPECT_EQ(true, test.mlies.empty());
    }

    {
        cb_similarity_producer test (qmov);
        EXPECT_EQ(0, test.run () );
        EXPECT_EQ(true, test.is_movie_loaded () );
        EXPECT_EQ(56, test.frameCount ());
        EXPECT_EQ(56, test.sp->shannonProjection().size () );
        EXPECT_EQ(false, vf_utils::math::contains_nan(test.sp->shannonProjection().begin(), test.sp->shannonProjection().end()));
        
        EXPECT_EQ(true, test.mlies.empty());
    }
    
}



#if 0

TEST(cinder_qtime_grabber, run)
{
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.mov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    boost::shared_ptr<rcFrameGrabber> grabber (reinterpret_cast<rcFrameGrabber*> (new vf_utils::qtime_support::CinderQtimeGrabber( qmov )) );
    
    EXPECT_TRUE (grabber.get() != NULL);
    
    grabber_error error = grabber->getLastError();
    int i = 0;
    uint8 dummy;
    // Grab everything
    EXPECT_TRUE (grabber->start());
    EXPECT_TRUE (grabber->frameCount() == 56);
    std::vector<rcFrameRef> images;
    // Note: infinite loop
    for( i = 0; ; ++i )
    {
        rcTimestamp curTimeStamp;
        rcRect videoFrame;
        rcWindow image, tmp;
        rcFrameRef framePtr;
        rcFrameGrabberStatus status = grabber->getNextFrame( framePtr, true );
        EXPECT_TRUE((status == rcFrameGrabberStatus::eFrameStatusEOF || status == rcFrameGrabberStatus::eFrameStatusOK ) );
        if (status != rcFrameGrabberStatus::eFrameStatusOK) break;
        images.push_back (framePtr);
        rcWindow win (framePtr);
        //        win.print(dummy, std::cout);
    }// End of For i++
    
    EXPECT_TRUE(images.size() == 56);
    
    EXPECT_TRUE(grabber->stop());
    
}



TEST(cinder_qtime_grabber_and_similarity, run)
{
 
    static std::string qmov_name ("box-move.mov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    boost::shared_ptr<rcFrameGrabber> grabber (reinterpret_cast<rcFrameGrabber*> (new vf_utils::qtime_support::CinderQtimeGrabber( qmov )) );
    ((vf_utils::qtime_support::CinderQtimeGrabber*)grabber.get())->print_to_ (std::cout);
    
    EXPECT_TRUE (grabber.get() != NULL);
    
    grabber_error error = grabber->getLastError();
    int i = 0;
    uint8 dummy;
    // Grab everything
    EXPECT_TRUE (grabber->start());
    EXPECT_TRUE (grabber->frameCount() == 56);
    //    std::vector<rcFrameRef> images;
    std::vector<rcWindow> images;
    // Note: infinite loop
    for( i = 0; ; ++i )
    {
        rcTimestamp curTimeStamp;
        rcRect videoFrame;
        rcWindow image, tmp;
        rcFrameRef framePtr;
        rcFrameGrabberStatus status = grabber->getNextFrame( framePtr, true );
        EXPECT_TRUE((status == rcFrameGrabberStatus::eFrameStatusEOF || status == rcFrameGrabberStatus::eFrameStatusOK ) );
        if (status == rcFrameGrabberStatus::eFrameStatusEOF) break;
        rcWindow win (framePtr);
        images.push_back (win);
        
        //  win.print(dummy, std::cout);
    }// End of For i++
    
    EXPECT_TRUE(images.size() == 56);
    
    std::vector<rcWindow> rand_images;

    for (uint32 i = 0; i < images.size(); i++)
    {
        EXPECT_EQ(true, images[i].isGray());
    }
    
    
    for (uint32 i = 0; i < images.size(); i++)
    {
        rcWindow tmp(16,16);
        tmp.copyPixelsFromWindow(images[i]);
        rand_images.push_back(tmp);
    }
    

    boost::shared_ptr<sm_producer> sp;
    sp =  boost::shared_ptr<sm_producer> ( new sm_producer () );
    sp->load_images (rand_images);
    EXPECT_EQ(56, sp->frames_in_content ());
    sp->operator()(0, 0);
    EXPECT_EQ(56, sp->frames_in_content());
    EXPECT_EQ(56, sp->shannonProjection().size () );
    EXPECT_EQ(false, vf_utils::math::contains_nan(sp->shannonProjection().begin(), sp->shannonProjection().end()));

    
    EXPECT_TRUE(grabber->stop());
    

}


TEST(UT_similarity_producer, run)
{
    // vf does not support QuickTime natively. The ut expectes and checks for failure
    static std::string qmov_name ("box-move.mov");
    static std::string rfymov_name ("box-move.rfymov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);
    
	UT_similarity_producer test (rfymov, qmov);
    EXPECT_EQ(0, test.run());
}



TEST (UT_videocache, run)
{
    
    static std::string rfymov_name ("rev2.rfymov");
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);
    
    UT_VideoCache test (rfymov);
    EXPECT_EQ(0, test.run () );
}





TEST( ut_similarity, run )
{
	UT_similarity test;
	EXPECT_EQ(0, test.run());
}


TEST (UT_movieconverter, run)
{
    
    static std::string qmov_name ("box-move.mov");
    static std::string rfymov_name ("box-move.rfymov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);
    
    UT_movieconverter test (rfymov.c_str(), qmov.c_str () );
    
    EXPECT_EQ (0, test.run());
}


TEST ( UT_ReifyMovieGrabber, run )
{
    
    static std::string rfymov_name ("rev2.rfymov");
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);
    
    UT_ReifyMovieGrabber test ( rfymov );
    EXPECT_EQ(0, test.run () );
}


TEST (UT_videocache, run)
{
    
    static std::string rfymov_name ("rev2.rfymov");
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);
    
    UT_VideoCache test (rfymov);
    EXPECT_EQ(0, test.run () );
}



TEST(UT_similarity_producer, run)
{
    
    static std::string qmov_name ("box-move.mov");
    static std::string rfymov_name ("box-move.rfymov");
    std::string qmov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), qmov_name);
    std::string rfymov = vf_utils::file_system::create_filespec  (s_gvp->test_data_folder (), rfymov_name);
    
	UT_similarity_producer test (rfymov, qmov);
    EXPECT_EQ(0, test.run());
}


#endif


int main(int argc, char **argv)
{
    std::string installpath = std::string (argv[0]);
    path install_path (installpath);
    ::testing::Environment* const g_env  = ::testing::AddGlobalTestEnvironment(new     genv (installpath) );
    s_gvp = reinterpret_cast<genv*>(g_env);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
    
	
	
}// TEST(GTestMainTest, ShouldSucceed) { }






