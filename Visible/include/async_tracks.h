#ifndef __ASYNC_TRACKS__
#define __ASYNC_TRACKS__


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
//#include "timestamp.h"
#include "time_index.h"
#include "vision/opencv_utils.hpp"

// *****************
// *                *
// *  Tracks        *
// *                *
// *****************

// Begining of track types. For now, just a sequence of index/time and doubles

typedef std::vector<index_time_t>  index_time_vec_t; // vector of pair of [ index, time_spec ]

// Timed value
template<class T>
using timed_t = std::pair<index_time_t, T>;

// Vector of timed value
template<class T, template<typename ELEM, typename ALLOC = std::allocator<ELEM>> class CONT = std::vector >
using timed_vec_t = CONT<timed_t<T>>;

// pair [ name, vector of timed values ]
template<class T>
using track_t = std::pair<std::string, timed_vec_t<T>>;

template<class T,template<typename ELEM, typename ALLOC = std::allocator<ELEM>> class CONT = std::vector >
using tracks_vec_t = CONT<track_t<T>>;

template<class T, template<typename ELEM, typename ALLOC = std::allocator<ELEM>> class CONT = std::vector >
using tracks_array_t = CONT<tracks_vec_t<T>>;

// track of timed result type of float
typedef timed_t<float> timedVal_t;  // Timed value
typedef timed_vec_t<float> timedVecOfVals_t;  // Vector of timed value
typedef track_t<float> namedTrack_t; // pair [ name, vector of timed values ]
typedef tracks_vec_t<float>  vecOfNamedTrack_t; // vector of pairs of  [ name, vector of timed values ]
typedef tracks_array_t<float>  arrayOfNamedTracks_t;

//
//// track of timed result type of cv::Mat
//typedef std::pair<index_time_t, cv::Mat> timed_mat_t;
//typedef std::vector<timed_mat_t>  timed_mat_vec_t;
//typedef std::pair<std::string, timed_mat_vec_t> namedTrack_Ofmat_t;
//typedef std::vector<namedTrack_Ofmat_t>  vecOfNamedTrack_Ofmat_t;
//typedef std::vector<vecOfNamedTrack_Ofmat_t>  arrayOfNamedTracks_Ofmat_t;


/*
 namedTrack_t : first name, second a vector of pair: timedVal_t (index_t, double)
 index_t is int64_t and time_spec_t
 */
template<typename T>
void domainFromPairedTracks_D (const namedTrack_t& src, std::vector<T>& times, std::vector<T>& values){
    
    const timedVecOfVals_t& data = src.second;
    
    const auto get_second = [](auto const& pair) -> auto const& { return pair.second; };
    
    // Get the values
    std::transform(std::begin(data), std::end(data),
                   std::back_inserter(values),
                   get_second);
    
    // Get the domain -- timestamp
    const auto get_first = [](auto const& pair) -> auto const& { return pair.first.first; };
    
    // Get the times in milliseconds
    // Get the values
    std::transform(std::begin(data), std::end(data),
                   std::back_inserter(times),
                   get_first);
}

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

typedef std::future<std::shared_ptr<std::vector<namedTrack_t>>> async_vecOfNamedTrack_t;

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

