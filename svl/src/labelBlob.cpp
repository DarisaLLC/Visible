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
momento::momento(const cv::Mat& image):m_bilevel(image.clone())
{
    run (image);
}

fPair momento::getEllipseAspect () const
{
    if (! isLoaded()) return fPair();
    
    getDirectionals ();
    return fPair(m_a,m_b);
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

        m_a = 2.0 * std::sqrt(lambda1/m00);
        m_b = 2.0 * std::sqrt(lambda2/m00);
        eigen_angle = angle;
    }
    
    m_eigen_ok = eigenok;
    m_eigen_done = true;
}

svl::labelBlob::labelBlob() : m_results_ready(false){
    // Signals we provide
    signal_results_ready = createSignal<results_ready_cb> ();
    signal_results = createSignal<results_cb> ();
    signal_graphics_ready = createSignal<graphics_ready_cb> ();
    
}


svl::labelBlob::labelBlob(const cv::Mat& gray, const cv::Mat& threshold_out, const int64_t client_id, const int minAreaPixelCount) : m_results_ready(false){
    // Signals we provide
    signal_results_ready = createSignal<results_ready_cb> ();
    signal_results = createSignal<results_cb> ();
    signal_graphics_ready = createSignal<graphics_ready_cb> ();
    reload(gray, threshold_out, client_id,  minAreaPixelCount);
}

svl::labelBlob::ref labelBlob::create(const cv::Mat& gray, const cv::Mat& threshold_out, const int64_t client_id, const int minAreaPixelCount) {
    auto reff = std::make_shared<labelBlob> (gray, threshold_out, minAreaPixelCount, client_id);
    return reff;
}

void svl::labelBlob::blob::update_moments(const cv::Mat& image) const {
    std::lock_guard<std::mutex> lock( m_mutex );
    m_moments_ready = false;
    cv::Mat window = image(m_roi);
    m_moments.run(window);
    m_moments_ready = true;
}

void svl::labelBlob::blob::update_points(const std::vector<cv::Point>& pts) const {
    m_points.clear();
    for (auto const & pt : pts){
        m_points.push_back(pt);
    }
}
cv::RotatedRect svl::labelBlob::blob::rotated_roi() const {
    std::lock_guard<std::mutex> lock( m_mutex );
    uDegree dg = m_moments.getOrientation();
    return cv::RotatedRect(m_moments.com(), m_roi.size(), dg.basic());
}


cv::RotatedRect svl::labelBlob::blob::rotated_roi_PCA() const {
    cv::RotatedRect result;
    
    //1. convert to matrix that contains point coordinates as column vectors
    const cv::Mat& binaryImg = m_moments.biLevel();
    int count = cv::countNonZero(binaryImg);
    if (count == 0) {
        std::cout << "Error::getBoundingRectPCA() encountered 0 pixels in binary image!" << std::endl;
        return cv::RotatedRect();
    }
    
    cv::Mat data(2, count, CV_32FC1);
    int dataColumnIndex = 0;
    for (int row = 0; row < binaryImg.rows; row++) {
        for (int col = 0; col < binaryImg.cols; col++) {
            if (binaryImg.at<unsigned char>(row, col) != 0) {
                data.at<float>(0, dataColumnIndex) = (float) col; //x coordinate
                data.at<float>(1, dataColumnIndex) = (float) (binaryImg.rows - row); //y coordinate, such that y axis goes up
                ++dataColumnIndex;
            }
        }
    }
    
    //2. perform PCA
    const int maxComponents = 1;
    cv::PCA pca(data, cv::Mat() /*mean*/, CV_PCA_DATA_AS_COL, maxComponents);
    //result is contained in pca.eigenvectors (as row vectors)
    //std::cout << pca.eigenvectors << std::endl;
    
    //3. get angle of principal axis
    float dx = pca.eigenvectors.at<float>(0, 0);
    float dy = pca.eigenvectors.at<float>(0, 1);
    float angle = atan2f(dy, dx)  / (float)CV_PI*180.0f;
    
    //find the bounding rectangle with the given angle, by rotating the contour around the mean so that it is up-right
    //easily finding the bounding box then
    cv::Point2f center(pca.mean.at<float>(0,0), binaryImg.rows - pca.mean.at<float>(1,0));
    cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, -angle, 1);
    cv::Mat rotationMatrixInverse = cv::getRotationMatrix2D(center, angle, 1);
    
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(binaryImg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    if (contours.size() != 1) {
        std::cout << "Warning: found " << contours.size() << " contours in binaryImg (expected one)" << std::endl;
        return result;
    }
    
    //turn vector of points into matrix (with points as column vectors, with a 3rd row full of 1's, i.e. points are converted to extended coords)
    int contour_size = static_cast<int>(contours[0].size());
    cv::Mat contourMat(3, contour_size, CV_64FC1);
    double* row0 = contourMat.ptr<double>(0);
    double* row1 = contourMat.ptr<double>(1);
    double* row2 = contourMat.ptr<double>(2);
    for (int i = 0; i < (int) contours[0].size(); i++) {
        row0[i] = (double) (contours[0])[i].x;
        row1[i] = (double) (contours[0])[i].y;
        row2[i] = 1;
    }
    
    cv::Mat uprightContour = rotationMatrix*contourMat;
    
    //get min/max in order to determine width and height
    double minX, minY, maxX, maxY;
    cv::minMaxLoc(cv::Mat(uprightContour, cv::Rect(0, 0, contour_size, 1)), &minX, &maxX); //get minimum/maximum of first row
    cv::minMaxLoc(cv::Mat(uprightContour, cv::Rect(0, 1, contour_size, 1)), &minY, &maxY); //get minimum/maximum of second row
    
    int minXi = cvFloor(minX);
    int minYi = cvFloor(minY);
    int maxXi = cvCeil(maxX);
    int maxYi = cvCeil(maxY);
    
    //fill result
    result.angle = angle;
    result.size.width = (float) (maxXi - minXi);
    result.size.height = (float) (maxYi - minYi);
    
    //Find the correct center:
    cv::Mat correctCenterUpright(3, 1, CV_64FC1);
    correctCenterUpright.at<double>(0, 0) = maxX - result.size.width/2;
    correctCenterUpright.at<double>(1,0) = maxY - result.size.height/2;
    correctCenterUpright.at<double>(2,0) = 1;
    cv::Mat correctCenterMat = rotationMatrixInverse*correctCenterUpright;
    cv::Point correctCenter = cv::Point(cvRound(correctCenterMat.at<double>(0,0)), cvRound(correctCenterMat.at<double>(1,0)));
    
    result.center = correctCenter;
    
    return result;
    
}

