//
//  labelBlob.cpp
//  Visible
//
//  Created by Arman Garakani on 9/3/18.
//

#include "vision/labelBlob.hpp"
#include "core/stl_utils.hpp"
#include <chrono>
#include "opencv2/features2d.hpp"

using namespace svl;
using namespace cv;

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

void momento::run(const cv::Mat& image, bool not_binary) const
{
    m_not_binary = not_binary;
    image.locateROI(m_size,m_offset);
    
    // Get the basic moments from Cv::Moments. It does respect ROI
    // Fill up the base and only the base.
    Moments mu =  cv::moments(image, m_not_binary);
    *((Moments*)this) = mu;
    mu11p = mu11 / m00;
    mu20p = mu20 / m00;
    mu02p = mu02 / m00;

    inv_m00 = 1.0 / mu.m00;
    mc = Point2f ((m10 * inv_m00), m01 * inv_m00);
    mc.x += m_offset.x;
    mc.y += m_offset.y;

    m_theta = (mu11p != 0) ? 0.5 * std::atan((2 * mu11p) / (mu20p - mu02p)) : 0.0;
#if 0
    double u30 = m30 - 3.0*mc.x*m20 + 2.0*mc.x*mc.x*m10;
    double u21 = m21 - 2.0*mc.x*m11 - mc.y*m20 + 2.0*mc.x*mc.x*m01;
    double u12 = m12 - 2.0*mc.y*m11 - mc.x*m02 + 2.0*mc.y*mc.y*m10;
    double u03 = m03 - 3*mc.y*m02 + 2*mc.y*mc.y*m01;
    double qx = cos(m_theta);
    double qy = -sin(m_theta);
    
    double s = u30*qx*qx*qx + 3.0*u21*qx*qx*qy + 3.0*u12*qx*qy*qy + u03*qy*qy*qy;
 
    std::cout << "  s  " << s << std::endl;
#endif
    
    m_is_loaded = true;
    m_eigen_ok = false;
    
}

momento::momento(): m_is_loaded (false), m_eigen_done(false), m_is_nan(true), m_contour_ok(false) {
    m_rotation =  cv::Mat::eye(3, 3, CV_32F);
    m_scale =  cv::Mat::eye(3, 3, CV_32F);
    m_translation =  cv::Mat::eye(3, 3, CV_32F);
    m_covar = cv::Mat::zeros(2, 2, CV_32F);
}
momento::momento(const momento& other){
    *this = other;
}
momento::momento(const cv::Mat& image, bool not_binary):m_bilevel(image.clone())
{
    m_rotation =  cv::Mat::eye(3, 3, CV_32F);
    m_scale =  cv::Mat::eye(3, 3, CV_32F);
    m_translation =  cv::Mat::eye(3, 3, CV_32F);
    m_covar = cv::Mat::zeros(2, 2, CV_32F);
    run (image,not_binary);
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
    return m_theta;
}

struct AreaCmp
{
    AreaCmp(const vector<float>& _areas) : areas(&_areas) {}
    bool operator()(int a, int b) const { return (*areas)[a] > (*areas)[b]; }
    const vector<float>* areas;
};

