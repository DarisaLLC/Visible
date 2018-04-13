#ifndef __ALGO_LIF__
#define __ALGO_LIF__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <deque>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
#include "core/singleton.hpp"
#include "async_producer.h"
#include "signaler.h"
#include "sm_producer.h"
#include "cinder_xchg.hpp"
#include "vision/histo.h"
#include "vision/opencv_utils.hpp"
#include "opencv2/video/tracking.hpp"
#include "getLuminanceAlgo.hpp"

using namespace cv;



struct OpticalFlowFarnebackRunner
{
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    std::shared_ptr<timed_mat_vec_t> operator()(std::vector<cv::Mat>& frames)
    {
        std::shared_ptr<timed_mat_vec_t> results ( new timed_mat_vec_t () );
        if (frames.size () < 2) return results;
        cv::Mat cflow;
        int ii = 0;
        std::string imgpath = "/Users/arman/tmp/oflow/fback" + toString(ii) + ".png";
        cvtColor(frames[ii], cflow, COLOR_GRAY2BGR);
        cv::imwrite(imgpath, frames[ii]);
        
        results->clear();
        for (++ii; ii < frames.size(); ii++)
        {
            cv::UMat uflow;
            timed_mat_t res;
            index_time_t ti;
            ti.first = ii;
            res.first = ti;
            calcOpticalFlowFarneback(frames[ii-1], frames[ii], uflow, 0.5, 1, 15, 3, 5, 1.2, cv::OPTFLOW_FARNEBACK_GAUSSIAN);
            uflow.copyTo(res.second);
            cvtColor(frames[ii], cflow, COLOR_GRAY2BGR);
            drawOptFlowMap(res.second, cflow, 16, 1.5, Scalar(0, 255, 0));
            std::string imgpath = "/Users/arman/tmp/oflow/fback" + toString(ii) + ".png";
            cv::imwrite(imgpath, frames[ii]);
            std::cout << "wrote Out " << imgpath << std::endl;
            results->emplace_back(res);
            
        }
        return results;
    }
    
    
    
    static void drawOptFlowMap(const cv::Mat& flow, cv::Mat& cflowmap, int step,
                               double, const Scalar& color)
    {
        for(int y = 0; y < cflowmap.rows; y += step)
            for(int x = 0; x < cflowmap.cols; x += step)
            {
                const Point2f& fxy = flow.at<Point2f>(y, x);
                line(cflowmap, cv::Point(x,y), cv::Point(cvRound(x+fxy.x), cvRound(y+fxy.y)),
                     color);
                circle(cflowmap, cv::Point(x,y), 2, color, -1);
            }
    }
    
};

// For base classing
class LifSignaler : public base_signaler
{
    virtual std::string
    getName () const { return "Lif Signaler"; }
};


class lif_processor : public LifSignaler
{
public:
    
    typedef void (sig_cb_content_loaded) ();
    typedef void (sig_cb_flu_stats_available) ();
    typedef void (sig_cb_frame_loaded) (int&, double&);
    
    typedef void (sig_cb_sm1d_available) (int&);
    typedef void (sig_cb_sm1dmed_available) (int&,int&);
    typedef std::vector<roiWindow<P8U>> channel_images_t;
    
    lif_processor ()
    {
        // Signals we provide
        signal_content_loaded = createSignal<lif_processor::sig_cb_content_loaded>();
        signal_flu_available = createSignal<lif_processor::sig_cb_flu_stats_available>();
        signal_frame_loaded = createSignal<lif_processor::sig_cb_frame_loaded>();
        signal_sm1d_available = createSignal<lif_processor::sig_cb_sm1d_available>();
        signal_sm1dmed_available = createSignal<lif_processor::sig_cb_sm1dmed_available>();
        
        // Signals we support
        m_sm = std::shared_ptr<sm_producer> ( new sm_producer () );
        std::function<void ()> sm_content_loaded_cb = std::bind (&lif_processor::sm_content_loaded, this);
        boost::signals2::connection ml_connection = m_sm->registerCallback(sm_content_loaded_cb);
        
    }
    