bool labelBlob::hasResults() const { return m_results_ready; }

void  labelBlob::reload (const cv::Mat& grayImage, const cv::Mat&threshold_output,const int64_t client_id, const int min_area_count) const {
    std::lock_guard<std::mutex> lock( m_mutex );
    m_min_area = min_area_count;
    m_grey = grayImage;
    m_threshold_out = threshold_output;
    m_results_ready = false;
    m_client_id = 666;
    
}

void  labelBlob::run() const {
    std::lock_guard<std::mutex> lock( m_mutex );
    
    m_start = std::chrono::high_resolution_clock::now();
    
    size_t count = cv::connectedComponentsWithStats(m_threshold_out, m_labels, m_stats, m_centroids);
    std::vector<cv::Mat> channels = { m_threshold_out,m_threshold_out,m_threshold_out};
    cv::merge(&channels[0],3,m_graphics);
    assert(count == m_stats.rows);
    std::map<int,vector<cv::Point>> lablemap;
    for (auto row = 0; row < m_labels.rows; row++)
        for (auto col = 0; col < m_labels.cols; col++)
        {
            auto l = m_labels.at<int>(row,col);
            if (l == 0) continue;
            std::vector<cv::Point> pts = lablemap[l];
            pts.emplace_back(col, row);
            lablemap[l] = pts;
        }
    
    m_moments.resize(0);
    m_rois.resize(0);
    
    auto check = [](int width, int height, cv::Rect2f& cand_roi){
        return (0 <= cand_roi.x && 0 <= cand_roi.width && cand_roi.x + cand_roi.width <= width &&
        0 <= cand_roi.y && 0 <= cand_roi.height && cand_roi.y + cand_roi.height <= height);
    };

    // Use m_stats.at<int>(i, cv::CC_STAT_AREA) to checkout the area
    // std::cout << i << " :  " << m_stats.at<int>(i, cv::CC_STAT_AREA) << std::endl;
    for(uint32_t i=1; i<=m_stats.rows; i++)
    {
        if( m_stats.at<int>(i, cv::CC_STAT_AREA) < m_min_area ) continue;
        int x = m_stats.at<int>(Point(0, i));
        int y = m_stats.at<int>(Point(1, i));
        int w = m_stats.at<int>(Point(2, i));
        int h = m_stats.at<int>(Point(3, i));
        auto roi = cv::Rect2f (x,y,w,h);
        if(!check(m_threshold_out.cols, m_threshold_out.rows, roi)) continue;
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = end-m_start;
        auto id = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
        m_blobs.emplace_back(i, id, roi);
        m_blobs.back().update_moments(m_grey);
        m_blobs.back().update_points(lablemap[i]);
    }
    // Inform listeners of new programs !
    if (signal_results_ready && signal_results_ready->num_slots() > 0)
        signal_results_ready->operator()(m_client_id);
    if (signal_results && signal_results->num_slots() > 0)
        signal_results->operator()(m_blobs);
    
    m_results_ready = true;
}

void labelBlob::run_async() const {
    auto fdone = stl_utils::reallyAsync(&labelBlob::run, this);
}

const std::vector<cv::KeyPoint>& labelBlob::keyPoints (bool regen) const {
    std::lock_guard<std::mutex> lock( m_mutex );
    
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
    std::lock_guard<std::mutex> lock( m_mutex );
    cv::drawKeypoints(m_graphics, kps,m_graphics, cv::Scalar(0,255,0),cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
    
    if (signal_graphics_ready && signal_graphics_ready->num_slots() > 0)
        signal_graphics_ready->operator()();
}


