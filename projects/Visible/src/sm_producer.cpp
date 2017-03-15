#include "sm_producer.h"
#include "sm_producer_impl.h"
#include "self_similarity.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>
#include "app_utils.hpp"
#include "cinder/qtime/Quicktime.h"
#include "cinder_xchg.hpp"
#include "core/simple_timing.hpp"
#include "core/stl_utils.hpp"
#include "vision/opencv_utils.hpp"
#include "vision/histo.h"

#include "qtimeAvfLink.h"

using namespace std::chrono;

using namespace boost;

using namespace ci;

using namespace svl;

namespace anonymous
{
    bool is_anaonymous_name (const boost::filesystem::path& pp, size_t check_size = 36)
    {
        string extension = pp.extension().string();
        return extension.length() == 0 && pp.filename().string().length() == check_size;
    }
}

sm_producer::sm_producer (bool auto_on_off)
{
    
    _impl = std::shared_ptr<sm_producer::spImpl> (new sm_producer::spImpl);
    if (auto_on_off)
        set_auto_run_on();
    
}

std::vector<roiWindow<P8U>>& sm_producer::images () const
{
    return _impl->images ();
}

bool sm_producer::load_content_file (const std::string& movie_fqfn)
{

    if (_impl) return _impl->loadMovie(movie_fqfn);
    return false;
}

bool sm_producer::load_image_directory (const std::string& dir_fqfn)
{
    if ( !boost::filesystem::exists( dir_fqfn ) ) return false;
    if ( !boost::filesystem::is_directory( dir_fqfn ) ) return false;
    
 
    if (_impl) return _impl->loadImageDirectory(dir_fqfn) > 0;
    return false;
}

bool sm_producer::operator() (int start_frame, int frames ) const
{
    if (_impl)
    {
        auto fcnt = (frames == 0) ? _impl-> _frameCount : frames;
        std::future<bool> bright = std::async(std::launch::async, &sm_producer::spImpl::generate_ssm, _impl, start_frame, fcnt);
        bright.wait();
        return bright.get();
    }
    return false;
}

void sm_producer::load_images(const images_vector_t &images)
{
    if (_impl) _impl->loadImages (images);
}

template<typename T> boost::signals2::connection
sm_producer::registerCallback (const std::function<T> & callback)
{
    return _impl->registerCallback  (callback);
}

template<typename T> bool
sm_producer::providesCallback () const
{
    return _impl->providesCallback<T> ();
}

bool sm_producer::set_auto_run_on() const { return (_impl) ? _impl->set_auto_run_on() : false; }
bool sm_producer::set_auto_run_off() const { return (_impl) ? _impl->set_auto_run_off() : false; }
int sm_producer::frames_in_content() const { return (_impl) ? (int) _impl->frame_count(): -1; }

//grabber_error sm_producer::last_error () const { if (_impl) return _impl->last_error(); return grabber_error::Unknown; }

const sm_producer::sMatrix_t& sm_producer::similarityMatrix () const { return _impl->m_SMatrix; }

const sm_producer::sMatrixProjection_t& sm_producer::meanProjection () const { assert(false); }

const sm_producer::sMatrixProjection_t& sm_producer::shannonProjection () const { return _impl->m_entropies; }

void sm_producer::spImpl::asset_reader_done_cb ()
{
    std::cout << " asset reader Done" << std::endl;
}