    const smProducerRef sm () const { return m_sm; }
    std::shared_ptr<sm_filter> smFilterRef () { return m_smfilterRef; }
    
    
    void load (const std::shared_ptr<qTimeFrameCache>& frames,const std::vector<std::string>& names)
    {
        create_named_tracks(names);
        load_channels_from_images(frames);
    }
    
    
    std::shared_ptr<vectorOfnamedTrackOfdouble_t> run_flu_statistics ()
    {
        std::vector<timed_double_vec_t> cts (2);
        std::vector<std::thread> threads(2);
        for (auto tt = 0; tt < 2; tt++)
        {
            threads[tt] = std::thread(IntensityStatisticsRunner(),
                                      std::ref(m_channel_images[tt]), std::ref(cts[tt]));
        }
        std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
        
        for (auto tt = 0; tt < 2; tt++)
            m_tracksRef->at(tt).second = cts[tt];
        
        // Call the content loaded cb if any
        if (signal_flu_available && signal_flu_available->num_slots() > 0)
            signal_flu_available->operator()();
        
        return m_tracksRef;
    }
    
    // Run to get Entropies and Median Level Set
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  run_pci ()
    {
        channel_images_t c2 = m_channel_images[2];
        auto sp =  sm();
        sp->load_images (c2);
        std::packaged_task<bool()> task([sp](){ return sp->operator()(0, 0);}); // wrap the function
        std::future<bool>  future_ss = task.get_future();  // get a future
        std::thread(std::move(task)).join(); // Finish on a thread
        if (future_ss.get())
        {
            // Signal we are done with ACI
            static int dummy;
            if (signal_sm1d_available && signal_sm1d_available->num_slots() > 0)
                signal_sm1d_available->operator()(dummy);
            
            m_entropies = sp->shannonProjection ();
            m_smat = sp->similarityMatrix();
            m_smfilterRef = std::make_shared<sm_filter> (m_entropies, m_smat);
            
            update ();
        }
        return m_pci_tracksRef;
    }
    
    const std::vector<Rectf>& rois () const { return m_rois; }
    const namedTrackOfdouble_t& similarity_track () const { return m_tracksRef->at(2); }
    const std::shared_ptr<vectorOfnamedTrackOfdouble_t>& all_track () const { return m_tracksRef; }
    
    // Update. Called also when cutoff offset has changed
    void update ()
    {
        if(m_pci_tracksRef && !m_pci_tracksRef->empty() && m_smfilterRef && m_smfilterRef->isValid())
            entropiesToTracks(m_pci_tracksRef->at(0));
    }
    
protected:
    boost::signals2::signal<lif_processor::sig_cb_content_loaded>* signal_content_loaded;
    boost::signals2::signal<lif_processor::sig_cb_flu_stats_available>* signal_flu_available;
    boost::signals2::signal<lif_processor::sig_cb_frame_loaded>* signal_frame_loaded;
    boost::signals2::signal<lif_processor::sig_cb_sm1d_available>* signal_sm1d_available;
    boost::signals2::signal<lif_processor::sig_cb_sm1dmed_available>* signal_sm1dmed_available;
    
private:
    
