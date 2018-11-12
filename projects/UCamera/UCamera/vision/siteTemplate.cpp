//
//  siteTemplate.cpp
//  Camera
//
//  Created by Arman Garakani on 5/12/17.
//

#include "siteTemplate.h"

static double eps_s=2.2204e-16;
static float lens_not_set = -1.0f;




bool siteTemplate::colorStatistics (const cv::Mat&src, colorStats_t& cs)
{
    cs.n = 0;
    if (src.channels() < 3) return false;
    cv::Scalar means,stddev;
    cv::meanStdDev(src,means, stddev);
    cs.n = src.cols * src.rows;
    cs.means[0] = means.val[0];
    cs.means[1] = means.val[1];
    cs.means[2] = means.val[2];
    
    cs.stds[0] = stddev.val[0];
    cs.stds[1] = stddev.val[1];
    cs.stds[2] = stddev.val[2];

    return true;
}

siteTemplate::siteTemplate(): mMode (mode::geo), mMatch (siteTemplate::match::cbp), boost::circular_buffer<timedMat_t>(2)
{
    mRepo.resize (16);
    mReduction.first = mReduction.second = 8;
    mSlide.first = mSlide.second = 4;
}

const uint64_t& siteTemplate::count ()
{
    return mCount;
}

const siteTemplate::state siteTemplate::current_state () const
{
    std::unique_lock <std::mutex> lock(mTrackingMutex, std::try_to_lock);
    return mState;
}
void siteTemplate::set_state (state st) const
{
    std::unique_lock <std::mutex> lock(mTrackingMutex, std::try_to_lock);
    mState = st;
}

const site_eval& siteTemplate::eval () const
{
    std::lock_guard<std::mutex> lock (mTrackingMutex);
    return mLast;
}


// Operations
bool siteTemplate::stop()
{
    set_state(siteTemplate::state::stopped);
    return current_state() == siteTemplate::state::stopped;
}

bool siteTemplate::start (match which)
{
    mMatch = which;
    
    auto ok = have(haves::fpoi);
    if (!ok) return ok;
    ok = current_state() == siteTemplate::state::uninialized || current_state() == siteTemplate::state::stopped;
    if (!ok) return false;
    
    m_finds.clear();
    set_state(siteTemplate::state::started);
    return true;
    // set the state to start
    // clear the counters
}

tickRecord_t siteTemplate::step (const timedMat_t& captured)
{
    switch(mMatch)
    {
        case match::cbp:
            return step_cbp(captured);
        case match::correlation:
            return step_correlation(captured);
    }
}


tickRecord_t siteTemplate::step_cbp (const timedMat_t& captured)
{
    static cv::RotatedRect s_vr;
    if (! have(haves::lastModel)) return make_tuple(false,false,fVector_2d(),s_vr);
    
    if (current_state() != state::stepping &&
        current_state() != state::started &&
        current_state() != state::filling ) return make_tuple(false,false,fVector_2d(), s_vr);
    
    std::pair<bool,fVector_2d> mini_pose;
    mini_pose.first = false;
    
    uint64_t frame_index;
    double frame_time;
    cv::Mat capture;
    std::tie(frame_index, frame_time, capture) = captured;
    cv::resize(capture, capture, mViewSize);
    
    
    // index: uint64_t, time: double, bgr original image cv::Mat,
    // signal used: cv::Matb,
    // mask: cv::Matb, N - 6
    // Roi: cv::Rect2i, Thresholds: value_limits, N - 5, N - 4
    // focal_point (lens_distance): float, N - 3
    // Model Hist : cv::Mat, Optional Hist Display image: cv::Mat N - 2, N - 1
    static size_t recordN = std::tuple_size<timedRecord_t>::value;
    cv::Mat model = std::get<std::tuple_size<timedRecord_t>::value-2>(mLastRecord);
    cv::Mat mask = std::get<std::tuple_size<timedRecord_t>::value-6>(mLastRecord);
    value_limits_t vl = std::get<std::tuple_size<timedRecord_t>::value-4>(mLastRecord);
    cBPSettings thresh (vl);
    cv::Rect2i trackWindow = std::get<std::tuple_size<timedRecord_t>::value-5>(mLastRecord);

    cv::Mat hsv, hue, backproj;
    
    cvtColor(capture, hsv, COLOR_BGR2HSV);
    cv::inRange(hsv, thresh.limits().first, thresh.limits().second, mask);
    
    // Copy the in range image very shallowly
    int ch[] = {0, 0};
    hue.create(hsv.size(), hsv.depth());
    mixChannels(&hsv, 1, &hue, 1, ch, 1);
    
    int hsize = 16;
    float hranges[] = {0,180};
    const float* phranges = hranges;
    
    calcBackProject(&hue, 1, 0, model, backproj, &phranges);
    backproj &= mask;
    RotatedRect trackBox = CamShift(backproj, trackWindow,TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
    auto ctr = trackBox.center;
    fVector_2d center (ctr.x, ctr.y);
    
    if( trackWindow.area() <= 1 )
    {
        int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
        trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                           trackWindow.x + r, trackWindow.y + r) & Rect(0, 0, cols, rows);
    }

    return make_tuple(true, true,center, trackBox);
}

