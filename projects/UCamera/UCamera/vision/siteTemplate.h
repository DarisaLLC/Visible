//
//  siteTemplate.h
//  Camera
//
//  Created by Arman Garakani on 5/12/17.
//

#ifndef siteTemplate_h
#define siteTemplate_h

#include <mutex>
#include <vector>
#include "vision/opencv_utils.hpp"
#include "vision/find.hpp"
#include "vision/colorchannels.h"

#include "core/bitvector.h"
#include "core/rectangle.h"
#include "siteCommon.h"
#include "core/singleton.hpp"
#include "core/rectangle.h"
#include "boost/circular_buffer.hpp"
#include <tuple>

typedef std::pair<float,float> pairf_t;
typedef std::pair<uint32_t,uint32_t> pairui_t;
typedef std::pair<int32_t,int32_t> pairi_t;

typedef std::array<int,3> value_limits_t;

// index: uint64_t, time: double, bgr original image cv::Mat,
// signal used: cv::Matb, mask: cv::Matb,
// Roi: cv::Rect2i, Thresholds: value_limits,
// camera_matrix: cv::Mat, other_matrix: cv::Mat, focal_point (lens_distance): float> timedMat_t;
// model hist: cv::Mat, Optional Hist Display image: cv::Mat

typedef std::tuple<uint64_t,double, cv::Mat> timedMat_t;

// Note: Placeholders for camera matrix, etc
typedef std::tuple<uint64_t,double, cv::Mat, cv::Mat, cv::Mat,  cv::Rect2i, value_limits_t, /*cv::Mat, cv::Mat,*/ float, cv::Mat,cv::Mat> timedRecord_t;

// no_error, just filled
typedef std::tuple<bool,bool,fVector_2d,cv::RotatedRect> tickRecord_t;

class cBPSettings : public value_limits_t
{
public:
    
    cBPSettings (int smin = 30, int vmin = 10, int vmax = 256)
    : value_limits_t {vmin,vmax,smin}
    {
        assert(update());
    }
    
    cBPSettings (const value_limits_t& params)
    : value_limits_t(params)
    {
        assert(update());
    }
    
    inline int vmax () const { return at(2); }
    inline void vmax (const int vv) const { m_vmax = vv; }
    
    inline int vmin () const { return at(1); }
    inline void vmin (const int vv) const { m_vmin = vv; }
    
    inline int smin () const { return at(0); }
    inline void smin (const int vv) const { m_smin = vv; }
    
    const std::pair<Scalar,Scalar>& limits () const { return m_limits; }
    
    bool update () const
    {
        bool valid = vmax() < 0 || vmax() > 256 || vmin() < 0 || vmin() >= vmax() || smin() < 0 || smin() >= 256;
        if (! valid) return valid;
        
        m_limits.first =  Scalar(0, smin(), std::min(vmin(),vmax()));
        m_limits.second = Scalar(180, 256, std::max(vmin(), vmax()));
        return valid;
    }

    value_limits_t dump () const
    {
        value_limits_t b (*this);
        return b;
    }
    
private:
    mutable int m_vmin, m_vmax, m_smin;
    mutable std::pair<Scalar,Scalar> m_limits;
    
};


class siteTemplate : public boost::circular_buffer<timedMat_t>
{
public:

    
    
    enum haves:int {na=0,str=na+1,base=str+1,full=base+1,
        com=full+1,lens=com+1,
        view=lens+1,image=view+1,fpoi=image+1,lastModel=fpoi+1,num_haves=lastModel+1};
    
    enum mode:int {avg=0,hum=avg+1,geo=hum+1,red=geo+1,mhue=red+1,num_modes=mhue+1};
    
    enum state:int {started=0,stepping=1,stopped=2,filling=3,error=4,uninialized=-1};
    
    enum match:int {correlation=0, cbp = 1};
    
    SINGLETONPTR(siteTemplate)
    
    siteTemplate();
    
    const std::string& getTitle ();
    void setTitle (const std::string&);
    
    void setLensPosition (const float);
    void unsetLensPosition ();
    float getLensPosition ();

    bool colorStatistics (const cv::Mat&, colorStats_t&);
    
    // Setup tracking
    bool setNormalizedTrainingRect (float x, float y, float width, float height);
    timedRecord_t trainCbp (const timedMat_t& bgr_original );
    

    const cv::Mat& getBaseTemplate ();
    const mode& getBaseMode () const;
    
  
    uint32_t getWidth () const;
    uint32_t getHeight () const;
    bool have (enum haves) const;
    const site_eval& eval () const;
    
    void resetImageSize (float width, float height);
    const uint64_t& count ();
    
    // Operations
    bool stop();
    bool start (match);
    
    // Convert the image,
    // take one from the ring buffer
    // process
    // and put in the ring buffer
    tickRecord_t step (const timedMat_t&);
    
    bool   isRunning();
    bool   hasStopped();
    bool   hasStarted();
    bool   isUninitialized();
    bool   isTrained ();
    
private:

    tickRecord_t step_correlation (const timedMat_t&);
    tickRecord_t step_cbp (const timedMat_t&);
    
    const state current_state () const;
    void set_state (state st) const;
    bool expand_rectangle (const fRect& rr, fRect& dd, pairi_t& slide);
    pairi_t mReduction;
    pairi_t mSlide;
    
   void prepareMultiChannel(const cv::Mat& image, cv::Mat& dst);
    
    mutable   cv::Mat mCacheMultiChannel;
    mutable match mMatch;
    
    mutable   cv::Mat mBaseTemplate;
    mutable   cv::Mat mBaseImage;
    mutable cv::Mat mChannelAccumulator;
    
    mutable std::vector<cv::Mat> mChannels;
    
    vector<cv::Mat> mObjpyr;
    
    cv::Size2f mViewSize;
    fRect mFixed;
    fRect mMoving;
    
    iRect mViewRect;
    mode mMode;
    uint64_t mCount;
    cv::Rect mCenteredRect;
  
    float mLensPosition;
    std::string mName;
    mutable bitvector mRepo;
    mutable std::mutex mTrackingMutex;
    
    // Simplest correlation tracker
    site_eval  mLast;
    mutable state mState;
    std::vector<findLocation> m_finds;
    
    // Camshift tracker
    mutable cv::Mat mModelHist;
    mutable cv::Mat mModelHistImage;
    
    mutable cv::Mat mHSV;
    mutable cv::Mat mHue;
    mutable cv::Mat mMask;
    mutable cv::Mat mBackProj;
    cBPSettings mbpParams;
    timedRecord_t mLastRecord;
    
    mutable std::vector<timedRecord_t> mTimeImages;
    
};
#endif /* siteTemplate_h */
