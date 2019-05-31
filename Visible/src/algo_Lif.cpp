//
//  algo_Lif.cpp
//  Visible
//
//  Created by Arman Garakani on 8/20/18.
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wunused-private-field"


#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <map>
#include "async_tracks.h"
#include "core/signaler.h"
#include "sm_producer.h"
#include "cinder_cv/cinder_xchg.hpp"
#include "cinder_cv/cinder_opencv.h"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "algo_Lif.hpp"
#include "logger/logger.hpp"
#include "result_serialization.h"
#include "vision/rc_filter1d.h"


const fPair& lif_serie_processor::ellipse_ab () const { return m_ab; }

const Rectf& lif_serie_processor::measuredArea() const { return m_measured_area; }

// Check if the returned has expired
std::weak_ptr<contraction_analyzer> lif_serie_processor::contractionWeakRef ()
{
    std::weak_ptr<contraction_analyzer> wp (m_caRef);
    return wp;
}



const std::vector<sides_length_t>& lif_serie_processor::cell_ends() const{
    return m_cell_ends;
}

void lif_serie_processor::generate_affine_windows () {
 
    int channel_to_use = m_channel_count - 1;
    internal_generate_affine_windows(m_all_by_channel[channel_to_use]);
}
/*
 * 1 monchrome channel. Compute 3D Standard Dev. per pixel
 */

void lif_serie_processor::run_detect_geometry (std::vector<roiWindow<P8U>>& images){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_3d_stats_done = false;
    
    std::vector<std::thread> threads(1);
    threads[0] = std::thread(&lif_serie_processor::generateVoxelsAndSelfSimilarities, this, images);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}


void lif_serie_processor::internal_generate_affine_windows (const std::vector<roiWindow<P8U>>& rws){

    std::unique_lock<std::mutex> lock(m_mutex);

    
   // std::string msg = " lproc thread: " + stl_utils::tostr(std::this_thread::get_id());
  //  vlogger::instance().console()->info(msg);
    
 
    auto affineCrop = [] (const vector<roiWindow<P8U> >::const_iterator& rw_src, cv::RotatedRect& rect){
      

        auto matAffineCrop = [] (Mat input, const RotatedRect& box){
            double angle = box.angle;
            auto size = box.size;
            
            //Adjust the box angle
            // Already Done
         //   std::string msg =             svl::to_string(box);
         //   vlogger::instance().console()->info(msg);

            //Rotate the text according to the angle
            auto transform = getRotationMatrix2D(box.center, angle, 1.0);
            Mat rotated;
            warpAffine(input, rotated, transform, input.size(), INTER_CUBIC);
            
            //Crop the result
            Mat cropped;
            getRectSubPix(rotated, size, box.center, cropped);
            copyMakeBorder(cropped,cropped,0,0,0,0,BORDER_CONSTANT,Scalar(0));
            return cropped;
        };

        cvMatRefroiP8U(*rw_src, src, CV_8UC1);
        return matAffineCrop(src, rect);

    };


    auto horizontal_vertical_projections = [](const cv::Mat& mm){
        cv::Mat hz (1, mm.cols, CV_32F);
        cv::Mat vt (mm.rows, 1, CV_32F);
        reduce(mm,vt,1,CV_REDUCE_SUM,CV_32F);
        reduce(mm,hz,0,CV_REDUCE_SUM,CV_32F);
        std::vector<float> hz_vec(mm.cols);
        for (auto ii = 0; ii < mm.cols; ii++) hz_vec[ii] = hz.at<float>(0,ii);
        std::vector<float> vt_vec(mm.rows);
        for (auto ii = 0; ii < mm.rows; ii++) vt_vec[ii] = vt.at<float>(ii,0);
        return std::make_pair(hz_vec, vt_vec);
    };
    
    uint32_t count = 0;
    uint32_t total = static_cast<uint32_t>(rws.size());
    m_affine_windows.clear();
    m_hz_profiles.clear();
    m_vt_profiles.clear();
    vector<roiWindow<P8U> >::const_iterator vitr = rws.begin();
    do
    {
        cv::Mat affine = affineCrop(vitr, m_motion_mass);
        auto clone = affine.clone();
        auto hvpair = horizontal_vertical_projections(clone);
        m_affine_windows.push_back(clone);
        m_hz_profiles.push_back(hvpair.first);
        m_vt_profiles.push_back(hvpair.second);
        count++;
    }
    while (++vitr != rws.end());
    assert(count==total);

    internal_generate_affine_translations();
    internal_generate_affine_profiles();
    save_affine_windows ();
    save_affine_profiles();
    
}