/*
 * Returns best match at full resolution
 */

 tickRecord_t siteTemplate::step_correlation (const timedMat_t& captured)
{
    static cv::RotatedRect s_vr;
    
    if (! have(haves::fpoi)) return make_tuple(false,false,fVector_2d(), s_vr);
    
    
    assert (mMoving != fRect () && mFixed != fRect () );
    
    if (current_state() != state::stepping &&
        current_state() != state::started &&
        current_state() != state::filling ) return make_tuple(false,false,fVector_2d(), s_vr);
    
    std::pair<bool,fVector_2d> mini_pose;
    mini_pose.first = false;
    
    uint64_t frame_index;
    double frame_time;
    cv::Mat capture;
    std::tie(frame_index, frame_time, capture) = captured;
    
    // Prepare Pipe
    switch (size())
    {
        case 0:
        {
            cv::Mat newImage;
            prepareMultiChannel(capture, newImage);
            push_back(std::make_tuple(frame_index, frame_time, newImage));
            std::cout << "E F " << size() << std::endl;
            return make_tuple(true,true,fVector_2d(), s_vr);
        }
        case 2:
            pop_front();
            break;
        case 1:
            break;
        default:
            std::cout << "Circular Buffer Overload" << std::endl;
            assert(false);
            break;
    }
    // Now put in the new one
    assert(size() == 1 && have(haves::fpoi));
    cv::Mat newImage;
    prepareMultiChannel(capture, newImage);
    
    std::cout << __LINE__ << slidingSpan(mMoving, mFixed) << std::endl;
    findLocation res =  find (std::get<2>(front()), mFixed, newImage, mMoving);
    std::cout << setprecision(3) << res << std::endl;

    fVector_2d center (res.position().x()+res.box().center().first,
                       res.position().y()+res.box().center().second);
    
    center.x(center.x()*mReduction.first);
    center.y(center.y()*mReduction.second);
    
    // fixed is setup by find
    mFixed = res.box();
    
    expand_rectangle(mFixed, mMoving, mSlide);


    
    push_back(std::make_tuple(frame_index, frame_time, newImage));
    
    return make_tuple(true,true,center, s_vr);
    
    // set the state to stepping
    // expect a new image. Run the current template
    // Find max correlation peak
    // train the template at the peak and push
    // update stats
    // if we are above confusion, stop. If we are between accept and confusion, continue. if we are less, go to the next model
    // done
}

bool   siteTemplate::isRunning()
{
    return current_state() == siteTemplate::state::stepping;
}
bool   siteTemplate::hasStopped()
{
    return current_state() == siteTemplate::state::stopped;
}
bool   siteTemplate::hasStarted()
{
    return current_state() == siteTemplate::state::started;
}
bool   siteTemplate::isUninitialized()
{
    return current_state() == siteTemplate::state::uninialized;
}

bool  siteTemplate::expand_rectangle (const fRect& rr, fRect& dd, pairi_t& slide)
{
    bool padcheck(false);
    fRect moving = rr.trim(-slide.first, -slide.first, -slide.second, -slide.second, padcheck);
    dd = moving;
    return padcheck;
}

const std::string& siteTemplate::getTitle ()
{
    static std::string null_string;
    
    if (mRepo[haves::str]) return mName;
    else return null_string;
}


void siteTemplate::setTitle (const std::string& title)
{
    mName = title;
    mRepo[haves::str] = true;
}

const siteTemplate::mode& siteTemplate::getBaseMode () const { return mMode; }