/*
* Load all the frames
*/
int sm_producer::spImpl::loadImageDirectory( const std::string& imageDir,  const std::vector<std::string>& supported_extensions)
{
    using namespace ci::fs;
    
    m_framePaths.clear();
    
    // make a list of all image files in the directory
    filesystem::directory_iterator end_itr;
    for( filesystem::directory_iterator i( imageDir ); i != end_itr; ++i )
    {
        std::unique_lock <std::mutex> lock(m_mutex, std::try_to_lock);
        // skip if not a file
        if( !filesystem::is_regular_file( i->status() ) ) continue;
        
        std::cout << "Checking " << i->path().string();
        
        if (std::find( supported_extensions.begin(), supported_extensions.end(), i->path().extension()  ) != supported_extensions.end())
        {
            std::cout << "  Extension matches " << std::endl;
            m_framePaths.push_back( i->path().string() );
        }
        else if (anonymous::is_anaonymous_name(i->path()))
        {
            std::cout << "  Anonymous rule matches " << std::endl;
            m_framePaths.push_back( i->path().string() );
        }
        else
        {
            std::cout << "Skipped " << i->path().string() << std::endl;
            continue;

        }
        
    }
    
    if (m_framePaths.empty()) return false;
    
    m_loaded_ref.resize(0);
    
    // Read data into memory buffer
    for (boost::filesystem::path& pp : m_framePaths)
    {
        std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );
        
        // OpenCv imread determines format from the content
        // -1 returns the loaded as is
        auto ipair = svl::image_io_read_surface (pp);
        roiWindow<P8U> rw;
        if (ipair.first)
        {
            rw = NewRedFromSurface(ipair.first);
        }
        else if (ipair.second)
        {
            rw = NewFromChannel(*ipair.second, 0);
        }
        else
            continue;
        
        auto mean = histoStats::mean(rw);
        std::cout << mean << std::endl;
        
        
        m_loaded_ref.emplace_back(rw);

    }

    if (m_loaded_ref.empty()) return -1;

    _frameCount = m_loaded_ref.size ();
    if (m_auto_run) generate_ssm (0,0);
    return (int) _frameCount;
}

/*
 * Load all the frames in the movie
 */
int sm_producer::spImpl::loadMovie( const std::string& movieFile )
{
    std::unique_lock <std::mutex> lock(m_mutex);

    {
        ScopeTimer timeit("avReader");
        m_assetReader = std::make_shared<avcc::avReader>(movieFile, false);
        m_assetReader->setUserDoneCallBack(std::bind(&sm_producer::spImpl::asset_reader_done_cb, this));
    }
    
    bool m_valid = m_assetReader->isValid();
    if (m_valid)
    {
        m_assetReader->info().printout();
        m_assetReader->run();
        m_qtime_cache_ref = qTimeFrameCache::create (m_assetReader);
        
        // Now load every frame, convert and update vector
        m_loaded_ref.resize(0);
        {
            ScopeTimer timeit("convertTo");
            m_qtime_cache_ref->convertTo (m_loaded_ref);
        }
        _frameCount = m_loaded_ref.size ();
        if (m_auto_run) generate_ssm (0,0);
        return (int) _frameCount;
        
    }
    return -1;

}

void sm_producer::spImpl::loadImages (const images_vector_t& images)
{
    m_loaded_ref.resize(0);
    vector<roiWindow<P8U> >::const_iterator vitr = images.begin();
    do
    {
        m_loaded_ref.push_back (*vitr++);
    }
    while (vitr != images.end());
    _frameCount = m_loaded_ref.size ();
    if (m_auto_run) generate_ssm (0,0);
        
}

bool sm_producer::spImpl::done_grabbing () const
{
    // unique lock. forces new shared locks to wait untill this lock is release
    std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );
    
    return _frameCount != 0 && m_qtime_cache_ref->count() == _frameCount;
}



// @todo add sampling and offset
bool sm_producer::spImpl::generate_ssm (int start_frames, int frames)
{
static    double tiny = 1e-10;
    
    std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );
    
    self_similarity_producerRef simi = std::make_shared<self_similarity_producer<P8U> > (frames, 0);

    
    simi->fill(m_loaded_ref);
    m_entropies.resize (0);
    bool ok = simi->entropies (m_entropies);
    m_SMatrix.resize (0);
    simi->selfSimilarityMatrix(m_SMatrix);
    std::cout << simi->matrixSz() << std::endl;
//    svl::stats<float>::PrintTo(simi->timeStats(), & std::cout);

    
    return ok;
}



template boost::signals2::connection sm_producer::registerCallback(const std::function<sm_producer::sig_cb_content_loaded>&);
template boost::signals2::connection sm_producer::registerCallback(const std::function<sm_producer::sig_cb_frame_loaded>&);