void lif_serie_processor::save_affine_windows (){

    if(fs::exists(mCurrentSerieCachePath)){
        std::string subdir ("affine_cell_images");
        auto save_path = mCurrentSerieCachePath / subdir;
        boost::system::error_code ec;
        if(!fs::exists(save_path)){
            fs::create_directory(save_path, ec);
            switch( ec.value() ) {
                case boost::system::errc::success: {
                    std::string msg = "Created " + save_path.string() ;
                    vlogger::instance().console()->info(msg);
                }
                break;
                default:
                    std::string msg = "Failed to create " + save_path.string() + ec.message();
                    vlogger::instance().console()->info(msg);
            }
        } // tried creatring it if was not already
        if(fs::exists(save_path)){
            std::lock_guard<std::mutex> process_lock(m_io_mutex);
            auto writer = get_image_writer();
            writer->operator()(save_path.string(), m_affine_windows);
        }
    }
}


void lif_serie_processor::save_affine_profiles (){
    
    if(fs::exists(mCurrentSerieCachePath)){
        std::string subdir ("affine_cell_profiles");
        auto save_path = mCurrentSerieCachePath / subdir;
        boost::system::error_code ec;
        if(!fs::exists(save_path)){
            fs::create_directory(save_path, ec);
            switch( ec.value() ) {
                case boost::system::errc::success: {
                    std::string msg = "Created " + save_path.string() ;
                    vlogger::instance().console()->info(msg);
                }
                    break;
                default:
                    std::string msg = "Failed to create " + save_path.string() + ec.message();
                    vlogger::instance().console()->info(msg);
            }
        } // tried creatring it if was not already
        if(fs::exists(save_path)){
            std::lock_guard<std::mutex> process_lock(m_io_mutex);
            auto this_csv_writer = std::make_shared<ioImageWriter>("hp");
            this_csv_writer->operator()(save_path.string(), m_hz_profiles);
            auto other_csv_writer = std::make_shared<ioImageWriter>("vp");
            other_csv_writer->operator()(save_path.string(), m_vt_profiles);
        }
    }
}

void lif_serie_processor::internal_generate_affine_translations (){
    if(m_affine_windows.empty()) return;

    auto trans = [] (const cv::Mat& minus, const cv::Mat& plus){
        auto tw = minus.cols / 3;
        auto th = minus.rows / 3;
        auto width = minus.cols;
        auto height = minus.rows;
        auto cw = (width - tw)/2;
        auto ch = (height-th)/2;
        auto left = cv::Mat(minus, cv::Rect(tw,ch,tw,th));
        auto right = cv::Mat(minus, cv::Rect(cw,ch,tw,th));
        auto plus_left = cv::Mat(plus, cv::Rect(0,0,plus.cols/2, plus.rows));
        auto plus_right = cv::Mat(plus, cv::Rect(plus.cols/2,0,plus.cols/2, plus.rows));
        
        cv::Mat norm_xcorr_img;
        std::pair<cv::Point,cv::Point> max_loc;
        dPair max_val;
        cv::matchTemplate(plus_left,left, norm_xcorr_img, cv::TM_CCORR_NORMED, left);
        cv::minMaxLoc(norm_xcorr_img, NULL, &max_val.first, NULL, &max_loc.first);
        cv::matchTemplate(plus_right,right, norm_xcorr_img, cv::TM_CCORR_NORMED, right);
        cv::minMaxLoc(norm_xcorr_img, NULL, &max_val.second, NULL, &max_loc.second);
        fVector_2d lv(max_loc.first.x,max_loc.first.y);
        fVector_2d rv(max_loc.second.x+plus.cols/2.0f,max_loc.second.y);
        return lv.distance(rv);
    };

    std::vector<float> translations;
    auto d0 = trans(m_affine_windows[0],m_affine_windows[0]);
    translations.push_back(static_cast<float>(d0));
    auto iter = m_affine_windows.begin();
    while (++iter < m_affine_windows.end()){
        auto tt = trans(*iter,*(iter-1));
        translations.push_back(static_cast<float>(tt));
    }
    assert(translations.size() == m_affine_windows.size());
    m_affine_translations = translations;
    m_affine_translations[0] = m_affine_translations[1];
    
}
void lif_serie_processor::internal_generate_affine_profiles(){
    
    m_norm_affine_translations = m_affine_translations;
    auto extremes = svl::norm_min_max(m_norm_affine_translations.begin(), m_norm_affine_translations.end());
    m_length_range.first = extremes.first;
    m_length_range.second = extremes.second;
    
    for (auto& mm : m_norm_affine_translations)
    {
        m_cell_lengths.push_back(mm);
    }
}
/**
 finalize_segmentation
    [grayscale,bi_level] -> create a blob runner, attach res_ready_lambda as callback, run blob runner
    [res_ready_lambda] processes only the top blob result (@todo reference blob result sorting criteria)
                       create an affine transform to affine warp the cell area
                       create a cell gl model for drawing it.
 
 @param mono gray scale image
 @param bi_level bi_level ( i.e. binarized image )
 */
