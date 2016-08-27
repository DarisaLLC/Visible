
#ifndef _UT_QTCACHE_H_
#define _UT_QTCACHE_H_

#include "basic_ut.hpp"
#include "qtime_cache.h"
#include <atomic>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/assert.hpp>

using namespace boost::assign; // bring 'operator+=()' into scope

typedef struct qutExpMapResult {
  QtimeCacheError  expError;
  QtimeCacheStatus expStatus;
  double             expTime;
} qutExpMapResult;

class UT_QtimeCache : public rcUnitTest {
 public:
  UT_QtimeCache(std::string movieFileName);
  ~UT_QtimeCache();

    virtual uint32 run();
 //   int get_which () { return which; }
 //   void set_which (int wh = 0) { which = wh; }
    

 private:
  void ctorTest();
  void mappingTest();
  void simpleAllocTest();
  void prefetchTest();
  void prefetchThreadTest();
  void cacheFullTest();
  void frameBufTest();
  void dtorTest();
 // void threadSafeTest();
  //  void threadSafe (bool);
  //  int which;
    
    
    std::vector<double> times;
    std::vector<double> expected;
    void fill_times ()
    {
        times +=
        0.0,
        0.0335714,
        0.0671429,
        0.100714,
        0.134286,
        0.167857,
        0.201429,
        0.235,
        0.268571,
        0.302143,
        0.335714,
        0.369286,
        0.402857,
        0.436429,
        0.47,
        0.503571,
        0.537143,
        0.570714,
        0.604286,
        0.637857,
        0.671429,
        0.705,
        0.738571,
        0.772143,
        0.805714,
        0.839286,
        0.872857,
        0.906428,
        0.94,
        0.973571,
        1.00714,
        1.04071,
        1.07429,
        1.10786,
        1.14143,
        1.175,
        1.20857,
        1.24214,
        1.27571,
        1.30929,
        1.34286,
        1.37643,
        1.41,
        1.44357,
        1.47714,
        1.51072,
        1.54429,
        1.57786,
        1.61143,
        1.645,
        1.67857,
        1.71214,
        1.74572,
        1.77929,
        1.81286,
        1.84643,
        0., 6., 8., 2000000000., 9000000000.;
    
        expected +=
        0,
        0.0335714,
        0.0671429,
        0.100714,
        0.134286,
        0.167857,
        0.201429,
        0.235,
        0.268571,
        0.302143,
        0.335714,
        0.369286,
        0.402857,
        0.436429,
        0.47,
        0.503571,
        0.537143,
        0.570714,
        0.604286,
        0.637857,
        0.671429,
        0.705,
        0.738571,
        0.772143,
        0.805714,
        0.839286,
        0.872857,
        0.906428,
        0.94,
        0.973571,
        1.00714,
        1.04071,
        1.07429,
        1.10786,
        1.14143,
        1.175,
        1.20857,
        1.24214,
        1.27571,
        1.30929,
        1.34286,
        1.37643,
        1.41,
        1.44357,
        1.47714,
        1.51072,
        1.54429,
        1.57786,
        1.61143,
        1.645,
        1.67857,
        1.71214,
        1.74572,
        1.77929,
        1.81286,
        1.84643, 1.84643, 1.84643, 1.84643,1.84643, 1.84643;
   
    }
    
};

// Thread-safe test specific stuff


#define QtimeCache_THREAD_CNT 5
#define QtimeCache_SZ         (QtimeCache_THREAD_CNT - 1)

class utQtimeCacheThread
{
public:
    utQtimeCacheThread (QtimeCache& cache);
  
    void run ();
  
  uint32      _ref_count[QtimeCache_THREAD_CNT*5];
  std::vector<uint32>    _frameTouch;
  uint32      _prefetches;
  uint32      _myErrors;
  uint32 _frame_count;
  QtimeCache&  _cache;
    static std::atomic<int> startTest;
};



#endif // _UT_QtimeCache_H_