bool siteTemplate::setNormalizedTrainingRect (float x, float y, float width, float height)
{
 
    assert (mViewSize.width > 0 && mViewSize.height > 0);
    
    mFixed = fRect(x * mViewSize.width, y * mViewSize.height,width * mViewSize.width, height * mViewSize.height);
    mCenteredRect = cv::Rect(x * mViewSize.width, y * mViewSize.height,width * mViewSize.width, height * mViewSize.height);
    
    if (mMatch == match::correlation)
    {
        expand_rectangle(mFixed, mMoving, mSlide);
        std::cout << __LINE__ << " " << mFixed << "~" << mMoving << " ::> " << slidingSpan(mMoving, mFixed) << std::endl;
        mBaseTemplate = mBaseImage.clone ();
    }
    mRepo[haves::fpoi] = true;
    return true;
    
}

timedRecord_t siteTemplate::trainCbp (const timedMat_t& captured)
{
    cv::Mat hsv;
    cv::Mat hue;
    cv::Mat mask;
    cv::Mat hist;
    cv::Mat histimg = Mat::zeros(200, 320, CV_8UC3);
    cv::Mat capture;
    
    uint64_t frame_index;
    double frame_time;
    std::tie(frame_index, frame_time, capture) = captured;
    mViewSize.width = capture.cols;
    mViewSize.height = capture.rows;
    
    setNormalizedTrainingRect(0.45, .45, .10, 0.10);
    
    cvtColor(capture, hsv, COLOR_BGR2HSV);
    cv::inRange(hsv, mbpParams.limits().first, mbpParams.limits().second, mask);
    
    // Copy the in range image very shallowly
    int ch[] = {0, 0};
    hue.create(hsv.size(), hsv.depth());
    mixChannels(&hsv, 1, &hue, 1, ch, 1);
    
    int hsize = 16;
    float hranges[] = {0,180};
    const float* phranges = hranges;
    
    // set up CAMShift search properties once
    Mat roi(hue, mCenteredRect), maskroi(mask, mCenteredRect);
    calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
    normalize(hist, hist, 0, 255, NORM_MINMAX);
    
    histimg = Scalar::all(0);
    int binW = histimg.cols / hsize;
    Mat buf(1, hsize, CV_8UC3);
    for( int i = 0; i < hsize; i++ )
        buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180./hsize), 255, 255);
    cvtColor(buf, buf, COLOR_HSV2BGR);
    
    for( int i = 0; i < hsize; i++ )
    {
        int val = saturate_cast<int>(hist.at<float>(i)*histimg.rows/255);
        rectangle( histimg, Point(i*binW,histimg.rows),
                  Point((i+1)*binW,histimg.rows - val),
                  Scalar(buf.at<Vec3b>(i)), -1, 8 );
    }

    // index: uint64_t, time: double, bgr original image cv::Mat,
    // signal_used: cv::Mat, mask: cv::Mat,
    // Roi: cv::Rect2i, Thresholds: value_limits,
    // camera_matrix: cv::Mat, other_matrix: cv::Mat, focal_point (lens_distance): float> timedMat_t;
    // hist: cv::Mat, Optional Hist Display image: cv::Mat
    float lp = getLensPosition();
    mLastRecord = std::make_tuple(frame_index, frame_time, capture, hue, mask, mCenteredRect, mbpParams.dump(), lp, hist, histimg);
    mRepo[haves::lastModel] = true;
    
    return mLastRecord;
}

/*
 * Create the single channel base template for a few options
 */

