//
//  labelBlob.cpp
//  Visible
//
//  Created by Arman Garakani on 9/3/18.
//

#include "labelBlob.hpp"
#include "core/stl_utils.hpp"
#include <chrono>


using namespace svl;

/***************
 #define CV_8U   0
 #define CV_8S   1
 #define CV_16U  2
 #define CV_16S  3
 #define CV_32S  4       stats
 #define CV_32F  5
 #define CV_64F  6       centroids
 
 ****************/

using blob = svl::labelBlob::blob;

void momento::run(const cv::Mat& image) const
{
    image.locateROI(m_size,m_offset);
    
    // Get the basic moments from Cv::Moments. It does respect ROI
    // Fill up the base and only the base.
    CvMoments mu =  cv::moments(image, false);
    *((CvMoments*)this) = mu;
    
    inv_m00 = mu.inv_sqrt_m00 * mu.inv_sqrt_m00;
    mc = Point2f ((m10 * inv_m00), m01 * inv_m00);
    mc.x += m_offset.x;
    mc.y += m_offset.y;
    m_is_loaded = true;
}

momento::momento(const momento& other){
    *this = other;
}
momento::momento(const cv::Mat& image)
{
    run (image);
}

fPair momento::getEllipseAspect () const
{
    if (! isLoaded()) return fPair();
    
    getDirectionals ();
    return fPair(a,b);
}

uRadian momento::getOrientation () const
{
    if (! isLoaded()) return uRadian(std::numeric_limits<double>::max());
    
    getDirectionals ();
    return theta;
}

void momento::getDirectionals () const
{
    if (m_eigen_done) return;
    
    Mat M = (Mat_<double>(2,2) << m20, m11, m11, m02);
    
    //Compute and store the eigenvalues and eigenvectors of covariance matrix CvMat* e_vect = cvCreateMat( 2 , 2 , CV_32FC1 );
    Mat e_vects = (Mat_<float>(2,2) << 0.0,0.0,0.0,0.0);
    Mat e_vals = (Mat_<float>(1,2) << 0.0,0.0);
    
    bool eigenok = cv::eigen(M, e_vals, e_vects);
    if (eigenok){
        int ev = (e_vals.at<float>(0,0) > e_vals.at<float>(0,1)) ? 0 : 1;
        double angle = atan2 (e_vects.at<float>(ev, 0),e_vects.at<float>(ev, 1));


        double mm2 = m20 - m02;
        auto tana = mm2 + std::sqrt (4*m11*m11 + mm2*mm2);
        theta = atan(2*m11 / tana);
        m_is_nan = isnan(theta);
        
        double cos2 = cos(theta)*cos(theta);
        double sin2 = sin(theta)*sin(theta);
        double sin2x = sin(theta)*cos(theta);

        double lambda1 = m20*cos2 + m11 * sin2x + m02*sin2;
        double lambda2 = m20*cos2 - m11 * sin2x + m02*sin2;

        a = 2.0 * std::sqrt(lambda1/m00);
        b = 2.0 * std::sqrt(lambda2/m00);
        eigen_angle = angle;
    }
    
    m_eigen_ok = eigenok;
    m_eigen_done = true;
}

svl::labelBlob::labelBlob() : m_results_ready(false){
    // Signals we provide
    signal_results_ready = createSignal<results_ready_cb> ();
    signal_graphics_ready = createSignal<graphics_ready_cb> ();
    
}


svl::labelBlob::labelBlob(const cv::Mat& gray, const cv::Mat& threshold_out, const int64_t client_id, const int minAreaPixelCount) : m_results_ready(false){
    // Signals we provide
    signal_results_ready = createSignal<results_ready_cb> ();
    signal_graphics_ready = createSignal<graphics_ready_cb> ();
    reload(gray, threshold_out, client_id,  minAreaPixelCount);
}

svl::labelBlob::ref labelBlob::create(const cv::Mat& gray, const cv::Mat& threshold_out, const int64_t client_id, const int minAreaPixelCount) {
    auto reff = std::make_shared<labelBlob> (gray, threshold_out, minAreaPixelCount, client_id);
    return reff;
}

void svl::labelBlob::blob::update_moments(const cv::Mat& image) const {
    m_moments_ready = false;
    cv::Mat window = image(m_roi);
    m_moments.run(window);
    m_moments_ready = true;
    
}
bool labelBlob::hasResults() const { return m_results_ready; }

void  labelBlob::reload (const cv::Mat& grayImage, const cv::Mat&threshold_output,const int64_t client_id, const int min_area_count) const {
    m_min_area = min_area_count;
    m_grey = grayImage;
    m_threshold_out = threshold_output;
    m_results_ready = false;
    m_client_id = 666;
    
}

void  labelBlob::run() const {
    m_start = std::chrono::high_resolution_clock::now();
    size_t count = cv::connectedComponentsWithStats(m_threshold_out, m_labels, m_stats, m_centroids);
    std::vector<cv::Mat> channels = { m_threshold_out,m_threshold_out,m_threshold_out};
    cv::merge(&channels[0],3,m_graphics);
    assert(count == m_stats.rows);
    
    m_moments.resize(0);
    m_rois.resize(0);
    
    for(uint32_t i=1; i<=m_stats.rows; i++)
    {
        if( m_stats.at<int>(i, cv::CC_STAT_AREA) < m_min_area ) continue;
        int x = m_stats.at<int>(Point(0, i));
        int y = m_stats.at<int>(Point(1, i));
        int w = m_stats.at<int>(Point(2, i));
        int h = m_stats.at<int>(Point(3, i));
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = end-m_start;
        auto id = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
        auto roi = cv::Rect2f (x,y,w,h);
        m_blobs.emplace_back(i, id, roi);
        m_blobs.back().update_moments(m_grey);
    }
    
  
    
    if (signal_results_ready && signal_results_ready->num_slots() > 0)
        signal_results_ready->operator()(m_client_id);
    m_results_ready = true;
}

void labelBlob::run_async() const {
    auto fdone = stl_utils::reallyAsync(&labelBlob::run, this);
}

const std::vector<cv::KeyPoint>& labelBlob::keyPoints (bool regen) const {
    if(! regen && ! (m_kps.empty() || m_blobs.empty() || m_kps.size() != m_blobs.size()))
        return m_kps;
    static RNG rng(12345);
    m_kps.clear();
    for (auto const & blob : m_blobs)
    {
        Scalar color = Scalar( rng.uniform(100, 255), rng.uniform(100,255), rng.uniform(100,255) );
        cv::rectangle(m_graphics, blob.roi(), color, 1);
        uRadian theta = blob.moments().getOrientation();
       
        if (blob.moments().isValidEigen() && ! blob.moments().isNan())
        {
            uDegree degs (theta);
            /*      @param _pt x & y coordinates of the keypoint
             @param _size keypoint diameter
             @param _angle keypoint orientation
             @param _response keypoint detector response on the keypoint (that is, strength of the keypoint)
             @param _octave pyramid octave in which the keypoint has been detected
             */
            m_kps.emplace_back(blob.moments().com(), blob.extend(), degs.Double());
        }else
            m_kps.emplace_back(cv::Point(0,0), 0.0);
    }
    return m_kps;
}
void labelBlob::drawOutput() const {

    const std::vector<cv::KeyPoint>& kps = keyPoints();
    cv::drawKeypoints(m_graphics, kps,m_graphics, cv::Scalar(0,255,0),cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
    
    if (signal_graphics_ready && signal_graphics_ready->num_slots() > 0)
        signal_graphics_ready->operator()();
}