    void sm_content_loaded ()
    {
        std::cout << "----------> sm content_loaded\n";
    }
    // Assumes LIF data -- use multiple window.
    void load_channels_from_images (const std::shared_ptr<qTimeFrameCache>& frames)
    {
        int64_t fn = 0;
        m_channel_images.clear();
        m_channel_images.resize (3);
        m_rois.resize (0);
        std::vector<std::string> names = {"Red", "Green","Blue"};
        
        while (frames->checkFrame(fn))
        {
            auto su8 = frames->getFrame(fn++);
            auto m3 = svl::NewRefMultiFromSurface (su8, names, fn);
            for (auto cc = 0; cc < m3->planes(); cc++)
                m_channel_images[cc].emplace_back(m3->plane(cc));
            
            // Assumption: all have the same 3 channel concatenated structure
            // Fetch it only once
            if (m_rois.empty())
            {
                for (auto cc = 0; cc < m3->planes(); cc++)
                {
                    const iRect& ir = m3->roi(cc);
                    m_rois.emplace_back(vec2(ir.ul().first, ir.ul().second), vec2(ir.lr().first, ir.lr().second));
                }
            }
        }
        
        // Call the content loaded cb if any
        if (signal_content_loaded && signal_content_loaded->num_slots() > 0)
            signal_content_loaded->operator()();
        
    }
    
    
    
    // Note tracks contained timed data.
    void entropiesToTracks (namedTrackOfdouble_t& track)
    {
        if (! m_smfilterRef->median_levelset_similarities  ()) return;
        
        
        track.second.clear();
        
        auto bee = m_smfilterRef->entropies().begin();
        auto mee = m_smfilterRef->filtered().begin();
        for (auto ss = 0; bee != m_smfilterRef->entropies().end() && ss < frame_count(); ss++, bee++, mee++)
        {
            index_time_t ti;
            ti.first = ss;
            timed_double_t res;
            res.first = ti;
            res.second = *mee;
            track.second.emplace_back(res);
        }
        
        // Signal we are done with median level set
        static int dummy;
        if (signal_sm1dmed_available && signal_sm1dmed_available->num_slots() > 0)
            signal_sm1dmed_available->operator()(dummy, dummy);
    }
    
    size_t frame_count () const
    {
        if (m_channel_images[0].size() == m_channel_images[1].size() && m_channel_images[1].size() == m_channel_images[2].size())
            return m_channel_images[0].size();
        else return 0;
    }
    
    // @note Specific to ID Lab Lif Files
    void create_named_tracks (const std::vector<std::string>& names)
    {
        m_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
        m_pci_tracksRef = std::make_shared<vectorOfnamedTrackOfdouble_t> ();
        
        m_tracksRef->resize (2);
        m_pci_tracksRef->resize (1);
        for (auto tt = 0; tt < names.size()-1; tt++)
            m_tracksRef->at(tt).first = names[tt];
        m_pci_tracksRef->at(0).first = names[2];
        
    }
    
    std::shared_ptr<timed_mat_vec_t> compute_oflow_threaded ()
    {
        return OpticalFlowFarnebackRunner()(std::ref(m_channel2_mats));
    }
    
    
    void loadImagesToMats (const sm_producer::images_vector_t& images, std::vector<cv::Mat>& mts)
    {
        mts.resize(0);
        vector<roiWindow<P8U> >::const_iterator vitr = images.begin();
        do
        {
            mts.emplace_back(vitr->height(), vitr->width(), CV_8UC(1), vitr->pelPointer(0,0), size_t(vitr->rowUpdate()));
        }
        while (++vitr != images.end());
    }
    
    
    
private:
    deque<double> m_entropies;
    deque<deque<double>> m_smat;
    smProducerRef m_sm;
    channel_images_t m_images;
    std::vector<channel_images_t> m_channel_images;
    std::vector<cv::Mat> m_channel2_mats;
    
    std::vector<Rectf> m_rois;
    Rectf m_all;
    std::shared_ptr<sm_filter> m_smfilterRef;
    std::deque<double> m_medianLevel;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_pci_tracksRef;
    std::shared_ptr<vectorOfnamedTrackOfdouble_t>  m_tracksRef;
};

template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_content_loaded>&);
//template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_flu_stats_available>&);
template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_frame_loaded>&);
template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_sm1d_available>&);
template boost::signals2::connection lif_processor::registerCallback(const std::function<lif_processor::sig_cb_sm1dmed_available>&);

#endif
