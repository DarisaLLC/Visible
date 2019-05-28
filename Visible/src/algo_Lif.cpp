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

    
    std::string msg = " lproc thread: " + stl_utils::tostr(std::this_thread::get_id());
    vlogger::instance().console()->info(msg);
    
 
    auto affineCrop = [] (const vector<roiWindow<P8U> >::const_iterator& rw_src, cv::RotatedRect& rect){
      

        auto matAffineCrop = [] (Mat input, const RotatedRect& box){
            double angle = box.angle;
            auto size = box.size;
            
            //Adjust the box angle
            // Already Done
            std::string msg =             svl::to_string(box);
            vlogger::instance().console()->info(msg);

            //Rotate the text according to the angle
            auto transform = getRotationMatrix2D(box.center, angle, 1.0);
            Mat rotated;
            warpAffine(input, rotated, transform, input.size(), INTER_CUBIC);
            
            //Crop the result
            Mat cropped;
            getRectSubPix(rotated, size, box.center, cropped);
            copyMakeBorder(cropped,cropped,10,10,10,10,BORDER_CONSTANT,Scalar(0));
            return cropped;
        };

        cvMatRefroiP8U(*rw_src, src, CV_8UC1);
        return matAffineCrop(src, rect);

    };

    uint32_t count = 0;
    uint32_t total = static_cast<uint32_t>(rws.size());
    m_affine_windows.clear();
    vector<roiWindow<P8U> >::const_iterator vitr = rws.begin();
    do
    {
        cv::Mat affine = affineCrop(vitr, m_motion_mass);
        auto clone = affine.clone();
        m_affine_windows.push_back(clone);
        count++;
    }
    while (++vitr != rws.end());
    assert(count==total);
    
    save_affine_windows (m_affine_windows);
    
}

void lif_serie_processor::save_affine_windows (const std::vector<cv::Mat>& rws){

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
            auto writer = get_image_writer();
            writer->operator()(save_path.string(), m_affine_windows);
        }
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
void lif_serie_processor::finalize_segmentation (cv::Mat& mono, cv::Mat& bi_level){
    
    std::lock_guard<std::mutex> lock(m_segmentation_mutex);
     
    m_main_blob = labelBlob::create(mono, bi_level, m_params.min_seqmentation_area(), 666);
    
    std::function<labelBlob::results_ready_cb> res_ready_lambda = [this](int64_t& cbi)
    {
        const std::vector<blob>& blobs = m_main_blob->results();
        if (! blobs.empty()){
            m_motion_mass = blobs[0].rotated_roi();
            std::vector<cv::Point2f> corners(4);
            std::string msg = svl::to_string(m_motion_mass);
            msg = " < " + msg;
            vlogger::instance().console()->info(msg);
            m_motion_mass.points(corners.data());
            pointsToRotatedRect(corners, m_motion_mass);
            msg = svl::to_string(m_motion_mass);
            msg = " > " + msg;
            vlogger::instance().console()->info(msg);
            std::vector<cv::Point2f> mid_points;
            svl::get_mid_points(m_motion_mass, mid_points);
            m_surface_affine = cv::getRotationMatrix2D(m_motion_mass.center,m_motion_mass.angle, 1);
            m_surface_affine.at<double>(0,2) -= (m_motion_mass.center.x - m_motion_mass.size.width/2);
            m_surface_affine.at<double>(1,2) -= (m_motion_mass.center.y - m_motion_mass.size.height/2);
            

            auto get_trims = [] (const cv::Rect& box, const cv::RotatedRect& rotrect) {
                
                
                //The order is bottomLeft, topLeft, topRight, bottomRight.
                std::vector<float> trims(4);
                cv::Point2f rect_points[4];
                rotrect.points( &rect_points[0] );
                cv::Point2f xtl; xtl.x = box.tl().x; xtl.y = box.tl().y;
                cv::Point2f xbr; xbr.x = box.br().x; xbr.y = box.br().y;
                cv::Point2f dtl (xtl.x - rect_points[1].x, xtl.y - rect_points[1].y);
                cv::Point2f dbr (xbr.x - rect_points[3].x, xbr.y - rect_points[3].y);
                trims[0] = rect_points[1].x < xtl.x ? xtl.x - rect_points[1].x : 0.0f;
                trims[1] = rect_points[1].y < xtl.y ? xtl.y - rect_points[1].y : 0.0f;
                trims[2] = rect_points[3].x > xbr.x ? rect_points[3].x  - xbr.x : 0.0f;
                trims[3] = rect_points[3].x > xbr.y ? rect_points[3].y  - xbr.y : 0.0f;
                std::transform(trims.begin(), trims.end(), trims.begin(),
                               [](float d) -> float { return std::roundf(d); });
                
                return trims;
            };
            
            auto trims = get_trims(cv::Rect(0,0,m_temporal_ss.cols, m_temporal_ss.rows), m_motion_mass);
            for (auto ff : trims) std::cout << ff << std::endl;
            
            cv::Point offset(trims[0],trims[1]);
            fPair pads (trims[0]+trims[2],trims[1]+trims[3]);
            cv::Mat canvas (m_temporal_ss.rows+pads.second, m_temporal_ss.cols+pads.first, CV_8U);
            canvas.setTo(0.0);
            cv::Mat win (canvas, cv::Rect(offset.x, offset.y, m_temporal_ss.cols, m_temporal_ss.rows));
            m_temporal_ss.copyTo(win);
            
            cv::Mat affine;
            cv::warpAffine(canvas, affine, m_surface_affine,
                           m_motion_mass.size, INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0,0,0));
            if(fs::exists(mCurrentSerieCachePath)){
                std::string imagename = "voxel_affine_.png";
                auto image_path = mCurrentSerieCachePath / imagename;
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


