#ifndef __ASYNC_PRODUCER__
#define __ASYNC_PRODUCER__


#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <array>
#include <sstream>
#include <typeindex>
#include <map>
#include <future>
#include "core/bitvector.h"
#include "timestamp.h"
#include "time_index.h"
#include "vision/opencv_utils.hpp"

// *****************
// *                *
// *  Tracks        *
// *                *
// *****************

// Begining of track types. For now, just a sequence of index/time and doubles

typedef std::vector<index_time_t>  index_time_vec_t;

// track of timed result type of double
typedef std::pair<index_time_t, double> timed_double_t;
typedef std::vector<timed_double_t>  timed_double_vec_t;
typedef std::pair<std::string, timed_double_vec_t> namedTrackOfdouble_t;
typedef std::vector<namedTrackOfdouble_t>  vectorOfnamedTrackOfdouble_t;
typedef std::vector<vectorOfnamedTrackOfdouble_t>  vectorOfVectorOfnamedTrackOfdouble_t;

// track of timed result type of cv::Mat
typedef std::pair<index_time_t, cv::Mat> timed_mat_t;
typedef std::vector<timed_mat_t>  timed_mat_vec_t;
typedef std::pair<std::string, timed_mat_vec_t> namedTrackOfmat_t;
typedef std::vector<namedTrackOfmat_t>  vectorOfnamedTrackOfmat_t;
typedef std::vector<vectorOfnamedTrackOfmat_t>  vectorOfVectorOfnamedTrackOfmat_t;


// *****************
// *                *
// *  Async         *
// *                *
// *****************


// Steps:
// 1. create a global function generating the non-future vector
// 2. launch the async call: std::async(std::launch::async, generating function, args .... )
// 3. Check if the async result is ready. If it is copy it to the non-future.
// Using
// template<typename R>
// bool is_ready(std::future<R> const& f)

typedef std::future<std::shared_ptr<std::vector<namedTrackOfdouble_t>>> async_vectorOfnamedTrackOfdouble_t;
typedef std::future<std::shared_ptr<std::vector<namedTrackOfmat_t>>> async_vectorOfnamedTrackOfmat_t;

template<typename R>
bool is_ready(std::future<R> const& f)

{ return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }



// A different Track design Unused
template<typename T, uint32_t N = 1>
class track : public std::array<T,N>
{
public:
    typedef std::array<T,N> result_array_t;
    inline static uint32_t count () { return N; }
    track () { clear (); }
    track (const index_time_t& ti) : mIt (ti) {}
    void clear () { mIt.first = 0; mIt.second = time_spec_t (); for (T v : *this) v = 0; }
    
private:
    index_time_t mIt;

};


typedef track<double, 1> track1D;
typedef track<double, 3> track3D;




// SynchronisedQueue.hpp
//
// A queue class that has thread synchronisation.
//

#include <string>
#include <queue>


// Queue class that has thread synchronisation
template <typename T>
class SynchronisedQueue
{
private:
    std::queue<T> m_queue;				// Use stl queue to store data
    std::mutex m_mutex;				// The mutex to synchronise on
    std::condition_variable m_cond;	// The condition to wait for
    
public:
    
    // Add data to the queue and notify others
    void Enqueue(const T& data)
    {
        // Acquire lock on the queue
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Add the data to the queue
        m_queue.push(data);
        
        // Notify others that data is ready
        m_cond.notify_one();
        
    } // Lock is automatically released here
    
    // Get data from the queue. Wait for data if not available
    T Dequeue()
    {
        // Acquire lock on the queue
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // When there is no data, wait till someone fills it.
        // Lock is automatically released in the wait and obtained again after the wait
        while (m_queue.size()==0) m_cond.wait(lock);
        
        // Retrieve the data from the queue
        T result=m_queue.front(); m_queue.pop();
        return result;
        
    } // Lock is automatically released here
};


#endif