void lif_serie_processor::finalize_segmentation (cv::Mat& mono, cv::Mat& bi_level, iPair& pad_trans){
    
    std::lock_guard<std::mutex> lock(m_segmentation_mutex);
    assert(mono.cols == bi_level.cols && mono.rows == bi_level.rows);
    cv::Rect padded_rect, image_rect;
    
    m_main_blob = labelBlob::create(mono, bi_level, m_params.min_seqmentation_area(), 666);
    
//    auto m_surface_affine = std::make_unique<cv::Mat>();
//    auto m_cell_ends = std::make_unique< std::vector<sides_length_t>>(2);
//    auto m_blobs = std::make_unique<std::vector<blob>>();
//    auto m_ab = std::make_unique<fPair>();
    auto cache_path = mCurrentSerieCachePath;
    std::function<labelBlob::results_cb> res_ready_lambda = [&,
                                                             m_blobs = m_blobs,
                                                             mono=mono,
                                                             pad_trans=pad_trans,
                                                             cache_path=cache_path,
                                                             signal_geometry_available=signal_geometry_available
                                                                   ](std::vector<blob>& blobs){
        if (! blobs.empty()){
          //  m_blobs->push_back(blobs[0]);
            RotatedRect motion_mass = blobs[0].rotated_roi();
            std::vector<cv::Point2f> corners(4);
       //     std::string msg = svl::to_string(motion_mass);
      //      msg = " < " + msg;
      //      vlogger::instance().console()->info(msg);
            motion_mass.points(corners.data());
            pointsToRotatedRect(corners, motion_mass);
            motion_mass.center.x += pad_trans.first;
            motion_mass.center.y += pad_trans.second;
        //   msg = svl::to_string(motion_mass);
         //   msg = " > " + msg;
         //   vlogger::instance().console()->info(msg);
            std::vector<cv::Point2f> mid_points;
            svl::get_mid_points(motion_mass, mid_points);
            m_surface_affine = cv::getRotationMatrix2D(motion_mass.center,motion_mass.angle, 1);
            m_surface_affine.at<double>(0,2) -= (motion_mass.center.x - motion_mass.size.width/2);
            m_surface_affine.at<double>(1,2) -= (motion_mass.center.y - motion_mass.size.height/2);
      
            
            cv::Mat affine;
            cv::warpAffine(mono, affine, m_surface_affine,
                           motion_mass.size, INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0,0,0));
            if(fs::exists(mCurrentSerieCachePath)){
                std::string imagename = "voxel_affine_.png";
                auto image_path = cache_path / imagename;
                cv::imwrite(image_path.string(), affine);
            }
            

            auto two_pt_dist = [mid_points](int a, int b){
                const cv::Point2f& pt1 = mid_points[a];
                const cv::Point2f& pt2 = mid_points[b];
                return sqrt(pow(pt1.x-pt2.x,2)+pow(pt1.y-pt2.y,2));
            };
            
            if (mid_points.size() == 4){
                float dists[2];
                float d = two_pt_dist(0,2);
                dists[0] = d;
                d = two_pt_dist(1,3);
                dists[1] = d;
                if (dists[0] >= dists[1]){
                    m_cell_ends[0].first = vec2(mid_points[0].x,mid_points[0].y);
                    m_cell_ends[0].second = vec2(mid_points[2].x,mid_points[2].y);
                    m_cell_ends[1].first = vec2(mid_points[1].x,mid_points[1].y);
                    m_cell_ends[1].second = vec2(mid_points[3].x,mid_points[3].y);
                }
                else{
                    m_cell_ends[1].first = vec2(mid_points[0].x,mid_points[0].y);
                    m_cell_ends[1].second = vec2(mid_points[2].x,mid_points[2].y);
                    m_cell_ends[0].first = vec2(mid_points[1].x,mid_points[1].y);
                    m_cell_ends[0].second = vec2(mid_points[3].x,mid_points[3].y);
                }
            }
            m_ab = blobs[0].moments().getEllipseAspect ();
            m_motion_mass = motion_mass;
            
            // Signal to listeners
            if (signal_geometry_available && signal_geometry_available->num_slots() > 0)
                signal_geometry_available->operator()();
        }
    };
 
    boost::signals2::connection results_ready_ = m_main_blob->registerCallback(res_ready_lambda);
    m_main_blob->run_async();
  

    
}