void momento::update_contours(const cv::Mat& image){
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, m_offset);
    if (contours.size() == 0) return;
    
    // Pick the largest one @todo parameterize this
    vector<int> sortIdx(contours.size());
     vector<float> areas(contours.size());
     for( int n = 0; n < (int)contours.size(); n++ ) {
         sortIdx[n] = n;
         areas[n] = contourArea(contours[n], false);
     }
     // sort contours so that largest contours go first
     std::sort(sortIdx.begin(), sortIdx.end(), AreaCmp(areas));
       
     approxPolyDP(contours[sortIdx[0]], m_poly, 1.0, true);
     m_perimeter = arcLength(contours[0], true);
     m_contour_ok = true;
}
/*
 If using Eigen:
 Eigen::Matrix2d covariance;
 covariance(0, 0) = m20;
 covariance(1, 0) = m11;
 covariance(0, 1) = m11;
 covariance(1, 1) = m02;
 
 Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> eigensolver (covariance);
 Eigen::Vector2d evector0 = eigensolver.eigenvectors().col(0);
 Eigen::Vector2d evector1 = eigensolver.eigenvectors().col(1);
 
*/
void momento::getDirectionals () const
{
    if (m_eigen_done) return;
    

//    auto stringifyMat = [](Mat& A){
//        std::stringstream ss;
//        for(int r=0; r<A.rows; r++)
//        {
//            for(int c=0; c<A.cols; c++)
//            {
//                ss <<A.at<double>(r,c)<<'\t';
//            }
//            ss <<endl;
//        }
//        return ss.str();
//    };


    // Compute the covariance matrix in order to determine scaling matrix
   m_covar.at<float>(0, 0) = mu20p;
   m_covar.at<float>(0, 1) = mu11p;
   m_covar.at<float>(1, 0) = mu11p;
   m_covar.at<float>(1, 1) = mu02p;
    
    // Compute eigenvalues and eigenvector
    cv::Mat e_vals, e_vects;
    bool eigenok = cv::eigen(m_covar, e_vals, e_vects);
    
    if (eigenok){
        // Create the scaling matrix
        if (mu20 > mu02) {
            m_scale.at<float>(0, 0) = std::pow(e_vals.at<float>(0, 0) * e_vals.at<float>(1, 0), 0.25) / std::sqrt(e_vals.at<float>(0, 0));
            m_scale.at<float>(1, 1) = std::pow(e_vals.at<float>(0, 0) * e_vals.at<float>(1, 0), 0.25) / std::sqrt(e_vals.at<float>(1, 0));
        }
        else {
            m_scale.at<float>(0, 0) = std::pow(e_vals.at<float>(0, 0) * e_vals.at<float>(1, 0), 0.25) / std::sqrt(e_vals.at<float>(1, 0));
            m_scale.at<float>(1, 1) = std::pow(e_vals.at<float>(0, 0) * e_vals.at<float>(1, 0), 0.25) / std::sqrt(e_vals.at<float>(0, 0));
        }
        
       // auto msg = stringifyMat(m_scale);
        
    }
    if (eigenok){
        int ev = (e_vals.at<float>(0,0) > e_vals.at<float>(0,1)) ? 0 : 1;
        double angle = atan2 (e_vects.at<float>(ev, 0),e_vects.at<float>(ev, 1));
        m_asr = e_vals.at<float>(0,0) / e_vals.at<float>(1,0);
        
        double mm2 = mu20p - mu02p;
        auto tana = mm2 + std::sqrt (4*mu11p*mu11p + mm2*mm2);
        auto theta = atan(2*mu11p / tana);
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

const std::vector<cv::Point>& svl::labelBlob::blob::poly() const {
    return m_moments.polygon();
}

const double& svl::labelBlob::blob::perimeter() const {
    return m_moments.perimeter();
}


void svl::labelBlob::blob::update_contours(const cv::Mat& image) const {
    std::lock_guard<std::mutex> lock( m_mutex );
    m_moments.update_contours(image);
}

svl::labelBlob::labelBlob(const cv::Mat& gray, const cv::Mat& threshold_out, const int64_t client_id, const int minAreaPixelCount) : m_results_ready(false){
    // Signals we provide
    signal_results_ready = createSignal<results_ready_cb> ();
    signal_results = createSignal<results_cb> ();
    signal_graphics_ready = createSignal<graphics_ready_cb> ();
    reload(gray, threshold_out, client_id,  minAreaPixelCount);
}

svl::labelBlob::ref labelBlob::create(const cv::Mat& gray, const cv::Mat& threshold_out, const int32_t client_id, const int minAreaPixelCount) {
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
    cv::PCA pca(data, cv::Mat() /*mean*/, cv::PCA::DATA_AS_COL, maxComponents);
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
    cv::findContours(binaryImg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
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
/*
 Params:
 grayImage: grey scale image
 threshold_out : mask of blob area
*/
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
    // Explicitly return sorted by area
    for(uint32_t i=1; i<m_stats.rows; i++)
    {
        int area = m_stats.at<int>(i, cv::CC_STAT_AREA);
        if( area < m_min_area) continue;
        int x = m_stats.at<int>(Point( CC_STAT_LEFT, i));
        int y = m_stats.at<int>(Point( CC_STAT_TOP, i));
        int w = m_stats.at<int>(Point(CC_STAT_WIDTH, i));
        int h = m_stats.at<int>(Point(CC_STAT_HEIGHT, i));
        auto roi = cv::Rect2f (x,y,w,h);
        if(!check(m_threshold_out.cols, m_threshold_out.rows, roi)) continue;
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = end-m_start;
        auto id = std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
        m_blobs.emplace_back(i, id, roi, area);
        m_blobs.back().update_moments(m_grey);
        cv::Mat window = m_threshold_out(roi);
        m_blobs.back().update_contours(window);
        m_blobs.back().update_points(lablemap[i]);
    }
    std::sort (m_blobs.begin(), m_blobs.end(),[](const blob&a, const blob&b){
        return a.iarea() > b.iarea();
    });
    
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


#if 0


/** ---------------------------------------------------------------
 *      calculates geometric properties of shape x,y
 *
 *      input:
 *        n      number of points
 *        x(.)   shape coordinate point arrays
 *        y(.)
 *        t(.)   skin-thickness array, used only if itype = 2
 *        itype  = 1 ...   integration is over whole area  dx dy
 *               = 2 ...   integration is over skin  area   t ds
 *
 *      output:
 *        xcen,ycen  centroid location
 *        ei11,ei22  principal moments of inertia
 *        apx1,apx2  principal-axis angles
 * ---------------------------------------------------------------*/
bool XFoil::aecalc(int n, double x[], double y[], double t[], int itype, double &area,
                   double &xcen, double &ycen, double &ei11, double &ei22,
                   double &apx1, double &apx2)
{
    double sint, aint, xint, yint, xxint, yyint, xyint;
    double eixx, eiyy, eixy, eisq;
    double dx, dy, xa, ya, ta, ds, da, c1, c2, sgn;
    int ip, io;
    sint  = 0.0;
    aint  = 0.0;
    xint  = 0.0;
    yint  = 0.0;
    xxint = 0.0;
    xyint = 0.0;
    yyint = 0.0;
    
    for (io = 1; io<= n; io++)
    {
        if(io==n) ip = 1;
        else ip = io + 1;
        
        
        dx =  x[io] - x[ip];
        dy =  y[io] - y[ip];
        xa = (x[io] + x[ip])*0.50;
        ya = (y[io] + y[ip])*0.50;
        ta = (t[io] + t[ip])*0.50;
        
        ds = sqrt(dx*dx + dy*dy);
        sint = sint + ds;
        
        if(itype==1)
        {
            //-------- integrate over airfoil cross-section
            da = ya*dx;
            aint  = aint  +       da;
            xint  = xint  + xa   *da;
            yint  = yint  + ya   *da/2.0;
            xxint = xxint + xa*xa*da;
            xyint = xyint + xa*ya*da/2.0;
            yyint = yyint + ya*ya*da/3.0;
        }
        else
        {
            //-------- integrate over skin thickness
            da = ta*ds;
            aint  = aint  +       da;
            xint  = xint  + xa   *da;
            yint  = yint  + ya   *da;
            xxint = xxint + xa*xa*da;
            xyint = xyint + xa*ya*da;
            yyint = yyint + ya*ya*da;
        }
    }
    
    area = aint;
    
    if(aint == 0.0)
    {
        xcen  = 0.0;
        ycen  = 0.0;
        ei11  = 0.0;
        ei22  = 0.0;
        apx1 = 0.0;
        apx2 = atan2(1.0,0.0);
        return false;
    }
    
    //---- calculate centroid location
    xcen = xint/aint;
    ycen = yint/aint;
    
    //---- calculate inertias
    eixx = yyint - (ycen)*(ycen)*aint;
    eixy = xyint - (xcen)*(ycen)*aint;
    eiyy = xxint - (xcen)*(xcen)*aint;
    
    //---- set principal-axis inertias, ei11 is closest to "up-down" bending inertia
    eisq  = 0.25*(eixx - eiyy)*(eixx - eiyy)  + eixy*eixy;
    sgn = sign(1.0 , eiyy-eixx );
    ei11 = 0.5*(eixx + eiyy) - sgn*sqrt(eisq);
    ei22 = 0.5*(eixx + eiyy) + sgn*sqrt(eisq);
    
    if(ei11==0.0 || ei22==0.0)
    {
        //----- vanishing section stiffness
        apx1 = 0.0;
        apx2 = atan2(1.0,0.0);
    }
    else
    {
        if(eisq/((ei11)*(ei22)) < pow((0.001*sint),4.0))
        {
            //----- rotationally-invariant section (circle, square, etc.)
            apx1 = 0.0;
            apx2 = atan2(1.0,0.0);
        }
        else
        {
            //----- normal airfoil section
            c1 = eixy;
            s1 = eixx-ei11;
            
            c2 = eixy;
            s2 = eixx-ei22;
            
            if(fabs(s1)>fabs(s2)) {
                apx1 = atan2(s1,c1);
                apx2 = apx1 + 0.5*PI;
            }
            else{
                apx2 = atan2(s2,c2);
                apx1 = apx2 - 0.5*PI;
            }
            
            if(apx1<-0.5*PI) apx1 = apx1 + PI;
            if(apx1>+0.5*PI) apx1 = apx1 - PI;
            if(apx2<-0.5*PI) apx2 = apx2 + PI;
            if(apx2>+0.5*PI) apx2 = apx2 - PI;
            
        }
    }
    
    return true;
}

#endif