void siteTemplate::prepareMultiChannel(const cv::Mat& image, cv::Mat& dst)
{
    cv::resize (image, mCacheMultiChannel, mViewSize);
    
    switch (mMode)
    {
        case avg:
        {
            // Convert to 32F.
            mCacheMultiChannel.convertTo(mCacheMultiChannel, CV_32F);
            cv::split (mCacheMultiChannel, mChannels);
            
            mChannels[0].copyTo(mChannelAccumulator);
            mChannelAccumulator += mChannels[1];
            mChannelAccumulator += mChannels[2];
            mChannelAccumulator += eps_s;
            mChannelAccumulator = mChannelAccumulator / 3.0f;
            mChannelAccumulator.convertTo(dst, CV_8U);
            break;
        }
        case hum:
        {
            cv::cvtColor(mCacheMultiChannel, dst, CV_RGB2GRAY);
            break;
        }
        case geo:
        {
            // Convert to 32F.
            mCacheMultiChannel.convertTo(mCacheMultiChannel, CV_32F);
            cv::split (mCacheMultiChannel, mChannels);
            
            mChannels[0].copyTo(mChannelAccumulator);
            cv::accumulateProduct(mChannelAccumulator, mChannels[1], mChannelAccumulator);
            cv::accumulateProduct(mChannelAccumulator, mChannels[2], mChannelAccumulator);
            cv::pow(mChannelAccumulator, 1/3.0, mChannelAccumulator);
            mChannelAccumulator.convertTo(dst, CV_8U);
            break;
        }
        case red:
        {
            cv::split (mCacheMultiChannel, mChannels);
            dst = mChannels[0];
            break;
        }
        case mhue:
        {
            cv::Mat hsv;
            cv::cvtColor(mCacheMultiChannel, hsv, CV_RGB2HSV);
            cv::split (hsv, mChannels);
            dst = mChannels[0];
            break;
        }
        default:
            assert(0);
            break;
            
    }
    
    // Now create the Pyramid
    
    
}


void siteTemplate::setLensPosition (const float lp)
{
    mLensPosition = lp;
    mRepo[haves::lens] = true;
}

void siteTemplate::unsetLensPosition ()
{
    mLensPosition = lens_not_set;
}

float siteTemplate::getLensPosition ()
{
    if(mRepo[haves::lens])
        return mLensPosition;
    else return lens_not_set;
}

// Reduces by reduction factors
// Setting ViewSize, prepares the gaussian template (5 pixels trimmed in all sides)

void siteTemplate::resetImageSize (float full_width, float full_height)
{
//    std::cout << __LINE__ << " " << full_width << "~" << full_height << std::endl;

    auto reduction = mReduction;
    if (mMatch != match::correlation)
    {
        reduction.first = reduction.second = 1;
    }
    uint32_t width = full_width/reduction.first;
    uint32_t height = full_height/reduction.second;
    
    if (mViewSize.width != width && mViewSize.height != height)
    {
     //   std::cout << __LINE__ << " " << full_width << "~" << full_height << std::endl;
        mViewSize.width = width;
        mViewSize.height = height;
        mViewRect.width(width);
        mViewRect.height(height);
    }
    
}

uint32_t siteTemplate::getWidth () const
{
    if (mRepo[haves::image])
        return mViewSize.width;
    else return 0;
}
uint32_t siteTemplate::getHeight () const
{
    if (mRepo[haves::image])
        return mViewSize.height;
    else return 0;
}

bool siteTemplate::have (enum haves h) const
{
    return mRepo[h];
}



#if 0
site_eval siteTemplate::findPointOfInterest ()
{
    static site_eval null_eval;
    
    if (!(mRepo[haves::image] && mRepo[haves::fpoi]))
    {
        return null_eval;
    }
    cv::Mat space;
    site_eval se;
    se.score = crossMatch (mBaseImage, mPoItemplate, space, true);
    std::cout << setprecision(3) << se.score * 1000 << std::endl;
    return se;
}


site_eval siteTemplate::checkLuminance ()
{
    site_eval ee;
    ee.valid = false;
    ee.valid = have(haves::base) && have(haves::gauss);
    auto medp = medianPoint (mBaseTemplate);
    ee.com_x = medp.first;
    ee.com_y = medp.second;
    return ee;
}

site_eval siteTemplate::checkLuminanceShape ()
{
    site_eval ee;
    ee.valid = false;
    ee.valid = have(haves::base) && have(haves::gauss);
    auto medp = medianPoint (mBaseTemplate);
    ee.com_x = medp.first;
    ee.com_y = medp.second;
    return ee;
}

site_eval siteTemplate::checkRedPeaks ()
{
    site_eval ee;
    cv::Mat space;
    ee.valid = false;
    ee.valid = have(haves::image) && have(haves::gauss);
    std::vector<uint32_t> bgr_max_counts, bgr_min_counts, bgr_med_counts;
    uint32_t bgr_equal_counts;
    float np = mBaseImage.rows * mBaseImage.cols;
    countRedMaxBGR(mBaseImage, bgr_max_counts, bgr_med_counts, bgr_min_counts, bgr_equal_counts);
    ee.score = bgr_max_counts[0] / np;
    return ee;
}
#endif