void lif_serie_processor::run_detect_geometry (const int channel_index){
    return run_detect_geometry(m_all_by_channel[channel_index]);
}


const cv::RotatedRect& lif_serie_processor::motion_surface() const { return m_motion_mass; }
const cv::Mat& lif_serie_processor::surfaceAffine() const { return m_surface_affine; }
const fPair& lif_serie_processor::length_range() const { return m_length_range; }


// Update. Called also when cutoff offset has changed
void lif_serie_processor::update ()
{
    if(m_contraction_pci_tracksRef && !m_contraction_pci_tracksRef->empty() && m_caRef && m_caRef->isValid())
        median_leveled_pci(m_contraction_pci_tracksRef->at(0));
}


void lif_serie_processor::contraction_analyzed (contractionContainer_t& contractions)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Call the contraction available cb if any
    if (signal_contraction_available && signal_contraction_available->num_slots() > 0)
    {
        contractionContainer_t copied = m_caRef->contractions();
        signal_contraction_available->operator()(copied);
    }
    vlogger::instance().console()->info(" Contractions Analyzed: ");
}



void pointsToRotatedRect (std::vector<cv::Point2f>& imagePoints, cv::RotatedRect& rotated_rect ){
    // get the left most
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.x > b.x;
    });
    
    std::sort (imagePoints.begin(), imagePoints.end(),[](const cv::Point2f&a, const cv::Point2f&b){
        return a.y < b.y;
    });
    
    auto cwOrder = [&] (std::vector<cv::Point2f>& cc, std::vector<std::pair<std::pair<int,int>,float>>& results) {
        const float d01 = glm::distance( fromOcv(cc[0]), fromOcv(cc[1]) );
        const float d02 = glm::distance( fromOcv(cc[0]), fromOcv(cc[2]) );
        const float d03 = glm::distance( fromOcv(cc[0]), fromOcv(cc[3]) );
        
        std::vector<std::pair<std::pair<int,int>,float>> ds = {{{0,1},d01}, {{0,2},d02}, {{0,3},d03}};
        std::sort (ds.begin(), ds.end(),[](std::pair<std::pair<int,int>,float>&a, const std::pair<std::pair<int,int>,float>&b){
            return a.second > b.second;
        });
        results = ds;
        auto print = [](std::pair<std::pair<int,int>,float>& vv) { std::cout << "::" << vv.second; };
        std::for_each(ds.begin(), ds.end(), print);
        std::cout << '\n';
    };
    
    std::vector<std::pair<std::pair<int,int>,float>> ranked_corners;
    cwOrder(imagePoints, ranked_corners);
    assert(ranked_corners.size()==3);
    fVector_2d tl (imagePoints[ranked_corners[1].first.first].x, imagePoints[ranked_corners[1].first.first].y);
    fVector_2d tr (imagePoints[ranked_corners[1].first.second].x, imagePoints[ranked_corners[1].first.second].y);
    fVector_2d bl (imagePoints[ranked_corners[2].first.second].x, imagePoints[ranked_corners[2].first.second].y);
    fLineSegment2d sideOne (tl, tr);
    fLineSegment2d sideTwo (tl, bl);
    
    
    cv::Point2f ctr(0,0);
    for (auto const& p : imagePoints){
        ctr.x += p.x;
        ctr.y += p.y;
    }
    ctr.x /= 4;
    ctr.y /= 4;
    rotated_rect.center = ctr;
    float dLR = sideOne.length();
    float dTB = sideTwo.length();
    rotated_rect.size.width = dTB;
    rotated_rect.size.height = dLR;
    std::cout << toDegrees(sideOne.angle().Double()) << " Side One " << sideOne.length() << std::endl;
    std::cout << toDegrees(sideTwo.angle().Double()) << " Side Two " << sideTwo.length() << std::endl;
    
    uDegree angle = sideTwo.angle();
    if (angle.Double() < -45.0){
        angle = angle + uDegree::piOverTwo();
        std::swap(rotated_rect.size.width,rotated_rect.size.height);
    }
    rotated_rect.angle = angle.Double();
}

#pragma GCC diagnostic pop


