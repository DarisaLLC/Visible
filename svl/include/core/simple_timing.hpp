#ifndef _SIMPLE_TIMING_H_
#define _SIMPLE_TIMING_H_

#include <cmath>
#include <string>
#include <chrono>
#include <iostream>

using namespace std::chrono;

  /** \brief Posix based timer. Reports time since last reset
    * 
    */

  class chronometer
  {
    public:
      chronometer ()
      {
        reset ();
      }

      template<typename TT = std::chrono::microseconds>
      int64_t getTime ()
      {
        auto duration = std::chrono::duration_cast<TT>(std::chrono::steady_clock::now () - start_time_);
          return duration.count();
      }

     
      inline void
      reset ()
      {
        start_time_ = std::chrono::steady_clock::now ();
      }

    protected:
      std::chrono::steady_clock::time_point start_time_;
  };

  /** \brief ScopeTimer
    * @todo take a callback to call on destruction
    * Uses destructor to read time from ctor
    * create an instance at the beginning of the function. Example:
    *
    * {
    *   civf::ScopeTime t1 ("doit");
    *   // ... 
    * }
    */
class ScopeTimer : public chronometer
  {
    public:
      inline ScopeTimer (const char* title = " scope_timer " ) : chronometer (), title_(title)
      {
      }

      inline ~ScopeTimer ()
      {
        auto val = this->getTime<std::chrono::nanoseconds> ();
        std::cerr << title_ << " took " << val << " nanosecs.\n";
      }

    private:
      std::string title_;
  };


#ifndef MEASURE_FUNCTION_TIME
#define MEASURE_FUNCTION_TIME \
  ScopeTime scopeTime(__func__)
#endif

/// Executes code, only if secs are gone since last exec.
#ifndef DO_EVERY_TS
#define DO_EVERY_TS(secs, currentTime, code) \
if (1) {\
  static double s_lastDone_ = 0.0; \
  double s_now_ = (currentTime); \
  if (s_lastDone_ > s_now_) \
    s_lastDone_ = s_now_; \
  if ((s_now_ - s_lastDone_) > (secs)) {        \
    code; \
    s_lastDone_ = s_now_; \
  }\
} else \
  (void)0
#endif

/// Executes code, only if secs are gone since last exec.
#ifndef DO_EVERY
#define DO_EVERY(secs, code) \
  DO_EVERY_TS(secs, chronometer::get_ptime(), code)
#endif




#endif  



