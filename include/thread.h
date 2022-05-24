#ifndef __THREADS__
#define __THREADS__

#include <memory>
#include <mutex>
#include <thread>


using std::mutex;
using std::recursive_mutex;
using std::thread;
typedef std::lock_guard<mutex> lock_guard;
typedef std::lock_guard<recursive_mutex> recursive_lock_guard;





#endif

